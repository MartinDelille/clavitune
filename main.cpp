#include <ApplicationServices/ApplicationServices.h>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <stk/ADSR.h>
#include <stk/BlitSquare.h>
#include <stk/JCRev.h>
#include <stk/OnePole.h>
#include <stk/RtAudio.h>
#include <stk/Stk.h>
#include <thread>

std::mutex mtx;

std::condition_variable cv;
bool running = true;

enum class KeyAction { Down, Up };

struct KeyEvent {
  KeyAction action;
};

std::queue<KeyEvent> keyEvents;

struct NoteEvent {
  stk::StkFloat frequency;
  KeyAction action;
  stk::StkFloat cutoff;
};

std::queue<NoteEvent> noteEvents;

std::vector<stk::StkFloat> notes = {261.63, 293.66, 329.63, 349.23,
                                    392.00, 440.00, 493.88, 523.25};

void composerLoop() {
  while (running) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return !keyEvents.empty() || !running; });
    if (!running) {
      break;
    }

    auto keyEvent = keyEvents.front();
    keyEvents.pop();

    NoteEvent noteEvent;
    noteEvent.frequency = notes[rand() % notes.size()];
    noteEvent.action = keyEvent.action;
    noteEvent.cutoff = (rand() % 50 + 50) / 100.;
    noteEvents.push(noteEvent);
  }
}

stk::BlitSquare *oscillator = nullptr;
stk::ADSR *adsr = nullptr;
stk::OnePole *filter = nullptr;
stk::JCRev *reverb = nullptr;

int tick(void *outputBuffer, void *, unsigned int nFrames, double,
         RtAudioStreamStatus, void *userData) {
  assert(oscillator != nullptr);
  assert(adsr != nullptr);
  assert(filter != nullptr);
  assert(reverb != nullptr);
  float *buffer = static_cast<float *>(outputBuffer);

  if (!noteEvents.empty()) {
    auto noteEvent = noteEvents.front();
    noteEvents.pop();
    if (noteEvent.action == KeyAction::Down) {
      oscillator->setFrequency(noteEvent.frequency);
      adsr->keyOn();
      filter->setPole(noteEvent.cutoff);
    } else if (noteEvent.action == KeyAction::Up) {
      adsr->keyOff();
    }
  }
  for (unsigned int i = 0; i < nFrames; i++) {
    float sample = oscillator->tick();
    sample *= adsr->tick();
    sample = filter->tick(sample);
    sample = reverb->tick(sample);

    buffer[2 * i] = sample;
    buffer[2 * i + 1] = sample;
  }

  return 0;
}

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event,
                    void *refcon) {
  CGKeyCode keyCode =
      (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  if (type == kCGEventKeyDown) {
    keyEvents.push({KeyAction::Down});
    cv.notify_one();
  } else if (type == kCGEventKeyUp) {
    keyEvents.push({KeyAction::Up});
    cv.notify_one();
  }

  return event;
}

int main() {
  stk::Stk::setSampleRate(44100);

  RtAudio dac;
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.nChannels = 2;

  std::cout << "Using device: " << dac.getDeviceInfo(parameters.deviceId).name
            << std::endl;
  unsigned int bufferFrames = 256;
  oscillator = new stk::BlitSquare();
  adsr = new stk::ADSR();
  filter = new stk::OnePole();
  reverb = new stk::JCRev();
  reverb->setEffectMix(0.99);
  adsr->setAttackTime(0.01);
  adsr->setDecayTime(0.1);
  adsr->setSustainLevel(0.8);
  adsr->setReleaseTime(0.5);

  dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, 44100, &bufferFrames,
                 tick);

  dac.startStream();

  CGEventMask mask =
      CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);

  CFMachPortRef eventTap =
      CGEventTapCreate(kCGHIDEventTap,        // capture at HID level
                       kCGHeadInsertEventTap, // insert before others
                       kCGEventTapOptionDefault, mask, callback, nullptr);

  if (!eventTap) {
    fprintf(stderr, "Failed to create event tap\n");
    return 1;
  }

  CFRunLoopSourceRef source =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

  CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);

  CGEventTapEnable(eventTap, true);

  std::thread composerThread(composerLoop);

  CFRunLoopRun();

  dac.stopStream();
  dac.closeStream();
  return 0;
}

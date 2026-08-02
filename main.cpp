#include <ApplicationServices/ApplicationServices.h>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <stk/Clarinet.h>
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

    std::cout << "Key pressed, composing..." << std::endl;
    auto keyEvent = keyEvents.front();
    keyEvents.pop();

    noteEvents.push({notes[rand() % notes.size()], keyEvent.action});
  }
}

int tick(void *outputBuffer, void *, unsigned int nFrames, double,
         RtAudioStreamStatus, void *userData) {
  stk::Clarinet *clarinet = static_cast<::stk::Clarinet *>(userData);
  float *buffer = static_cast<float *>(outputBuffer);

  if (!noteEvents.empty()) {
    auto noteEvent = noteEvents.front();
    noteEvents.pop();
    if (noteEvent.action == KeyAction::Down) {
      clarinet->noteOn(noteEvent.frequency, 0.5);
    } else if (noteEvent.action == KeyAction::Up) {
      clarinet->noteOff(0.5);
    }
  }
  for (unsigned int i = 0; i < nFrames; i++) {
    float sample = clarinet->tick();

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
  stk::Clarinet clarinet;

  dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, 44100, &bufferFrames,
                 tick, &clarinet);

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

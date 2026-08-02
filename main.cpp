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
bool wake = false;

bool playNote = false;
stk::StkFloat currentNote = 0.0;

std::vector<stk::StkFloat> notes = {261.63, 293.66, 329.63, 349.23,
                                    392.00, 440.00, 493.88, 523.25};

void composerLoop() {
  while (running) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return wake || !running; });
    if (!running) {
      break;
    }
    wake = false;

    std::cout << "Key pressed, composing..." << std::endl;
    currentNote = notes[rand() % notes.size()];
    playNote = true;
  }
}

int tick(void *outputBuffer, void *, unsigned int nFrames, double,
         RtAudioStreamStatus, void *userData) {
  stk::Clarinet *clarinet = static_cast<::stk::Clarinet *>(userData);
  float *buffer = static_cast<float *>(outputBuffer);

  if (playNote) {
    clarinet->noteOn(currentNote, 0.8);
    playNote = false;
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
  if (type == kCGEventKeyDown) {
    CGKeyCode keyCode =
        (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

    {
      std::lock_guard<std::mutex> lock(mtx);
      wake = true;
    }

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

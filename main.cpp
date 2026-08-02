#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <stk/ADSR.h>
#include <stk/BlitSquare.h>
#include <stk/OnePole.h>
#include <stk/RtAudio.h>
#include <stk/Stk.h>
#include <thread>
#ifdef __linux__
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <vector>
#endif

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

int tick(void *outputBuffer, void *, unsigned int nFrames, double,
         RtAudioStreamStatus, void *userData) {
  assert(oscillator != nullptr);
  assert(adsr != nullptr);
  assert(filter != nullptr);
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
    float sample = filter->tick(oscillator->tick() * adsr->tick());

    buffer[2 * i] = sample;
    buffer[2 * i + 1] = sample;
  }

  return 0;
}

#ifdef __APPLE__
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
#endif

#ifdef __linux__
std::vector<int> findKeyboardFds() {
  std::vector<int> fds;
  DIR *dir = opendir("/dev/input");
  if (!dir) {
    return fds;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strncmp(entry->d_name, "event", 5) != 0) {
      continue;
    }

    std::string path = std::string("/dev/input/") + entry->d_name;
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      continue;
    }

    unsigned long evBits = 0;
    unsigned long keyBits[KEY_MAX / (8 * sizeof(unsigned long)) + 1] = {};
    bool isKeyboard =
        ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), &evBits) >= 0 &&
        (evBits & (1UL << EV_KEY)) &&
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) >= 0 &&
        (keyBits[KEY_A / (8 * sizeof(unsigned long))] &
         (1UL << (KEY_A % (8 * sizeof(unsigned long)))));

    if (isKeyboard) {
      fds.push_back(fd);
    } else {
      close(fd);
    }
  }
  closedir(dir);

  return fds;
}

void linuxKeyboardLoop(std::vector<int> fds) {
  std::vector<struct pollfd> pfds;
  for (int fd : fds) {
    pfds.push_back({fd, POLLIN, 0});
  }

  while (running) {
    if (poll(pfds.data(), pfds.size(), 200) <= 0) {
      continue;
    }

    for (auto &pfd : pfds) {
      if (!(pfd.revents & POLLIN)) {
        continue;
      }

      struct input_event ev;
      while (read(pfd.fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_KEY || ev.value == 2) {
          continue;
        }
        keyEvents.push({ev.value == 1 ? KeyAction::Down : KeyAction::Up});
        cv.notify_one();
      }
    }
  }

  for (int fd : fds) {
    close(fd);
  }
}
#endif

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
  adsr->setAttackTime(0.01);
  adsr->setDecayTime(0.1);
  adsr->setSustainLevel(0.8);
  adsr->setReleaseTime(0.5);

  dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, 44100, &bufferFrames,
                 tick);

  dac.startStream();

#ifdef __APPLE__
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
#elif defined(__linux__)
  auto keyboardFds = findKeyboardFds();
  if (keyboardFds.empty()) {
    fprintf(stderr, "No keyboard device found under /dev/input "
                    "(is your user in the 'input' group?)\n");
    return 1;
  }

  std::thread composerThread(composerLoop);
  std::thread keyboardThread(linuxKeyboardLoop, std::move(keyboardFds));
  keyboardThread.join();
#endif

  dac.stopStream();
  dac.closeStream();
  return 0;
}

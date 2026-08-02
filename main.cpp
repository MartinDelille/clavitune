#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;

std::condition_variable cv;
bool running = true;
bool wake = false;

void composerLoop() {
  while (running) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return wake || !running; });
    if (!running) {
      break;
    }
    wake = false;

    std::cout << "Key pressed, composing..." << std::endl;
  }
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

  return 0;
}

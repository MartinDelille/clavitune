#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event,
                    void *refcon) {
  if (type == kCGEventKeyDown) {
    CGKeyCode keyCode =
        (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

    printf("Key down: %d\n", keyCode);
  }

  return event; // return NULL to suppress the event
}

int main() {
  CGEventMask mask =
      CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);

  CFMachPortRef eventTap =
      CGEventTapCreate(kCGHIDEventTap,        // capture at HID level
                       kCGHeadInsertEventTap, // insert before others
                       kCGEventTapOptionDefault, mask, callback, NULL);

  if (!eventTap) {
    fprintf(stderr, "Failed to create event tap\n");
    return 1;
  }

  CFRunLoopSourceRef source =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

  CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);

  CGEventTapEnable(eventTap, true);

  CFRunLoopRun();

  return 0;
}

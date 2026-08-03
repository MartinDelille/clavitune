# Clavitune

## macOS

System Settings
→ Privacy & Security
→ Input Monitoring
→ enable your app/Terminal

## Linux

Prerequisites: `cmake`, a C++ compiler, and ALSA headers (`libasound2-dev`
on Debian/Ubuntu).

Keyboard capture reads raw events from `/dev/input/event*`, which requires
your user to be in the `input` group:

```
sudo usermod -aG input $USER
```

Log out and back in (or `newgrp input`) for the group change to take
effect.

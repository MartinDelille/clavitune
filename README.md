# Clavitune

## Build

Install `uv`, then build with:

```
make
```

`uv` manages the Conan 2 environment. Conan installs STK on both platforms
and ALSA on Linux. The macOS audio frameworks remain provided by macOS itself.

The build uses committed platform profiles. On Linux, run:

```
make PROFILE=conan/profiles/linux
```

## macOS

System Settings
→ Privacy & Security
→ Input Monitoring
→ enable your app/Terminal

## Linux

Prerequisites: `uv`, `cmake`, and a C++ compiler.

Keyboard capture reads raw events from `/dev/input/event*`, which requires
your user to be in the `input` group:

```
sudo usermod -aG input $USER
```

Log out and back in (or `newgrp input`) for the group change to take
effect.

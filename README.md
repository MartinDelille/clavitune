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

Conan's `libalsa` package doesn't ship the `alsa-plugins` ecosystem (pulse,
pipewire, dmix, ...) and its own default config/plugin paths don't resolve
once the library is relocated out of the Conan cache. `make r` points ALSA
at the system's config and plugins (`/usr/share/alsa/alsa.conf` and the
distro's `alsa-lib` plugin directory) at runtime to work around this — this
requires an ALSA runtime (e.g. `libasound2`) to be installed on the system,
which is normally already the case.

OpenVR SDK
---

# Fork

* This enhances the simplehmd hmd driver to add keyboard controls.
* This also includes a revised version of the openvr specialized for mingw compilation: `openvr\headers\openvr_mingw.hpp`

# Known issue
The vr output window doesn't seem to have proper window focus, so key interaction seems to trigger various windows to pop up. Its not excessive though.

## Installation

1. A Windows x64 release is available [here](https://github.com/ShoryuKyzan/openvr/releases/tag/simplehmd-1.1), but otherwise, build the driver using vs-openvr_samples.sln in visual studio 2019 (see below for more setup details)
2. Copy the driver from openvr\samples\drivers\output\drivers\simplehmd to C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd
3. Open C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd\resources\settings\default.vrsettings. Set `enable` to `true`.
4. Open C:\Program Files (x86)\Steam\steamapps\common\SteamVR\resources\settings\default.vrsettings and set `forcedDriver` to `simplehmd`.
5. Run SteamVR

## Controls

* WASD to move in the X/Z plane
* Q/E to move up/down the Y plane
* R to reset.
* Arrow keys to rotate around X/Y axes
* Page Up/Down to rotate around the Z axis.

# Original README
OpenVR is an API and runtime that allows access to VR hardware from multiple
vendors without requiring that applications have specific knowledge of the
hardware they are targeting. This repository is an SDK that contains the API
and samples. The runtime is under SteamVR in Tools on Steam.

### Documentation

#### Application API

Documentation for the Application API is available on
the [GitHub Wiki](https://github.com/ValveSoftware/openvr/wiki/API-Documentation).

#### Driver API

Current documentation for the Driver API can be found in [docs/Driver_API_Documentation.md](docs/).

* Old driver API documentation can still be found on
  the [GitHub Wiki](https://github.com/ValveSoftware/openvr/wiki/Driver-Documentation).

### About

More information on OpenVR and SteamVR can be found on https://steamvr.com

OpenVR SDK
---

# Fork

* This enhances the simplehmd hmd and simplecontroller driver to add keyboard controls.
* This also includes a revised version of the openvr specialized for mingw compilation: `openvr\headers\openvr_mingw.hpp`
* Initial HMD position/orientation can be set from defaults file.


# Known issues
* The vr output window doesn't seem to have proper window focus, so key interaction seems to trigger various windows to pop up. Its not excessive though.
* Not bug-free. Sometimes client program may crash (most notably after pressing R). Simply restart steamvr.
* Controller left/right and page up/down rotation seem to do the same thing. Unsure why.

## Installation

1. A Windows x64 release is available [here](https://github.com/ShoryuKyzan/openvr/releases/), but otherwise, build the driver using vs-openvr_samples.sln in visual studio 2019 (see below for more setup details)
2. Copy the driver from openvr\samples\drivers\output\drivers\simplehmd (or the zipped folder)  to C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd
  * Optional, only if you wish to use this
3. Copy the driver from openvr\samples\drivers\output\drivers\simplecontroller (or the zipped folder) to C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplecontroller 
  * Optional, only if you wish to use this
4. Open C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd\resources\settings\default.vrsettings. Set `enable` to `true`.
5. Open C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplecontroller\resources\settings\default.vrsettings. Set `enable` to `true`.
6. (Optional) Open C:\Program Files (x86)\Steam\steamapps\common\SteamVR\resources\settings\default.vrsettings and set `forcedDriver` to `simplehmd`.
7. Run SteamVR

## Configuration
Initial HMD position and rotation can be set from this file
`C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd\resources\settings\default.vrsettings`
* `simplehmd_device_hmd`
  * `initialXMeters` - initial X position in meters (unsure if its in meters)
  * `initialYMeters` - initial Y position in meters (unsure if its in meters)
  * `initialZMeters` - initial Z position in meters (unsure if its in meters)
  * `initialYawEuler` - initial Yaw as a euler angle
  * `initialPitchEuler` - initial Pitch as a euler angle
  * `initialRollEuler` - initial Roll as a euler angle

## Controls

* Switching device controlled by keyboard
  * Control+1 - HMD. It is enabled by default
  * Control+2 - Left controller
  * Control+3 - Right controller
  * These are the controls even if one of the 2 drivers are not installed or enabled
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

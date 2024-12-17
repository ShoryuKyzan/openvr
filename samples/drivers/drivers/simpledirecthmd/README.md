# Simple Direct HMD Driver

## Dependencies

- GLFW 3.3 or later
- OpenGL 4.1 or later

## TODO

This driver provides an example on how to add a HMD device to SteamVR.

The device driver has an additional component, `IVRDisplayComponent` which provides the display information for the HMD.

Ensure that the window is in focus when running the sample to see the output. The hmd will slowly move up and down to
demonstrate providing pose data to OpenVR.

## Folder Structure

`simplehmd/` - contains resource files.

`src/` - contains source code.

## Building

### Windows with Visual Studio

1. Download and install GLFW from https://www.glfw.org/
2. Set the GLFW_DIR environment variable to point to your GLFW installation
3. Open the solution in Visual Studio 2019
4. Build the solution

### CMake

Use the solution or cmake in `samples/` to build this driver.
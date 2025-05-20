//============ Copyright (c) Valve Corporation, All rights reserved. ============
#pragma once

#include <array>
#include <string>

#include "openvr_driver.h"
#include <atomic>
#include <thread>

enum MyComponent
{
	MyComponent_a_touch,
	MyComponent_a_click,

	MyComponent_trigger_value,
	MyComponent_trigger_click,

	MyComponent_haptic,

	MyComponent_MAX
};

//-----------------------------------------------------------------------------
// Purpose: Represents a single tracked device in the system.
// What this device actually is (controller, hmd) depends on the
// properties you set within the device (see implementation of Activate)
//-----------------------------------------------------------------------------
class MyControllerDeviceDriver : public vr::ITrackedDeviceServerDriver
{
public:
	MyControllerDeviceDriver( vr::ETrackedControllerRole role );

	vr::EVRInitError Activate( uint32_t unObjectId ) override;

	void EnterStandby() override;

	void *GetComponent( const char *pchComponentNameAndVersion ) override;

	void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize ) override;

	vr::DriverPose_t GetPose() override;

	void Deactivate() override;

	// ----- Functions we declare ourselves below -----

	const std::string &MyGetSerialNumber();

	void MyRunFrame();
	void MyProcessEvent( const vr::VREvent_t &vrevent );

	void MyPoseUpdateThread();

private:
	std::atomic< vr::TrackedDeviceIndex_t > my_controller_index_;

	vr::ETrackedControllerRole my_controller_role_;

	std::string my_controller_model_number_;
	std::string my_controller_serial_number_;

	// variables to keyboard-driven track position, rotation, and input state
	bool input_enabled_;
	float custom_x, custom_y, custom_z;
	float custom_yaw, custom_pitch, custom_roll;

	bool initial_position_set;
	vr::HmdVector3_t hmd_position_initial;
	vr::HmdQuaternion_t hmd_orientation_initial;
	int frame_num;

	std::array< vr::VRInputComponentHandle_t, MyComponent_MAX > input_handles_;

	std::atomic< bool > is_active_;
	std::thread my_pose_update_thread_;
};
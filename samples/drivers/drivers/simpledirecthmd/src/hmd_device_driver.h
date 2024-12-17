//============ Copyright (c) Valve Corporation, All rights reserved. ============
#pragma once

#include <array>
#include <string>
#include <map>
#include <vector>

#include "openvr_driver.h"
#include <atomic>
#include <thread>
#include <GLFW\glfw3.h>

enum MyComponent
{
	MyComponent_system_touch,
	MyComponent_system_click,

	MyComponent_MAX
};

struct MyHMDDisplayDriverConfiguration
{
	int32_t window_x;
	int32_t window_y;

	int32_t window_width;
	int32_t window_height;

	int32_t render_width;
	int32_t render_height;
};

class MyHMDDirectDisplayComponent : public vr::IVRDriverDirectModeComponent
{
public:
	~MyHMDDirectDisplayComponent();
	explicit MyHMDDirectDisplayComponent( const MyHMDDisplayDriverConfiguration &config );

	/** Called to allocate textures for applications to render into.  One of these per eye will be passed back to SubmitLayer each frame. */
	void CreateSwapTextureSet( uint32_t unPid, const SwapTextureSetDesc_t *pSwapTextureSetDesc, SwapTextureSet_t *pOutSwapTextureSet ) override;

	/** Used to textures created using CreateSwapTextureSet.  Only one of the set's handles needs to be used to destroy the entire set. */
	void DestroySwapTextureSet( vr::SharedTextureHandle_t sharedTextureHandle ) override;

	/** Used to purge all texture sets for a given process. */
	void DestroyAllSwapTextureSets( uint32_t unPid ) override;

	/** After Present returns, calls this to get the next index to use for rendering. */
	void GetNextSwapTextureSetIndex( vr::SharedTextureHandle_t sharedTextureHandles[ 2 ], uint32_t( *pIndices )[ 2 ] ) override;

	/** Call once per layer to draw for this frame.  One shared texture handle per eye.  Textures must be created
	* using CreateSwapTextureSet and should be alternated per frame.  Call Present once all layers have been submitted. */
	void SubmitLayer( const SubmitLayerPerEye_t( &perEye )[ 2 ] ) override;

	/** Submits queued layers for display. */
	void Present( vr::SharedTextureHandle_t syncTexture ) override;

private:
	MyHMDDisplayDriverConfiguration config_;

	// OpenGL related members
	GLFWwindow* window_ = nullptr;
	GLuint shader_program_ = 0;
	GLuint vertex_array_ = 0;
	GLuint vertex_buffer_ = 0;
	GLuint texture_id_ = 0;
	
	// Texture set management
	struct SwapTextureSet {
		vr::SharedTextureHandle_t handles[2];
		GLuint textures[2];
		int32_t current_index;
	};
	std::map<uint32_t, std::vector<SwapTextureSet>> texture_sets_by_process_;
	
	bool InitializeGL();
	void ShutdownGL();
	void RenderTexture(GLuint texture);
};

//-----------------------------------------------------------------------------
// Purpose: Represents a single tracked device in the system.
// What this device actually is (controller, hmd) depends on what the
// IServerTrackedDeviceProvider calls to TrackedDeviceAdded and the
// properties within Activate() of the ITrackedDeviceServerDriver class.
//-----------------------------------------------------------------------------
class MyHMDControllerDeviceDriver : public vr::ITrackedDeviceServerDriver
{
public:
	struct KeyboardInput {
		float x = 0.0f;  // Left/Right position (A/D)
		float y = 0.0f;  // Up/Down position (Q/E)
		float z = 0.0f;  // Forward/Back position (W/S)
		float yaw = 0.0f;   // Left/Right rotation (Left/Right arrows)
		float pitch = 0.0f; // Up/Down rotation (Up/Down arrows)
		float roll = 0.0f;  // Roll rotation (Page Up/Page Down)
	};

	MyHMDControllerDeviceDriver();
	vr::EVRInitError Activate( uint32_t unObjectId ) override;
	void EnterStandby() override;
	void *GetComponent( const char *pchComponentNameAndVersion ) override;
	void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize ) override;
	vr::DriverPose_t GetPose() override;
    void UpdateFromKeyboard();
    void Deactivate() override;

    // ----- Functions we declare ourselves below -----
	const std::string &MyGetSerialNumber();
	void MyRunFrame();
	void MyProcessEvent( const vr::VREvent_t &vrevent );
	void MyPoseUpdateThread();

private:
	std::unique_ptr< MyHMDDirectDisplayComponent > my_display_component_;

	std::string my_hmd_model_number_;
	std::string my_hmd_serial_number_;

	std::array< vr::VRInputComponentHandle_t, MyComponent_MAX > my_input_handles_{};
	std::atomic< int > frame_number_;
	std::atomic< bool > is_active_;
	std::atomic< uint32_t > device_index_;

	std::thread my_pose_update_thread_;

	KeyboardInput keyboard_input_;
};

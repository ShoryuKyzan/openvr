//============ Copyright (c) Valve Corporation, All rights reserved. ============
#include "driverlog.h"
#include "vrmath.h"
#include <string.h>
#include <Windows.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <map>
#include "hmd_device_driver.h"


// Let's create some variables for strings used in getting settings.
// This is the section where all of the settings we want are stored. A section name can be anything,
// but if you want to store driver specific settings, it's best to namespace the section with the driver identifier
// ie "<my_driver>_<section>" to avoid collisions
static const char *my_hmd_main_settings_section = "driver_simplehmd";
static const char *my_hmd_display_settings_section = "simplehmd_display";

MyHMDControllerDeviceDriver::MyHMDControllerDeviceDriver()
{
	// Keep track of whether Activate() has been called
	is_active_ = false;

	// We have our model number and serial number stored in SteamVR settings. We need to get them and do so here.
	// Other IVRSettings methods (to get int32, floats, bools) return the data, instead of modifying, but strings are
	// different.
	char model_number[ 1024 ];
	vr::VRSettings()->GetString( my_hmd_main_settings_section, "model_number", model_number, sizeof( model_number ) );
	my_hmd_model_number_ = model_number;

	// Get our serial number depending on our "handedness"
	char serial_number[ 1024 ];
	vr::VRSettings()->GetString( my_hmd_main_settings_section, "serial_number", serial_number, sizeof( serial_number ) );
	my_hmd_serial_number_ = serial_number;

	// Here's an example of how to use our logging wrapper around IVRDriverLog
	// In SteamVR logs (SteamVR Hamburger Menu > Developer Settings > Web console) drivers have a prefix of
	// "<driver_name>:". You can search this in the top search bar to find the info that you've logged.
	DriverLog( "My Dummy HMD Model Number: %s", my_hmd_model_number_.c_str() );
	DriverLog( "My Dummy HMD Serial Number: %s", my_hmd_serial_number_.c_str() );

	// Display settings
	MyHMDDisplayDriverConfiguration display_configuration{};
	display_configuration.window_x = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_x" );
	display_configuration.window_y = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_y" );

	display_configuration.window_width = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_width" );
	display_configuration.window_height = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_height" );

	display_configuration.render_width = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "render_width" );
	display_configuration.render_height = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "render_height" );

	// Instantiate our display component
	my_display_component_ = std::make_unique< MyHMDDirectDisplayComponent >( display_configuration );
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver after our
//  IServerTrackedDeviceProvider calls IVRServerDriverHost::TrackedDeviceAdded.
//-----------------------------------------------------------------------------
vr::EVRInitError MyHMDControllerDeviceDriver::Activate( uint32_t unObjectId )
{
	// Let's keep track of our device index. It'll be useful later.
	// Also, if we re-activate, be sure to set this.
	device_index_ = unObjectId;

	// Set a member to keep track of whether we've activated yet or not
	is_active_ = true;

	// For keeping track of frame number for animating motion.
	frame_number_ = 0;

	// Properties are stored in containers, usually one container per device index. We need to get this container to set
	// The properties we want, so we call this to retrieve a handle to it.
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer( device_index_ );

	// Let's begin setting up the properties now we've got our container.
	// A list of properties available is contained in vr::ETrackedDeviceProperty.

	// First, let's set the model number.
	vr::VRProperties()->SetStringProperty( container, vr::Prop_ModelNumber_String, my_hmd_model_number_.c_str() );

	// Next, display settings

	// Get the ipd of the user from SteamVR settings
	const float ipd = vr::VRSettings()->GetFloat( vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float );
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_UserIpdMeters_Float, ipd );

	// For HMDs, it's required that a refresh rate is set otherwise VRCompositor will fail to start.
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_DisplayFrequency_Float, 0.f );

	// The distance from the user's eyes to the display in meters. This is used for reprojection.
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.f );

	// How long from the compositor to submit a frame to the time it takes to display it on the screen.
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.11f );

	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_IsOnDesktop_Bool, false );

	vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, true);

	// Now let's set up our inputs
	// This tells the UI what to show the user for bindings for this controller,
	// As well as what default bindings should be for legacy apps.
	// Note, we can use the wildcard {<driver_name>} to match the root folder location
	// of our driver.
	vr::VRProperties()->SetStringProperty( container, vr::Prop_InputProfilePath_String, "{simplehmd}/input/mysimplehmd_profile.json" );

	// Let's set up handles for all of our components.
	// Even though these are also defined in our input profile,
	// We need to get handles to them to update the inputs.
	vr::VRDriverInput()->CreateBooleanComponent( container, "/input/system/touch", &my_input_handles_[ MyComponent_system_touch ] );
	vr::VRDriverInput()->CreateBooleanComponent( container, "/input/system/click", &my_input_handles_[ MyComponent_system_click ] );

	my_pose_update_thread_ = std::thread( &MyHMDControllerDeviceDriver::MyPoseUpdateThread, this );

	// We've activated everything successfully!
	// Let's tell SteamVR that by saying we don't have any errors.
	return vr::VRInitError_None;
}

//-----------------------------------------------------------------------------
// Purpose: If you're an HMD, this is where you would return an implementation
// of vr::IVRDisplayComponent, vr::IVRVirtualDisplay or vr::IVRDirectModeComponent.
//-----------------------------------------------------------------------------
void *MyHMDControllerDeviceDriver::GetComponent( const char *pchComponentNameAndVersion )
{
	if ( strcmp( pchComponentNameAndVersion, vr::IVRDisplayComponent_Version ) == 0 )
	{
		return my_display_component_.get();
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when a debug request has been made from an application to the driver.
// What is in the response and request is up to the application and driver to figure out themselves.
//-----------------------------------------------------------------------------
void MyHMDControllerDeviceDriver::DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
{
	if ( unResponseBufferSize >= 1 )
		pchResponseBuffer[ 0 ] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: This is never called by vrserver in recent OpenVR versions,
// but is useful for giving data to vr::VRServerDriverHost::TrackedDevicePoseUpdated.
//-----------------------------------------------------------------------------
vr::DriverPose_t MyHMDControllerDeviceDriver::GetPose()
{
	vr::DriverPose_t pose = { 0 };

	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;

	// Convert Euler angles to quaternion using helper function
	pose.qRotation = HmdQuaternion_FromEulerAngles(keyboard_input_.roll, keyboard_input_.pitch, keyboard_input_.yaw);

	pose.vecPosition[0] = keyboard_input_.x;
	pose.vecPosition[1] = keyboard_input_.y;
	pose.vecPosition[2] = keyboard_input_.z;

	// pose.vecVelocity[0] = keyboard_input_.x;
	// pose.vecVelocity[1] = keyboard_input_.y;
	// pose.vecVelocity[2] = keyboard_input_.z;

	pose.poseIsValid = true;
	pose.deviceIsConnected = true;
	pose.result = vr::TrackingResult_Running_OK;
	pose.shouldApplyHeadModel = true;

	return pose;
}

void MyHMDControllerDeviceDriver::UpdateFromKeyboard()
{
	const float move_speed = 0.01f;
	const float rotate_speed = 0.02f;

	// Reset control
	if (GetAsyncKeyState('R') & 0x8000) {
		keyboard_input_.x = 0.0f;
		keyboard_input_.y = 0.0f; 
		keyboard_input_.z = 0.0f;
		keyboard_input_.yaw = 0.0f;
		keyboard_input_.pitch = 0.0f;
		keyboard_input_.roll = 0.0f;
	}

	// Position controls
	if (GetAsyncKeyState('A') & 0x8000) keyboard_input_.x -= move_speed;  // Left
	if (GetAsyncKeyState('D') & 0x8000) keyboard_input_.x += move_speed;  // Right
	if (GetAsyncKeyState('W') & 0x8000) keyboard_input_.z -= move_speed;  // Forward
	if (GetAsyncKeyState('S') & 0x8000) keyboard_input_.z += move_speed;  // Back
	if (GetAsyncKeyState('Q') & 0x8000) keyboard_input_.y -= move_speed;  // Down
	if (GetAsyncKeyState('E') & 0x8000) keyboard_input_.y += move_speed;  // Up

	// Rotation controls
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) keyboard_input_.yaw -= rotate_speed;   // Turn left
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) keyboard_input_.yaw += rotate_speed;  // Turn right
	if (GetAsyncKeyState(VK_UP) & 0x8000) keyboard_input_.pitch -= rotate_speed;   // Look up
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) keyboard_input_.pitch += rotate_speed; // Look down
	if (GetAsyncKeyState(VK_PRIOR) & 0x8000) keyboard_input_.roll -= rotate_speed; // Roll left (Page Up)
	if (GetAsyncKeyState(VK_NEXT) & 0x8000) keyboard_input_.roll += rotate_speed;  // Roll right (Page Down)
}

void MyHMDControllerDeviceDriver::MyPoseUpdateThread()
{
	while (is_active_)
	{
		UpdateFromKeyboard();
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(device_index_, GetPose(), sizeof(vr::DriverPose_t));
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when the device should enter standby mode.
// The device should be put into whatever low power mode it has.
// We don't really have anything to do here, so let's just log something.
//-----------------------------------------------------------------------------
void MyHMDControllerDeviceDriver::EnterStandby()
{
	DriverLog( "HMD has been put into standby." );
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when the device should deactivate.
// This is typically at the end of a session
// The device should free any resources it has allocated here.
//-----------------------------------------------------------------------------
void MyHMDControllerDeviceDriver::Deactivate()
{
	// Let's join our pose thread that's running
	// by first checking then setting is_active_ to false to break out
	// of the while loop, if it's running, then call .join() on the thread
	if ( is_active_.exchange( false ) )
	{
		my_pose_update_thread_.join();
	}

	// unassign our controller index (we don't want to be calling vrserver anymore after Deactivate() has been called
	device_index_ = vr::k_unTrackedDeviceIndexInvalid;
}


//-----------------------------------------------------------------------------
// Purpose: This is called by our IServerTrackedDeviceProvider when its RunFrame() method gets called.
// It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
void MyHMDControllerDeviceDriver::MyRunFrame()
{
	frame_number_++;
	// update our inputs here
}


//-----------------------------------------------------------------------------
// Purpose: This is called by our IServerTrackedDeviceProvider when it pops an event off the event queue.
// It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
void MyHMDControllerDeviceDriver::MyProcessEvent( const vr::VREvent_t &vrevent )
{
}


//-----------------------------------------------------------------------------
// Purpose: Our IServerTrackedDeviceProvider needs our serial number to add us to vrserver.
// It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
const std::string &MyHMDControllerDeviceDriver::MyGetSerialNumber()
{
	return my_hmd_serial_number_;
}

//-----------------------------------------------------------------------------
// DISPLAY DRIVER METHOD DEFINITIONS
//-----------------------------------------------------------------------------

MyHMDDirectDisplayComponent::MyHMDDirectDisplayComponent( const MyHMDDisplayDriverConfiguration &config )
	: config_( config )
{
	if (!InitializeGL()) {
		DriverLog("Failed to initialize OpenGL");
	}
}

MyHMDDirectDisplayComponent::~MyHMDDirectDisplayComponent() {
	ShutdownGL();
}

bool MyHMDDirectDisplayComponent::InitializeGL() {
	if (!glfwInit()) {
		DriverLog("Failed to initialize GLFW");
		return false;
	}

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window_ = glfwCreateWindow(config_.window_width, config_.window_height, "SimpleDirectHMD", nullptr, nullptr);
	if (!window_) {
		DriverLog("Failed to create GLFW window");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window_);

	// Initialize basic shader program and quad for texture display
	const char* vertex_shader =
		"#version 410\n"
		"layout(location = 0) in vec2 position;\n"
		"layout(location = 1) in vec2 texcoord;\n"
		"out vec2 v_texcoord;\n"
		"void main() {\n"
		"    gl_Position = vec4(position, 0.0, 1.0);\n"
		"    v_texcoord = texcoord;\n"
		"}\n";

	const char* fragment_shader =
		"#version 410\n"
		"in vec2 v_texcoord;\n"
		"uniform sampler2D tex;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"    fragColor = texture(tex, v_texcoord);\n"
		"}\n";

	// Compile vertex shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, nullptr);
	glCompileShader(vs);

	// Check vertex shader compilation
	GLint success = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(vs, sizeof(infoLog), nullptr, infoLog);
		DriverLog("Vertex shader compilation failed: %s", infoLog);
		return false;
	}

	// Compile fragment shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, nullptr);
	glCompileShader(fs);

	// Check fragment shader compilation
	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(fs, sizeof(infoLog), nullptr, infoLog);
		DriverLog("Fragment shader compilation failed: %s", infoLog);
		return false;
	}

	// Create and link shader program
	shader_program_ = glCreateProgram();
	glAttachShader(shader_program_, vs);
	glAttachShader(shader_program_, fs);
	glLinkProgram(shader_program_);

	// Check program linking
	glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(shader_program_, sizeof(infoLog), nullptr, infoLog);
		DriverLog("Shader program linking failed: %s", infoLog);
		return false;
	}

	// Clean up shaders
	glDeleteShader(vs);
	glDeleteShader(fs);

	// Create quad vertices (two triangles forming a rectangle)
	float vertices[] = {
		// positions    // texture coords
		-1.0f,  1.0f,  0.0f, 1.0f,  // top left
		-1.0f, -1.0f,  0.0f, 0.0f,  // bottom left
		 1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
		-1.0f,  1.0f,  0.0f, 1.0f,  // top left
		 1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
		 1.0f,  1.0f,  1.0f, 1.0f   // top right
	};

	// Create and bind VAO
	glGenVertexArrays(1, &vertex_array_);
	glBindVertexArray(vertex_array_);

	// Create and bind VBO
	glGenBuffers(1, &vertex_buffer_);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Set vertex attributes
	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1); // texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// Set texture uniform
	glUseProgram(shader_program_);
	glUniform1i(glGetUniformLocation(shader_program_, "tex"), 0);

	// Enable vsync
	glfwSwapInterval(1);

	return true;
}

void MyHMDDirectDisplayComponent::ShutdownGL() {
	if (shader_program_) {
		glDeleteProgram(shader_program_);
		shader_program_ = 0;
	}
	
	if (vertex_buffer_) {
		glDeleteBuffers(1, &vertex_buffer_);
		vertex_buffer_ = 0;
	}
	
	if (vertex_array_) {
		glDeleteVertexArrays(1, &vertex_array_);
		vertex_array_ = 0;
	}

	if (window_) {
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}
	
	glfwTerminate();
}

void MyHMDDirectDisplayComponent::CreateSwapTextureSet(uint32_t unPid, 
	const SwapTextureSetDesc_t* pSwapTextureSetDesc,
	SwapTextureSet_t* pOutSwapTextureSet) 
{
	SwapTextureSet set;
	set.current_index = 0;
	
	// Create two textures for the swap set
	glGenTextures(2, set.textures);
	for (int i = 0; i < 2; i++) {
		glBindTexture(GL_TEXTURE_2D, set.textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
			pSwapTextureSetDesc->nWidth, pSwapTextureSetDesc->nHeight,
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		set.handles[i] = (vr::SharedTextureHandle_t)set.textures[i];
		pOutSwapTextureSet->rSharedTextureHandles[i] = set.handles[i];
	}
	
	texture_sets_by_process_[unPid].push_back(set);
}

void MyHMDDirectDisplayComponent::DestroySwapTextureSet(vr::SharedTextureHandle_t sharedTextureHandle) {
	// Find and destroy the texture set containing this handle
	for (auto& pair : texture_sets_by_process_) {
		auto& sets = pair.second;
		for (auto it = sets.begin(); it != sets.end(); ++it) {
			if (it->handles[0] == sharedTextureHandle || it->handles[1] == sharedTextureHandle) {
				glDeleteTextures(2, it->textures);
				sets.erase(it);
				return;
			}
		}
	}
}

void MyHMDDirectDisplayComponent::DestroyAllSwapTextureSets(uint32_t unPid) {
	auto it = texture_sets_by_process_.find(unPid);
	if (it != texture_sets_by_process_.end()) {
		for (auto& set : it->second) {
			glDeleteTextures(2, set.textures);
		}
		texture_sets_by_process_.erase(it);
	}
}

void MyHMDDirectDisplayComponent::GetNextSwapTextureSetIndex(
	vr::SharedTextureHandle_t sharedTextureHandles[2], uint32_t(*pIndices)[2]) 
{
	// Toggle between 0 and 1 for double buffering
	(*pIndices)[0] = (*pIndices)[1] = 0;
}

void MyHMDDirectDisplayComponent::SubmitLayer(const SubmitLayerPerEye_t(&perEye)[2]) {
	// Store the textures to be rendered in Present()
	for (int eye = 0; eye < 2; eye++) {
		// Render the texture for each eye
		GLuint tex = (GLuint)perEye[eye].hTexture;
		RenderTexture(tex);
	}
}

void MyHMDDirectDisplayComponent::Present(vr::SharedTextureHandle_t syncTexture) {
	// Swap buffers to display the rendered content
	glfwSwapBuffers(window_);
	glfwPollEvents();
}

void MyHMDDirectDisplayComponent::RenderTexture(GLuint texture) {
	glClear(GL_COLOR_BUFFER_BIT);
	
	glUseProgram(shader_program_);
	glBindVertexArray(vertex_array_);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


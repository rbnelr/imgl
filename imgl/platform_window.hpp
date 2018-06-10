
#include "glad.h"

#include "windows.h"
#include "Shlobj.h"

#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32 1
#include "glfw3native.h"

#include "simple_file_io.hpp"
bool write_blob (string const& name, void const* data, uptr sz) {
	return write_fixed_size_binary_file(name, data, sz);
}
bool load_blob (string const& name, void* data, uptr sz) {
	return load_fixed_size_binary_file(name, data, sz);
}

void APIENTRY ogl_debuproc (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, cstr message, void const* userParam) {

	//if (source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB) return;

	// hiding irrelevant infos/warnings
	switch (id) {
		case 131185: // Buffer detailed info (where the memory lives which is supposed to depend on the usage hint)
		//case 1282: // using shader that was not compiled successfully
		//
		//case 2: // API_ID_RECOMPILE_FRAGMENT_SHADER performance warning has been generated. Fragment shader recompiled due to state change.
		//case 131218: // Program/shader state performance warning: Fragment shader in program 3 is being recompiled based on GL state.
		//
		//			 //case 131154: // Pixel transfer sync with rendering warning
		//
		//			 //case 1282: // Wierd error on notebook when trying to do texture streaming
		//			 //case 131222: // warning with unused shadow samplers ? (Program undefined behavior warning: Sampler object 0 is bound to non-depth texture 0, yet it is used with a program that uses a shadow sampler . This is undefined behavior.), This might just be unused shadow samplers, which should not be a problem
		//			 //case 131218: // performance warning, because of shader recompiling based on some 'key'
			return;
	}

	cstr src_str = "<unknown>";
	switch (source) {
		case GL_DEBUG_SOURCE_API_ARB:				src_str = "GL_DEBUG_SOURCE_API_ARB";				break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:		src_str = "GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB";		break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:	src_str = "GL_DEBUG_SOURCE_SHADER_COMPILER_ARB";	break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:		src_str = "GL_DEBUG_SOURCE_THIRD_PARTY_ARB";		break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:		src_str = "GL_DEBUG_SOURCE_APPLICATION_ARB";		break;
		case GL_DEBUG_SOURCE_OTHER_ARB:				src_str = "GL_DEBUG_SOURCE_OTHER_ARB";				break;
	}

	cstr type_str = "<unknown>";
	switch (source) {
		case GL_DEBUG_TYPE_ERROR_ARB:				type_str = "GL_DEBUG_TYPE_ERROR_ARB";				break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:	type_str = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB";	break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:	type_str = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB";	break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:			type_str = "GL_DEBUG_TYPE_PORTABILITY_ARB";			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:			type_str = "GL_DEBUG_TYPE_PERFORMANCE_ARB";			break;
		case GL_DEBUG_TYPE_OTHER_ARB:				type_str = "GL_DEBUG_TYPE_OTHER_ARB";				break;
	}

	cstr severity_str = "<unknown>";
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH_ARB:			severity_str = "GL_DEBUG_SEVERITY_HIGH_ARB";		break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:			severity_str = "GL_DEBUG_SEVERITY_MEDIUM_ARB";		break;
		case GL_DEBUG_SEVERITY_LOW_ARB:				severity_str = "GL_DEBUG_SEVERITY_LOW_ARB";			break;
	}

	fprintf(stderr, "OpenGL debug proc: severity: %s src: %s type: %s id: %d  %s\n",
		severity_str, src_str, type_str, id, message);
}

RectI to_rect (RECT const& r) {
	return { s32v2(r.left,r.top), s32v2(r.right,r.bottom) };
}

struct Window {
	GLFWwindow*			window = nullptr;
	s32v2				framebuffer_size_px = 0;

	bool				fullscreen = false;

	WINDOWPLACEMENT		win32_windowplacement = {}; // Zeroed
	void get_win32_windowplacement () {
		GetWindowPlacement(glfwGetWin32Window(window), &win32_windowplacement);
	}
	void set_win32_windowplacement () {
		SetWindowPlacement(glfwGetWin32Window(window), &win32_windowplacement);
	}

	bool is_open () {
		return window != nullptr;
	}

	~Window () {
		if (is_open())
			close_window();
	}

	s32v2 get_size () {
		return framebuffer_size_px;
	}


	void toggle_fullscreen () {
		if (!fullscreen) {
			
			get_win32_windowplacement();

			GLFWmonitor* fullscreen_monitor = nullptr;
			{
				int count;
				GLFWmonitor** monitors = glfwGetMonitors(&count);

				if (monitors)
					fullscreen_monitor = monitors[0];
			}

			GLFWvidmode const* mode = glfwGetVideoMode(fullscreen_monitor);
			glfwSetWindowMonitor(window, fullscreen_monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

		} else {
			auto r = to_rect(win32_windowplacement.rcNormalPosition);
			glfwSetWindowMonitor(window, NULL, r.low.x,r.low.y, r.get_size().x,r.get_size().y, 0);

		}

		need_to_set_swap_interval = true;

		fullscreen = !fullscreen;
	}

	void save_window_positioning () {
		
		if (fullscreen) {
			// keep window positioning that we got when we switched to fullscreen
		} else {
			get_win32_windowplacement();
		}

		if (!write_fixed_size_binary_file("saves/window_placement.bin", &win32_windowplacement, sizeof(win32_windowplacement))) {
			fprintf(stderr, "Could not save window_placement to saves/window_placement.bin, window position and size won't be restored on the next launch of this app.");
		}
	}
	bool load_window_positioning () {
		return load_fixed_size_binary_file("saves/window_placement.bin", &win32_windowplacement, sizeof(win32_windowplacement));
	}

	//
	void open_window (std::string const& title, s32v2 default_size) { // should only happen once
		
		glfwSetErrorCallback(glfw_error_proc);

		assert(glfwInit() != 0);

		bool placement_loaded = load_window_positioning();

		fullscreen = false; // always start in windowed mode

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		bool gl_vaos_required = true;

		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

		s32v2 size = placement_loaded ? to_rect(win32_windowplacement.rcNormalPosition).get_size() : default_size;

		window = glfwCreateWindow(size.x,size.y, title.c_str(), NULL, NULL);

		glfwSetWindowUserPointer(window, this);

		if (placement_loaded) {
			set_win32_windowplacement();
		}

		glfwSetKeyCallback(window,			glfw_key_event);
		glfwSetCharModsCallback(window,		glfw_char_event);
		glfwSetMouseButtonCallback(window,	glfw_mouse_button_event);
		glfwSetScrollCallback(window,		glfw_mouse_scroll);

		glfwMakeContextCurrent(window);

		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		if (GLAD_GL_ARB_debug_output) {
			glDebugMessageCallbackARB(ogl_debuproc, 0);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

			// without GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB ogl_debuproc needs to be thread safe
		}

		GLuint vao; // one global vao for everything

		if (gl_vaos_required) {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
		}

	}

	// blocks on resize
	bool begin_window (e_input_mode input_mode=WAIT_FOR_INPUT) {
		mouse_wheel_diff = 0;
		do_toggle_fullscreen = false;

		if (		input_mode == WAIT_FOR_INPUT ) {
			glfwWaitEvents();
		} else if ( input_mode == POLL_FOR_INPUT ) {
			glfwPollEvents();
		} else assert(not_implemented);

		if (do_toggle_fullscreen)
			toggle_fullscreen();

		glfwGetFramebufferSize(window, &framebuffer_size_px.x,&framebuffer_size_px.y);

		{
			double x,y;
			glfwGetCursorPos(window, &x, &y);
			mcursor_pos_px = s32v2((int)x,(int)y);
		}

		bool keep_open = !glfwWindowShouldClose(window);
		return keep_open;
	}
	void end_window (e_display_mode display_mode=VSYNC) {
		
		set_vsync(display_mode);

		glfwSwapBuffers(window);
	}

	void close_window () {
		
		save_window_positioning();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	//
	bool need_to_set_swap_interval = true; // need to set vsync mode initially

	void set_vsync (e_display_mode display_mode) {
		#if 1 // optimize this to only get called on changed display_mode
		static e_display_mode current_display_mode = display_mode;
		need_to_set_swap_interval = need_to_set_swap_interval || current_display_mode == display_mode;

		if (!need_to_set_swap_interval)
			return;
		#endif

		int interval = 1;

		if (display_mode == VSYNC)
			interval = 1; // -1 swap interval technically requires a extension check which requires wgl loading in addition to gl loading
		else if (display_mode == UNLIMITED)
			interval = 0;
		else assert(not_implemented);

		glfwSwapInterval(interval);
	}

	static void glfw_error_proc (int err, cstr msg) {
		fprintf(stderr, "GLFW Error! 0x%x '%s'\n", err, msg);
	}

	bool lmb_down = false;
	bool rmb_down = false;

	s32v2 mcursor_pos_px = 0;
	int mouse_wheel_diff = 0;

	bool do_toggle_fullscreen = false;

	static void glfw_mouse_button_event (GLFWwindow* window, int button, int action, int mods) {
		Window* obj = (Window*)glfwGetWindowUserPointer(window);

		bool went_down = action == GLFW_PRESS;

		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				obj->lmb_down = went_down;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				obj->rmb_down = went_down;
				break;
		}
	}
	static void glfw_mouse_scroll (GLFWwindow* window, double xoffset, double yoffset) {
		Window* obj = (Window*)glfwGetWindowUserPointer(window);

		obj->mouse_wheel_diff += (int)yoffset;
	}
	static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods) {
		Window* obj = (Window*)glfwGetWindowUserPointer(window);

		ImGuiIO& io = ImGui::GetIO();
		
		if (key >= 0 && key < ARRLEN(io.KeysDown))
			io.KeysDown[key] = action != GLFW_RELEASE;
		
		// from https://github.com/ocornut/imgui/blob/master/examples/opengl3_example/imgui_impl_glfw_gl3.cpp
		(void)mods; // Modifiers are not reliable across systems
		io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
		io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
		io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

		switch (key) {
			case GLFW_KEY_F11: {
				if (action == GLFW_PRESS)
					obj->do_toggle_fullscreen = true;
			} break;
		}
	}
	static void glfw_char_event (GLFWwindow* window, unsigned int codepoint, int mods) {
		Window* obj = (Window*)glfwGetWindowUserPointer(window);

		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharacter((char)codepoint);
	}
};

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <cassert>

constexpr bool not_implemented = false; // use like: assert(not_implemented);

#include "basic_typedefs.hpp"
#include "vector.hpp"
#include "colors.hpp"

enum e_input_mode {
	POLL_FOR_INPUT,
	WAIT_FOR_INPUT,
};
enum e_display_mode {
	VSYNC,
	UNLIMITED,
};

struct Rect {
	fv2	low;
	fv2	high;

	fv2 get_size () {
		return high -low;
	}
};
struct RectI {
	s32v2	low;
	s32v2	high;

	s32v2	get_size () {
		return high -low;
	}
};

// abstracts window opening, opengl context init, window position saving, vsync, etc.  (window buffer clearing currently also happens here)
// changing OS and or graphics library (DirectX, Vulkan, etc.) would require a different header here
#include "platform_window.hpp"

Window window; // if this represents an open window then the destructor will always close the window at the end of the program, so close_window is optional

struct Region {
	Rect	rect_window_px;

	fv2 full_avail_size_px () { return rect_window_px.get_size(); }
};

Region get_current_region () {
	return {{ 0, (fv2)window.get_size() }};
}

bool begin_window (std::string const& title, rgb8 clear_col=0, e_input_mode input_mode=WAIT_FOR_INPUT, s32v2 default_size=s32v2(1280,720)) {
	if (!window.is_open()) {
		window.open_window(title, default_size);
	}

	bool keep_open = window.begin_window(input_mode);

	auto sz = window.get_size();
	glScissor(0,0, sz.x,sz.y);
	glViewport(0,0, sz.x,sz.y);

	rgbf col = (rgbf)clear_col / 255;
	glClearColor(col.x, col.y, col.z, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	return keep_open;
}

void end_window (e_display_mode display_mode=VSYNC) {
	window.end_window(display_mode);
}

void close_window () {
	window.close_window();
}

//
#include "rendering.hpp" // abstracts how

void text (std::string const& str, std::string font="c:/windows/fonts/arial.ttf") {
	int lines = count_lines(str);

	auto reg = get_current_region();

	f32 avail_height_px = reg.full_avail_size_px().y;
	f32 desired_line_h_px = avail_height_px / (float)lines;

	auto* sized_font = cached_font(font, desired_line_h_px);

	render_text(window, reg, str, sized_font, desired_line_h_px);
}


/*
	Example usage of the library
*/

#include "imgl.hpp"

void draw () {
	text("Training in\nProgress...");
}

int main () {
	
	for (;;) {
		if (!begin_window("Mein Tolles Grafikprogramm", rgb8(255,128,39), POLL_FOR_INPUT))
			break; // Window was closed via a standart close command (Close button or Alt-F4)

		//begin_imgui();

		bool my_own_exit_condition = false;
		if (my_own_exit_condition)
			break;

		draw();

		//end_imgui();

		//end_window(vsync_mode(MAX_REFRESH_RATE));
		end_window(VSYNC);
	}
	//close_window(); // optional: explicitly close window, but window will be closed anyway at program exit

	return 0;
}

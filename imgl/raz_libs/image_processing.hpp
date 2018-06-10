#pragma once

void flip_vertical_inplace (void* rows, uptr row_size, uptr rows_count) {
	for (uptr row=0; row<rows_count/2; ++row) {
		char* row_a = (char*)rows +row_size * row;
		char* row_b = (char*)rows +row_size * (rows_count -1 -row);
		for (uptr i=0; i<row_size; ++i) {
			std::swap(row_a[i], row_b[i]);
		}
	}
}

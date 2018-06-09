
#include "font.hpp"

/*
#include "sorted_vector.hpp"
#include <memory>
using std::unique_ptr;
using std::make_unique;

struct Cached_Texture_Less { // for sorted_vector
	inline bool operator() (Sized_Font const& l,		Sized_Font const& r) const {		return std::less<string>()(l.filepath, r.filepath); }
	inline bool operator() (string const& l_filepath,	Sized_Font const& r) const {		return std::less<string>()(l_filepath, r.filepath); }
	inline bool operator() (Sized_Font const& l,		string const& r_filepath) const {	return std::less<string>()(l.filepath, r_filepath); }
};
sorted_vector<unique_ptr<Sized_Font>, Cached_Texture_Less> cached_fonts;

Sized_Font* cached_font (std::string font_filepath, f32 desired_line_h_px) {
	auto it = cached_fonts.find(font_filepath);
	if (it == cached_fonts.end()) {
		it = cached_fonts.insert( make_unique<Sized_Font>( load_font_file(font_filepath) ) );
	}
	return it->get();
}
*/

#include <vector>

#include "texture.hpp"
using namespace gl_texture;

#include "shader.hpp"

void flip_vertical_inplace (void* rows, uptr row_size, uptr rows_count) {
	for (uptr row=0; row<rows_count/2; ++row) {
		char* row_a = (char*)rows +row_size * row;
		char* row_b = (char*)rows +row_size * (rows_count -1 -row);
		for (uptr i=0; i<row_size; ++i) {
			std::swap(row_a[i], row_b[i]);
		}
	}
}

struct Sized_Font {
	stbtt_bakedchar chars['~' -' ']; // all ascii

	Texture2D atlas;
	s32v2 atlas_size;

	int get_char_index (char c) {
		if (!(c >= ' ' && c <= '~'))
			return -1;
		return c -' ';
	}
};
Sized_Font load_font_file (std::string font_filepath, f32 desired_line_h_px) {
	Blob file_data;
	assert(load_binary_file(font_filepath, &file_data));
	
	f32 desired_font_h_px = desired_line_h_px; // this is not correct i think

	typedef u8 pixel;

	int atlas_w = 512;
	int atlas_h = atlas_w;
	std::vector<pixel> atlas_img (atlas_w * atlas_h);

	Sized_Font sf;

	int used_rows;

	for (;;) {
		auto ret = stbtt_BakeFontBitmap((u8*)file_data.data,0, desired_font_h_px,
										atlas_img.data(), atlas_w,atlas_h,
										' ', '~' -' ', // all ascii glyphs
										sf.chars);
		if (ret > 0) {
			// success
			used_rows = ret;
			break;
		}
		// fail, not all glyphs could fit into the atlas, try again with bigger atlas

		atlas_w *= 2;
		atlas_h = atlas_w;
		atlas_img.resize(atlas_w * atlas_h);
		
		assert(atlas_w <= 1024*4);
	}

	atlas_h = used_rows;
	atlas_img.resize(atlas_w * atlas_h);

	flip_vertical_inplace(&atlas_img[0], atlas_w * sizeof(pixel), atlas_h);

	sf.atlas_size = s32v2(atlas_w,atlas_h);
	sf.atlas = single_mip_texture(PF_LR8, atlas_img.data(), sf.atlas_size, FILTER_BILINEAR, FILTER_NO_ANISO);

	return sf;
}

Sized_Font* cached_font (std::string font_filepath, f32 desired_line_h_px) {
	static Sized_Font one_font_for_now = std::move( load_font_file(font_filepath, desired_line_h_px) );
	return &one_font_for_now;
}

bool is_newline_c (char c) {	return c == '\n' || c == '\r'; }
bool newline (char** pcur) {
	char* cur = *pcur;

	char c = *cur;
	if (!is_newline_c(c)) {
		return false;
	}

	++cur;

	char c2 = *cur;
	if (is_newline_c(c2) && c2 != c) {
		++cur; // skip '\n\r' and '\r\n'
	}

	*pcur = cur;
	return true;
}

int count_lines (std::string const& s) {
	int lines = 1;

	char* cur = (char*)s.c_str();

	while (*cur != '\0') {
		if (newline(&cur)) {
			++lines;
		} else {
			++cur;
		}
	}

	return lines;
}

struct Textured_Vertex {
	fv2		pos_screen;
	fv2		uv;
	rgba8	col;
};
void draw_textured (std::vector<Textured_Vertex> const& vbo_data, Texture2D const& tex) {
	
	auto alloc_vbo = [] () {
		GLuint vbo;
		glGenBuffers(1, &vbo);
		return vbo;
	};
	static GLuint vbo = alloc_vbo();

	auto bind_vbos = [&] (Shader const& shad, GLuint vbo) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		GLint loc_pos =		glGetAttribLocation(shad.get_prog_handle(), "attr_pos_screen");
		GLint loc_uv =		glGetAttribLocation(shad.get_prog_handle(), "attr_uv");
		GLint loc_col =		glGetAttribLocation(shad.get_prog_handle(), "attr_col");

		glEnableVertexAttribArray(loc_pos);
		glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, pos_screen));

		glEnableVertexAttribArray(loc_uv);
		glVertexAttribPointer(loc_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, uv));

		glEnableVertexAttribArray(loc_col);
		glVertexAttribPointer(loc_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, col));
	};

	static auto shad = load_shader("shaders/textured.vert", "shaders/textured.frag");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	glUseProgram(shad.get_prog_handle());

	static GLint loc_screen_dim = glGetUniformLocation(shad.get_prog_handle(), "screen_dim");
	glUniform2f(loc_screen_dim, (f32)window.get_size().x,(f32)window.get_size().y);


	static GLint tex_unit = 0;

	static GLint tex_loc = glGetUniformLocation(shad.get_prog_handle(), "tex");
	glUniform1i(tex_loc, tex_unit);

	static GLint draw_wireframe_loc = glGetUniformLocation(shad.get_prog_handle(), "draw_wireframe");
	glUniform1i(draw_wireframe_loc, false);

	bind_texture(tex_unit, tex);


	bind_vbos(shad, vbo);

	if (vbo_data.size() > 0) {
		glBufferData(GL_ARRAY_BUFFER, vbo_data.size() * sizeof(Textured_Vertex), NULL, GL_STREAM_DRAW); // buffer orphan
		glBufferData(GL_ARRAY_BUFFER, vbo_data.size() * sizeof(Textured_Vertex), &vbo_data[0], GL_STREAM_DRAW);

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vbo_data.size());
	}
}
void push_rect (std::vector<Textured_Vertex>* vbo_data, fv2 pos_l, fv2 pos_h, fv2 uv_l, fv2 uv_h, rgba8 col) {
	for (fv2 p : { fv2(1,0), fv2(1,1), fv2(0,0), fv2(0,0), fv2(1,1), fv2(0,1) } )
		vbo_data->push_back({ lerp(pos_l,pos_h,p), lerp(uv_l,uv_h,p), col });
}

void render_text (Window const& wnd, Region reg, std::string const& text, Sized_Font* font, f32 desired_line_h_px) { // start new line at newline chars
	
	std::vector<Textured_Vertex> vbo_data;
	
	fv2 cur_point = reg.rect_window_px.low +fv2(0, 300);
	
	char* cur = (char*)text.c_str();

	while (*cur != '\0') {
		
		if (newline(&cur)) {
			cur_point.x = 0;
			cur_point.y += 500;
			continue;
		}

		int char_index = font->get_char_index(*cur++);
		if (char_index == -1) {
			continue; // just skip for now
		}

		stbtt_aligned_quad quad;
		stbtt_GetBakedQuad(font->chars, font->atlas_size.x,font->atlas_size.y, char_index, &cur_point.x,&cur_point.y, &quad, true);

		// i flipped the atlas, so that the texture is not flipped vertically in opengl debuggers, so i need to flip the v of the uvs too
		quad.t0 = 1 -quad.t0;
		quad.t1 = 1 -quad.t1;

		push_rect(&vbo_data, fv2(quad.x0,quad.y0),fv2(quad.x1,quad.y1), fv2(quad.s0,quad.t0),fv2(quad.s1,quad.t1), 255);
	}

	draw_textured(vbo_data, font->atlas);
}

void sdf_test () {
	auto std_test_tex = [] () {
		Blob file_data;
		assert(load_binary_file("c:/windows/fonts/arial.ttf", &file_data));

		stbtt_fontinfo fontinfo;

		assert(stbtt_InitFont(&fontinfo, (u8*)file_data.data, 0));


		s32v2 sdf_glyph_origin;

		s32v2 sdf_size;
		u8* sdf_pixels = stbtt_GetCodepointSDF(&fontinfo, stbtt_ScaleForPixelHeight(&fontinfo, 220), (int)'a', 50, 180, 180.0f/50, &sdf_size.x,&sdf_size.y, &sdf_glyph_origin.x,&sdf_glyph_origin.y);

		flip_vertical_inplace(sdf_pixels, sdf_size.x * sizeof(u8), sdf_size.y);

		auto sdf_tex = single_mip_texture(PF_LR8, sdf_pixels, sdf_size, FILTER_BILINEAR, FILTER_NO_ANISO);

		return sdf_tex;
	};
	static auto tex = std_test_tex();

	fv2 full_sz = (fv2)window.get_size();
	fv2 sz = min(full_sz.x,full_sz.y) * 0.9f;

	fv2 pos = (full_sz -sz) / 2;

	std::vector<Textured_Vertex> vbo_data;
	push_rect(&vbo_data, pos, pos+sz, fv2(0,1),fv2(1,0), 255);

	draw_textured(vbo_data, tex);
}

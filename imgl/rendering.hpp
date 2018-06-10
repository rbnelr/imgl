
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

#include "shader.hpp"
#include "texture.hpp"
using namespace gl_texture;

#include "image_processing.hpp"

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

#include "opengl_draw.hpp"

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

	if (vbo_data.size() > 0)
		draw_textured(&vbo_data[0], vbo_data.size(), font->atlas);
}

void sdf_test () {
	struct Sized_Tex {
		Texture2D	tex;
		s32v2		size = 0;
	};
	
	auto std_test_tex = [] (char codepoint, std::string fontpath, f32 font_height, f32 padding_ratio, f32 onedge_val) {
		Sized_Tex sdf;
		
		Blob file_data;
		if (!load_binary_file(fontpath.c_str(), &file_data))
			return sdf;

		stbtt_fontinfo fontinfo;

		if (!stbtt_InitFont(&fontinfo, (u8*)file_data.data, 0))
			return sdf;

		if (codepoint == (int)' ')
			return sdf;

		f32 padding = font_height * padding_ratio;

		s32v2 sdf_glyph_origin;

		u8* sdf_pixels = stbtt_GetCodepointSDF(&fontinfo, stbtt_ScaleForPixelHeight(&fontinfo, font_height), (int)codepoint, padding, onedge_val, onedge_val / padding, &sdf.size.x,&sdf.size.y, &sdf_glyph_origin.x,&sdf_glyph_origin.y);

		flip_vertical_inplace(sdf_pixels, sdf.size.x * sizeof(u8), sdf.size.y);

		sdf.tex = single_mip_texture(PF_LR8, sdf_pixels, sdf.size, FILTER_BILINEAR, FILTER_NO_ANISO);

		stbtt_FreeSDF(sdf_pixels, nullptr);

		return sdf;
	};
	static Sized_Tex tex;

	static bool regen = true;

	static std::string codepoint = "a";
	static std::string fontpath = "c:/windows/fonts/arial.ttf";
	static f32 font_height = 64;
	static f32 padding_ratio = 0.2f;
	static f32 onedge_val = 180;

	regen = ImGui::InputText_str("codepoint", &codepoint) || regen;
	regen = ImGui::InputText_str("fontpath", &fontpath) || regen;
	regen = ImGui::DragFloat("font_height", &font_height, 0.05f) || regen;
	regen = ImGui::DragFloat("padding_ratio", &padding_ratio, 0.005f) || regen;
	regen = ImGui::DragFloat("onedge_val", &onedge_val, 0.05f) || regen;

	if (regen)
		tex = std_test_tex(codepoint.size() >= 1 ? codepoint[0] : '_', fontpath, font_height, padding_ratio, onedge_val);
	regen = false;

	ImGui::Value("glyph size", tex.size);

	fv2 full_sz = (fv2)window.get_size();
	fv2 sz = min(full_sz.x,full_sz.y) * 0.9f;
	sz.x = sz.y * ((f32)tex.size.x / (f32)tex.size.y);

	fv2 pos = (full_sz -sz) / 2;

	std::vector<Textured_Vertex> vbo_data;
	push_rect(&vbo_data, pos, pos+sz, fv2(0,1),fv2(1,0), 255);

	static auto shad = load_shader("shaders/sdf_font.vert", "shaders/sdf_font.frag");

	if (vbo_data.size() > 0 && all(tex.size > 0))
		draw_textured(&vbo_data[0], vbo_data.size(), tex.tex, shad);
}

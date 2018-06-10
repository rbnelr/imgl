
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

struct SDF_Font_Glyph {
	s32v2	atlas_pos;
	s32v2	size = 0;
	s32v2	glyph_origin = 0;

	int		stbtt_glyph_index = -1;
};
struct SDF_Font_Atlas {
	Texture2D	tex;
	s32v2		size = 0;

	Blob			ttf_file_data;

	stbtt_fontinfo	stbtt_fontinfo;

	static constexpr int glyphs_first = ' ';
	static constexpr int glyphs_last = '~';
	static constexpr int glyphs_count = glyphs_last -glyphs_first +1;

	bool is_valid () { return tex.get_handle() != 0; }

	SDF_Font_Glyph	glyphs[glyphs_count]; // ASCII range

	SDF_Font_Glyph* get_codepoint (int c) {
		if (!(c >= glyphs_first && c <= glyphs_last))
			return nullptr;
		return &glyphs[c -glyphs_first];
	}
};

void sdf_test () {
	
	auto std_test_tex = [] (std::string fontpath, f32 font_height, int padding, u8 onedge_val, f32* val_delta_per_texel) {
		SDF_Font_Atlas sdf;
		
		if (!load_binary_file(fontpath.c_str(), &sdf.ttf_file_data))
			return sdf;

		if (!stbtt_InitFont(&sdf.stbtt_fontinfo, (u8*)sdf.ttf_file_data.data, 0))
			return sdf;

		f32 scale = stbtt_ScaleForPixelHeight(&sdf.stbtt_fontinfo, font_height);

		*val_delta_per_texel = (f32)onedge_val / (f32)padding;
		
		typedef u8 pixel;

		struct Glyph {
			pixel*	sdf_pixels = nullptr;

			~Glyph () {
				stbtt_FreeSDF(sdf_pixels, nullptr);
			}
		};
		Glyph glyphs[SDF_Font_Atlas::glyphs_count] = {};
		stbrp_rect	stbrp_rects[SDF_Font_Atlas::glyphs_count] = {};

		for (int i=0; i<SDF_Font_Atlas::glyphs_count; ++i) {
			auto& g = glyphs[i];
			auto& output_g = sdf.glyphs[i];

			output_g.stbtt_glyph_index = stbtt_FindGlyphIndex(&sdf.stbtt_fontinfo, (int)(SDF_Font_Atlas::glyphs_first +i));

			g.sdf_pixels = stbtt_GetGlyphSDF(&sdf.stbtt_fontinfo, scale, output_g.stbtt_glyph_index, padding, onedge_val, *val_delta_per_texel, &output_g.size.x,&output_g.size.y, &output_g.glyph_origin.x,&output_g.glyph_origin.y);

			auto& rect = stbrp_rects[i];

			rect.w = output_g.size.x;
			rect.h = output_g.size.y;
		}

		int atlas_width = 256;
		int atlas_height = atlas_width;

		int used_height;
		bool all_packed = false;
		while (!all_packed) {
			stbrp_context	context;
			int				num_nodes = atlas_width;
			std::vector<stbrp_node>	nodes(num_nodes);

			stbrp_init_target(&context, atlas_width, atlas_height, &nodes[0], num_nodes);

			bool res = stbrp_pack_rects(&context, stbrp_rects, SDF_Font_Atlas::glyphs_count) != 0;

			all_packed = true;
			used_height = 0;

			for (int i=0; i<SDF_Font_Atlas::glyphs_count; ++i) {
				auto& r = stbrp_rects[i];
				
				if (!r.was_packed) {
					all_packed = false;
					break;
				}
				used_height = max(used_height, r.y +r.h);

				sdf.glyphs[i].atlas_pos = s32v2(r.x, r.y);
			}

			assert(all_packed == res);
			
			if (!all_packed) {
				atlas_width *= 2;
				atlas_height = atlas_width;
			}
		}


		std::vector<pixel> atlas_pixels(atlas_width * atlas_height, 0); // clear to zero

		for (int i=0; i<SDF_Font_Atlas::glyphs_count; ++i) {
			auto& r = stbrp_rects[i];

			for (int y=0; y<r.h; ++y) {
				for (int x=0; x<r.w; ++x) {
					s32v2 out_pixel = s32v2(r.x +x, r.y +y);

					atlas_pixels[out_pixel.y * atlas_width +out_pixel.x] = glyphs[i].sdf_pixels[y * r.w +x];
				}
			}
		}

		sdf.size = s32v2(atlas_width,atlas_height);
		
		flip_vertical_inplace(&atlas_pixels[0], sdf.size.x * sizeof(u8), sdf.size.y);
		
		sdf.tex = single_mip_texture(PF_LR8, &atlas_pixels[0], sdf.size, FILTER_BILINEAR, FILTER_NO_ANISO);

		return sdf;
	};
	static SDF_Font_Atlas sdf_atlas;

	static bool regen = true;

	static std::string fontpath = "c:/windows/fonts/times.ttf";
	static f32 font_height = 64;
	static f32 padding_ratio = 0.05f;
	static int onedge_val = 180;

	regen = ImGui::InputText_str("fontpath", &fontpath) || regen;
	regen = ImGui::DragFloat("font_height", &font_height, 0.05f) || regen;

	regen = ImGui::DragFloat("padding_ratio", &padding_ratio, 0.005f) || regen;
	regen = ImGui::DragInt("onedge_val", &onedge_val, 0.05f, 0,255) || regen;

	static f32 sdf_delta_per_texel;

	if (regen)
		sdf_atlas = std_test_tex(fontpath, font_height, (int)roundf(font_height * padding_ratio), (u8)onedge_val, &sdf_delta_per_texel);
	regen = false;

	static f32 outline_ratio = 0;//0.03f;
	ImGui::DragFloat("outline_ratio", &outline_ratio, 0.005f);

	float outline = font_height * outline_ratio;

	ImGui::Value("glyph size", sdf_atlas.size);
	ImGui::Value("outline", outline);
	ImGui::Value("sdf_delta_per_texel", sdf_delta_per_texel);

	fv2 sdf_delta_per_uv_unit = (sdf_delta_per_texel / 255) * (fv2)sdf_atlas.size;
	f32 outline_delta = (sdf_delta_per_texel / 255) * outline;

	static f32 display_scale = 470;
	ImGui::DragFloat("display size", &display_scale);

	std::vector<Textured_Vertex> vbo_data;
	std::vector<Solid_Vertex> dbg_overlay_vbo_data;
	if (0 && sdf_atlas.is_valid()) {
		fv2 full_sz = (fv2)window.get_size();
		fv2 sz = min(full_sz.x,full_sz.y) * 0.9f;
		sz.x = sz.y * ((f32)sdf_atlas.size.x / (f32)sdf_atlas.size.y);

		fv2 pos = (full_sz -sz) / 2;

		push_rect(&vbo_data, pos, pos+sz, fv2(0,1),fv2(1,0), 255);
	} else if (sdf_atlas.is_valid()) {
		
		cstr text = "Training in\nProgress...";
		char* cur = (char*)text;

		fv2 rect_p = fv2(500, 200);
		fv2 rect_sz = fv2(800, 500);

		push_rect_outline(&dbg_overlay_vbo_data, rect_p, rect_p +rect_sz, rgba8(100,255,100,255));

		f32 scale = stbtt_ScaleForPixelHeight(&sdf_atlas.stbtt_fontinfo, display_scale);

		f32 ascent, descent, line_gap;
		{
			int iascent, idescent, iline_gap;
			stbtt_GetFontVMetrics(&sdf_atlas.stbtt_fontinfo, &iascent, &idescent, &iline_gap);

			ascent =	scale * (f32)iascent;
			descent =	scale * (f32)idescent;
			line_gap =	scale * (f32)iline_gap;
		}

		fv2 point = rect_p +fv2(0,ascent);

		while (*cur != '\0')  {
			
			if (newline(&cur)) {
				point.x = rect_p.x;
				point.y += -descent +line_gap +ascent;
				continue;
			}

			auto* glyph = sdf_atlas.get_codepoint(*cur);
			if (!glyph) {
				// not in atlas
			} else {
				
				fv2 pos_l = point +(0					+(fv2)glyph->glyph_origin) * (scale / stbtt_ScaleForPixelHeight(&sdf_atlas.stbtt_fontinfo, font_height));
				fv2 pos_h = point +((fv2)glyph->size	+(fv2)glyph->glyph_origin) * (scale / stbtt_ScaleForPixelHeight(&sdf_atlas.stbtt_fontinfo, font_height));

				fv2 uv_l = map( (fv2)glyph->atlas_pos +0,					0, (fv2)sdf_atlas.size);
				fv2 uv_h = map( (fv2)glyph->atlas_pos +(fv2)glyph->size,	0, (fv2)sdf_atlas.size);

				uv_l.y = 1 -uv_l.y;
				uv_h.y = 1 -uv_h.y;

				push_rect(&vbo_data, pos_l,pos_h, uv_l,uv_h, 255);
			}

			f32 advance;
			{
				int iadvance, ileftSideBearing;
				stbtt_GetGlyphHMetrics(&sdf_atlas.stbtt_fontinfo, glyph->stbtt_glyph_index, &iadvance, &ileftSideBearing);
				advance = scale * (f32)iadvance;
			}

			static bool do_kerning = true;
			static ImGuiOnceUponAFrame once;
			if (once) ImGui::Checkbox("kerning", &do_kerning);
			
			if (do_kerning && cur[1] != '\0') {
				auto* next_glyph = sdf_atlas.get_codepoint(cur[1]);

				if (next_glyph) {
					advance += scale * stbtt_GetGlyphKernAdvance(&sdf_atlas.stbtt_fontinfo, glyph->stbtt_glyph_index, next_glyph->stbtt_glyph_index);
				}
			}

			point.x += advance;

			++cur;
		}
	}

	static auto shad = load_shader("shaders/sdf_font.vert", "shaders/sdf_font.frag");

	static f32 aa_px_dist = 1;
	ImGui::DragFloat("aa_px_dist", &aa_px_dist, 0.05f);

	if (shad.get_prog_handle() != 0) {
		glUseProgram(shad.get_prog_handle());

		glUniform1f(glGetUniformLocation(shad.get_prog_handle(), "onedge_val"), (f32)onedge_val / 255);
		glUniform1f(glGetUniformLocation(shad.get_prog_handle(), "outline_delta"), outline_delta);
		glUniform2fv(glGetUniformLocation(shad.get_prog_handle(), "sdf_delta_per_uv_unit"), 1, &sdf_delta_per_uv_unit.x);
		glUniform1f(glGetUniformLocation(shad.get_prog_handle(), "aa_px_dist"), aa_px_dist);

		if (vbo_data.size() > 0 && all(sdf_atlas.size > 0))
			draw_textured(&vbo_data[0], vbo_data.size(), sdf_atlas.tex, shad);
	}

	if (dbg_overlay_vbo_data.size())
		draw_solid(&dbg_overlay_vbo_data[0], dbg_overlay_vbo_data.size());
}

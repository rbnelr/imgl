#pragma once

#include <vector>

#include "shader.hpp"
#include "texture.hpp"
using namespace gl_texture;

#include "vector.hpp"
#include "colors.hpp"

GLuint alloc_vbo () {
	GLuint vbo;
	glGenBuffers(1, &vbo);
	return vbo;
}

struct Solid_Vertex {
	fv2		pos_screen;
	rgba8	col;

	static void setup_attributes (Shader const& shad, GLuint vbo) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		GLint loc_pos =		glGetAttribLocation(shad.get_prog_handle(), "attr_pos_screen");
		GLint loc_col =		glGetAttribLocation(shad.get_prog_handle(), "attr_col_srgba");

		glEnableVertexAttribArray(loc_pos);
		glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE, sizeof(Solid_Vertex), (void*)offsetof(Solid_Vertex, pos_screen));

		glEnableVertexAttribArray(loc_col);
		glVertexAttribPointer(loc_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Solid_Vertex), (void*)offsetof(Solid_Vertex, col));
	}
};
void push_rect (std::vector<Solid_Vertex>* vbo_data, fv2 pos_l, fv2 pos_h, rgba8 col=255) {
	for (fv2 p : { fv2(1,0), fv2(1,1), fv2(0,0), fv2(0,0), fv2(1,1), fv2(0,1) } )
		vbo_data->push_back({ lerp(pos_l,pos_h,p), col });
}
void push_quad (std::vector<Solid_Vertex>* vbo_data, fv2 a, fv2 b, fv2 c, fv2 d, rgba8 col=255) {
	for (fv2 p : { fv2(1,0), fv2(1,1), fv2(0,0), fv2(0,0), fv2(1,1), fv2(0,1) } )
		vbo_data->push_back({ lerp( lerp(a,b,p.x), lerp(d,c,p.x), p.y ), col });
}
void push_line (std::vector<Solid_Vertex>* vbo_data, fv2 a, fv2 b, rgba8 col=255, f32 width=1) {
	fv2 dir = b -a;
	fv2 ndir = normalize(dir);
	fv2 normal = rotate2_90(ndir);

	fv2 quad_a = a -normal * width/2;
	fv2 quad_b = a +normal * width/2;
	fv2 quad_c = b +normal * width/2;
	fv2 quad_d = b -normal * width/2;

	push_quad(vbo_data, quad_a,quad_b,quad_c,quad_d, col);
}
void push_rect_outline (std::vector<Solid_Vertex>* vbo_data, fv2 pos_l, fv2 pos_h, rgba8 col=255, f32 line_width=1) {
	fv2 a = pos_l;
	fv2 b = fv2(pos_h.x, pos_l.y);
	fv2 c = pos_h;
	fv2 d = fv2(pos_l.x, pos_h.y);

	push_line(vbo_data, a,b, col, line_width);
	push_line(vbo_data, b,c, col, line_width);
	push_line(vbo_data, c,d, col, line_width);
	push_line(vbo_data, d,a, col, line_width);
}

void draw_solid (Solid_Vertex const* vertex_data, uptr vertex_count, Shader const& shad) {

	static GLuint vbo = alloc_vbo();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	glUseProgram(shad.get_prog_handle());

	glUniform2f(glGetUniformLocation(shad.get_prog_handle(), "screen_dim"), (f32)window.get_size().x,(f32)window.get_size().y);

	glUniform1i(glGetUniformLocation(shad.get_prog_handle(), "draw_wireframe"), false);

	Solid_Vertex::setup_attributes(shad, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Solid_Vertex), NULL, GL_STREAM_DRAW); // buffer orphan
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Solid_Vertex), vertex_data, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertex_count);
}
void draw_solid (Solid_Vertex const* vertex_data, uptr vertex_count) {

	static auto shad = load_shader("shaders/solid.vert", "shaders/solid.frag");

	draw_solid(vertex_data, vertex_count, shad);
}

struct Textured_Vertex {
	fv2		pos_screen;
	fv2		uv;
	rgba8	col;

	static void setup_attributes (Shader const& shad, GLuint vbo) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		GLint loc_pos =		glGetAttribLocation(shad.get_prog_handle(), "attr_pos_screen");
		GLint loc_uv =		glGetAttribLocation(shad.get_prog_handle(), "attr_uv");
		GLint loc_col =		glGetAttribLocation(shad.get_prog_handle(), "attr_col_srgba");

		glEnableVertexAttribArray(loc_pos);
		glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, pos_screen));

		glEnableVertexAttribArray(loc_uv);
		glVertexAttribPointer(loc_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, uv));

		glEnableVertexAttribArray(loc_col);
		glVertexAttribPointer(loc_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Textured_Vertex), (void*)offsetof(Textured_Vertex, col));
	}
};
void push_rect (std::vector<Textured_Vertex>* vbo_data, fv2 pos_l, fv2 pos_h, fv2 uv_l, fv2 uv_h, rgba8 col) {
	for (fv2 p : { fv2(1,0), fv2(1,1), fv2(0,0), fv2(0,0), fv2(1,1), fv2(0,1) } )
		vbo_data->push_back({ lerp(pos_l,pos_h,p), lerp(uv_l,uv_h,p), col });
}

void draw_textured (Textured_Vertex const* vertex_data, uptr vertex_count, Texture2D const& tex, Shader const& shad) {

	static GLuint vbo = alloc_vbo();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);

	glUseProgram(shad.get_prog_handle());

	glUniform2f(glGetUniformLocation(shad.get_prog_handle(), "screen_dim"), (f32)window.get_size().x,(f32)window.get_size().y);

	glUniform1i(glGetUniformLocation(shad.get_prog_handle(), "draw_wireframe"), false);


	GLint tex_unit = 0;

	glUniform1i(glGetUniformLocation(shad.get_prog_handle(), "tex"), tex_unit);

	bind_texture(tex_unit, tex);


	Textured_Vertex::setup_attributes(shad, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Textured_Vertex), NULL, GL_STREAM_DRAW); // buffer orphan
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Textured_Vertex), vertex_data, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertex_count);
}
void draw_textured (Textured_Vertex const* vertex_data, uptr vertex_count, Texture2D const& tex) {

	static auto shad = load_shader("shaders/textured.vert", "shaders/textured.frag");

	draw_textured(vertex_data, vertex_count, tex, shad);
}

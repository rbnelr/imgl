#pragma once

#include <string>

#include "glad.h"
#include "glfw3.h"

#include "simple_file_io.hpp"

#include "assert.h"

// default alloc results in "empty" class that still destructs, but the destructor does nothing since it's empty
// no copy ctor/assign only move
// use like:
/*
	class T {
		void* my_resource = nullptr;

	public:
		MOVE_ONLY_FACTORY_PRODUCED_CLASS(T) // move operators implemented with swap

		~T () {
			// destructor can destruct default constructed class
			free(my_resource); // free(nullptr) is ok
			// or
			if (my_resource)
				api_delete(my_resource);
		}

		static T my_factory (args...) {
			T t;
			// construct a T
			return t;
		}
	};
	void swap (T& l, T& r) {
		std::swap(l.my_resource, r.my_resource);
	}
*/
#define MOVE_ONLY_FACTORY_PRODUCED_CLASS(CLASS) \
	friend void swap (CLASS& l, CLASS& r); \
	CLASS () {} \
	CLASS& operator= (CLASS& r) = delete; \
	CLASS (CLASS& r) = delete; \
	CLASS& operator= (CLASS&& r) {	swap(*this, r);	return *this; } \
	CLASS (CLASS&& r) {				swap(*this, r); }

class Shader {
	
	GLuint	prog_handle = 0;

public:
	MOVE_ONLY_FACTORY_PRODUCED_CLASS(Shader);

	~Shader () {
		if (prog_handle != 0)
			glDeleteProgram(prog_handle);
	}

	//
	GLuint	get_prog_handle () const {	return prog_handle; }

	static bool get_shader_compile_log (GLuint shad, std::string* log) {
		GLsizei log_len;
		{
			GLint temp = 0;
			glGetShaderiv(shad, GL_INFO_LOG_LENGTH, &temp);
			log_len = (GLsizei)temp;
		}

		if (log_len <= 1) {
			return false; // no log available
		} else {
			// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in str, so we have to allocate one additional char and then resize it away at the end

			log->resize(log_len);

			GLsizei written_len = 0;
			glGetShaderInfoLog(shad, log_len, &written_len, &(*log)[0]);
			assert(written_len == (log_len -1));

			log->resize(written_len);

			return true;
		}
	}
	static bool get_program_link_log (GLuint prog, std::string* log) {
		GLsizei log_len;
		{
			GLint temp = 0;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &temp);
			log_len = (GLsizei)temp;
		}

		if (log_len <= 1) {
			return false; // no log available
		} else {
			// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in str, so we have to allocate one additional char and then resize it away at the end

			log->resize(log_len);

			GLsizei written_len = 0;
			glGetProgramInfoLog(prog, log_len, &written_len, &(*log)[0]);
			assert(written_len == (log_len -1));

			log->resize(written_len);

			return true;
		}
	}

	static bool load_shader (GLenum type, std::string const& filepath, GLuint* shad) {
		*shad = glCreateShader(type);

		std::string source;
		if (!load_text_file(filepath, &source)) {
			fprintf(stderr, "Could not load shader source!\n");
			return false;
		}

		{
			cstr ptr = source.c_str();
			glShaderSource(*shad, 1, &ptr, NULL);
		}

		glCompileShader(*shad);

		bool success;
		{
			GLint status;
			glGetShaderiv(*shad, GL_COMPILE_STATUS, &status);

			std::string log_str;
			bool log_avail = get_shader_compile_log(*shad, &log_str);

			success = status == GL_TRUE;
			if (!success) {
				// compilation failed
				fprintf(stderr, "OpenGL error in shader compilation \"%s\"!\n>>>\n%s\n<<<\n", filepath.c_str(), log_avail ? log_str.c_str() : "<no log available>");
			} else {
				// compilation success
				if (log_avail) {
					fprintf(stderr, "OpenGL shader compilation log \"%s\":\n>>>\n%s\n<<<\n", filepath.c_str(), log_str.c_str());
				}
			}
		}

		return success;
	}
	static Shader load_program (std::string const& vert_filepath, std::string const& frag_filepath) {
		Shader shad;
		
		shad.prog_handle = glCreateProgram();

		GLuint vert;
		GLuint frag;

		bool compile_success = true;

		bool vert_success = load_shader(GL_VERTEX_SHADER,		vert_filepath, &vert);
		bool frag_success = load_shader(GL_FRAGMENT_SHADER,		frag_filepath, &frag);

		if (!(vert_success && frag_success)) {
			glDeleteProgram(shad.prog_handle);
			shad.prog_handle = 0;
			return shad;
		}

		glAttachShader(shad.prog_handle, vert);
		glAttachShader(shad.prog_handle, frag);

		glLinkProgram(shad.prog_handle);

		bool success;
		{
			GLint status;
			glGetProgramiv(shad.prog_handle, GL_LINK_STATUS, &status);

			std::string log_str;
			bool log_avail = get_program_link_log(shad.prog_handle, &log_str);

			success = status == GL_TRUE;
			if (!success) {
				// linking failed
				fprintf(stderr, "OpenGL error in shader linkage \"%s\"|\"%s\"!\n>>>\n%s\n<<<\n", vert_filepath.c_str(), frag_filepath.c_str(), log_avail ? log_str.c_str() : "<no log available>");
			} else {
				// linking success
				if (log_avail) {
					fprintf(stderr, "OpenGL shader linkage log \"%s\"|\"%s\":\n>>>\n%s\n<<<\n", vert_filepath.c_str(), frag_filepath.c_str(), log_str.c_str());
				}
			}
		}

		glDetachShader(shad.prog_handle, vert);
		glDetachShader(shad.prog_handle, frag);

		glDeleteShader(vert);
		glDeleteShader(frag);

		return shad;
	}

};
void swap (Shader& l, Shader& r) {
	std::swap(l.prog_handle, r.prog_handle);
}

Shader load_shader (std::string const& vert_filepath, std::string const& frag_filepath) {
	return Shader::load_program(vert_filepath, frag_filepath);
}

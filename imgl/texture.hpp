#pragma once

#include "glad.h"

#include "vector.hpp"

namespace gl_texture {

	enum pixel_format_e {
		PF_LR8,
	};
	enum minmag_filter_e {
		FILTER_NEAREST,
		FILTER_BILINEAR,
		FILTER_TRILINEAR,
	};
	enum aniso_filter_e {
		FILTER_NO_ANISO,
		FILTER_ANISO,
	};
	enum border_e {
		BORDER_CLAMP,
	};

	// minimal but still useful texture class
	// if you need the size of the texture or the mipmap count just create a wrapper class
	class Texture2D {
		friend void bind_texture (int tet_unit, Texture2D const& tex);
		friend void swap (Texture2D& l, Texture2D& r);

		GLuint	handle = 0;

	public:

		// default alloc unalloced texture
		Texture2D () {}
		// auto free texture
		~Texture2D () {
			if (handle) // maybe this helps to optimize out destructing of unalloced textures
				glDeleteTextures(1, &handle); // would be ok to delete unalloced texture (gpu_handle = 0)
		}

		// no implicit copy
		Texture2D& operator= (Texture2D& r) = delete;
		Texture2D (Texture2D& r) = delete;
		// move operators
		Texture2D& operator= (Texture2D&& r) {	swap(*this, r);	return *this; }
		Texture2D (Texture2D&& r) {				swap(*this, r); }

		//
		GLuint	get_handle () const {	return handle; }

		//
		static Texture2D generate () {
			Texture2D tex;
			glGenTextures(1, &tex.handle);
			return tex;
		}
	
		void upload_mipmap (int mip, pixel_format_e format, u8* pixels, s32v2 size_px) {

			GLenum internal_format;
			GLenum cpu_format;

			switch (format) {
				case PF_LR8:	internal_format = GL_R8;	cpu_format = GL_RED;	break;
				default: assert(not_implemented);
			}
			
			glBindTexture(GL_TEXTURE_2D, handle);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, mip, internal_format, size_px.x,size_px.y, 0, cpu_format, GL_UNSIGNED_BYTE, pixels);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

	private:
		void set_active_mips (int first, int last) { // i am not sure that just changing this actually works correctly
			glBindTexture(GL_TEXTURE_2D, handle);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, first);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, last);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	public:
		
		// single mip
		void upload (pixel_format_e format, u8* pixels, s32v2 size_px) {
			upload_mipmap(0, format, pixels, size_px);

			set_active_mips(0, 0); // no mips
		}
		/* // doesnt work ??
		void gen_mipmaps () {
		glBindTexture(GL_TEXTURE_2D, gpu_handle);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		}
		*/

		void set_minmag_filtering (minmag_filter_e minmag_filter) {
			GLenum min, mag;

			switch (minmag_filter) {
				case FILTER_NEAREST:	min = mag = GL_NEAREST;								break;
				case FILTER_BILINEAR:	min = mag = GL_LINEAR;								break;
				case FILTER_TRILINEAR:	min = GL_LINEAR_MIPMAP_LINEAR;	mag = GL_LINEAR;	break;
			}

			glBindTexture(GL_TEXTURE_2D, handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void enable_filtering_anisotropic (aniso_filter_e aniso_filter) {
			if (GL_ARB_texture_filter_anisotropic) {
				GLfloat aniso;
				if (aniso_filter == FILTER_ANISO) {
					GLfloat max_aniso;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
					aniso = max_aniso;
				} else {
					aniso = 1;
				}
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, aniso);
			}
		}

		void set_border (border_e border) {
			glBindTexture(GL_TEXTURE_2D, handle);
			assert(border == BORDER_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

	};
	void swap (Texture2D& l, Texture2D& r) {
		std::swap(l.handle, r.handle);
	}

	void bind_texture (int tex_unit, Texture2D const& tex) {
		glActiveTexture(GL_TEXTURE0 +tex_unit);
		glBindTexture(GL_TEXTURE_2D, tex.handle);
	}

	Texture2D single_mip_texture (pixel_format_e format, u8* pixels, s32v2 size_px, minmag_filter_e minmag=FILTER_BILINEAR, aniso_filter_e aniso=FILTER_ANISO, border_e border=BORDER_CLAMP) {
		auto tex = Texture2D::generate();
		tex.upload(format, pixels, size_px);
		
		tex.set_minmag_filtering(minmag);
		tex.enable_filtering_anisotropic(aniso);
		tex.set_border(border);

		return tex;
	}
}

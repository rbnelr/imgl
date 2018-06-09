#pragma once

#include <string>
using std::string;

#include "stdio.h"

#include "basic_typedefs.hpp"

bool load_text_file (string const& filepath, string* text) {

	FILE* f = fopen(filepath.c_str(), "rb"); // we don't want "\r\n" to "\n" conversion, because it interferes with our file size calculation
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	text->resize(filesize);

	uptr ret = fread(&(*text)[0], 1,text->size(), f);
	if (ret != (uptr)filesize) return false;

	return true;
}

struct Blob {
	void*	data = nullptr;
	uptr	size = 0;

	//
	static Blob alloc (uptr size) {
		Blob b;
		b.data = malloc(size);
		b.size = size;
		return b;
	}

	// default unallocated
	Blob () {}
	// always auto free
	~Blob () {
		free(data); // ok to free nullptr
	}

	// no implicit copy
	Blob& operator= (Blob& r) = delete;
	Blob (Blob& r) = delete;
	// move operators
	friend void swap (Blob& l, Blob& r);
	Blob& operator= (Blob&& r) {	swap(*this, r);	return *this; }
	Blob (Blob&& r) {				swap(*this, r); }

};
void swap (Blob& l, Blob& r) {
	std::swap(l.data, r.data);
	std::swap(l.size, r.size);
}


bool load_binary_file (string const& filepath, Blob* blob) {

	FILE* f = fopen(filepath.c_str(), "rb"); // we don't want "\r\n" to "\n" conversion
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	auto tmp = Blob::alloc(filesize);

	uptr ret = fread(tmp.data, 1,tmp.size, f);
	if (ret != (uptr)filesize) return false;

	*blob = std::move(tmp);
	return true;
}

bool load_fixed_size_binary_file (string const& filepath, void* data, uptr sz) {

	FILE* f = fopen(filepath.c_str(), "rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (filesize != sz)
		return false;

	uptr ret = fread(data, 1,sz, f);
	if (ret != sz)
		return false;

	return true;
}

bool write_fixed_size_binary_file (string const& filepath, void const* data, uptr sz) {

	FILE* f = fopen(filepath.c_str(), "wb");
	if (!f)
		return false;

	uptr ret = fwrite(data, 1,sz, f);
	if (ret != sz)
		return false;

	return true;
}

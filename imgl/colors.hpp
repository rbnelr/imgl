#pragma once

#include "vector.hpp"

typedef u8v3	rgb8;
typedef u8v4	rgba8;
typedef fv3		rgbf;
typedef fv4		rgbaf;

template <typename T> T to_linear (T srgb) {
	return select(srgb <= T(0.0404482362771082f),
		srgb * (T(1)/T(12.92f)),
		pow( (srgb +T(0.055f)) * T(1/1.055f), T(2.4f) )
		);
}
template <typename T> T to_srgb (T linear) {
	return select(linear <= T(0.00313066844250063f),
		linear * T(12.92f),
		( T(1.055f) * pow(linear, T(1/2.4f)) ) -T(0.055f)
		);
}

rgbf hsl_to_rgb (rgbf hsl) {
#if 0
	// modified from http://www.easyrgb.com/en/math.php
	f32 H = hsl.x;
	f32 S = hsl.y;
	f32 L = hsl.z;

	auto Hue_2_RGB = [] (f32 a, f32 b, f32 vH) {
		if (vH < 0) vH += 1;
		if (vH > 1) vH -= 1;
		if ((6 * vH) < 1) return a +(b -a) * 6 * vH;
		if ((2 * vH) < 1) return b;
		if ((3 * vH) < 2) return a +(b -a) * ((2.0f/3) -vH) * 6;
		return a;
	};

	fv3 rgb;
	if (S == 0) {
		rgb = fv3(L);
	} else {
		f32 a, b;

		if (L < 0.5f)	b = L * (1 +S);
		else			b = (L +S) -(S * L);

		a = 2 * L -b;

		rgb = fv3(	Hue_2_RGB(a, b, H +(1.0f / 3)),
			Hue_2_RGB(a, b, H),
			Hue_2_RGB(a, b, H -(1.0f / 3)) );
	}

	return to_linear(rgb);
#else
	f32 hue = hsl.x;
	f32 sat = hsl.y;
	f32 lht = hsl.z;

	f32 hue6 = hue*6.0f;

	f32 c = sat*(1.0f -abs(2.0f*lht -1.0f));
	f32 x = c * (1.0f -abs(mod(hue6, 2.0f) -1.0f));
	f32 m = lht -(c/2.0f);

	rgbf rgb;
	if (		hue6 < 1.0f )	rgb = rgbf(c,x,0);
	else if (	hue6 < 2.0f )	rgb = rgbf(x,c,0);
	else if (	hue6 < 3.0f )	rgb = rgbf(0,c,x);
	else if (	hue6 < 4.0f )	rgb = rgbf(0,x,c);
	else if (	hue6 < 5.0f )	rgb = rgbf(x,0,c);
	else						rgb = rgbf(c,0,x);
	rgb += m;

	return to_linear(rgb);
#endif
}

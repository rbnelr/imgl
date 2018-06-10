#version 330 core // version 3.3

in		vec2	attr_pos_screen;
in		vec4	attr_col_srgba;

out		vec4	vs_col;

uniform	vec2	screen_dim;

vec3 to_srgb (vec3 linear) {
	bvec3 cutoff = lessThanEqual(linear, vec3(0.00313066844250063));
	vec3 higher = vec3(1.055) * pow(linear, vec3(1.0/2.4)) -vec3(0.055);
	vec3 lower = linear * vec3(12.92);

	return mix(higher, lower, cutoff);
}
vec3 to_linear (vec3 srgb) {
	bvec3 cutoff = lessThanEqual(srgb, vec3(0.0404482362771082));
	vec3 higher = pow((srgb +vec3(0.055)) / vec3(1.055), vec3(2.4));
	vec3 lower = srgb / vec3(12.92);

	return mix(higher, lower, cutoff);
}
vec4 to_srgb (vec4 linear) {	return vec4(to_srgb(linear.rgb), linear.a); }
vec4 to_linear (vec4 srgba) {	return vec4(to_linear(srgba.rgb), srgba.a); }

void main () {
	vec2 pos = attr_pos_screen / screen_dim;
	pos.y = 1 -pos.y; // positions are specified top-down
	
	gl_Position =		vec4(pos * 2 -1, 0,1);
	vs_col =			to_linear(attr_col_srgba);
}

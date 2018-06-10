#version 330 core // version 3.3

in		vec2	vs_uv;
in		vec4	vs_col;

out		vec4	frag_col;

uniform	float	onedge_val;
uniform	float	outline_delta;
uniform	vec2	sdf_delta_per_uv_unit;
uniform	float	aa_px_dist;

uniform sampler2D	tex;

// for wireframe
uniform bool	draw_wireframe = false;

in		vec3	vs_barycentric;

float wireframe_edge_factor () {
	vec3 d = fwidth(vs_barycentric);
	vec3 a3 = smoothstep(vec3(0.0), d*1.5, vs_barycentric);
	return min(min(a3.x, a3.y), a3.z);
}

float map (float val, float in_a, float in_b) {
	return (val -in_a) / (in_b -in_a);
}
float map (float val, float in_a, float in_b, float out_a, float out_b) {
	return mix(out_a, out_b, map(val, in_a, in_b));
}
float saturate (float val) {
	return clamp(val, 0,1);
}

void main () {
	
	float sdf = texture(tex, vs_uv).r;

	if (1 != 0) {

		vec2 uv_size = vec2(dFdx(vs_uv.x), dFdy(vs_uv.y));
		vec2 sdf_delta_per_fragment = sdf_delta_per_uv_unit * uv_size;

		float edge_aa = (aa_px_dist * sdf_delta_per_fragment).x; // x == y should be ?

		float glyph_alpha = saturate(map(sdf, onedge_val -outline_delta -edge_aa/2, onedge_val -outline_delta +edge_aa/2, 0,1));
		float outline_alpha = saturate(map(sdf, onedge_val -edge_aa/2, onedge_val +edge_aa/2, 0,1));

		if (outline_delta == 0)
			outline_alpha = 1;

		frag_col = vec4(1,1,1,glyph_alpha) * mix(vec4(1,1,1,1), vec4(1,0.01,0.01,1), outline_alpha);
	}
	else if (0 != 0) {
		frag_col = vec4(1,1,1,sdf) * vs_col;
	}
	else if (1 != 0) {
		vec3 col = mix(vec3(1,0,0), vec3(0,1,0), sdf);
		float lines = abs(mod(sdf, 0.5) -0.25) / 0.25;

		frag_col = vec4(col * lines, 1);
	}

	//
	if (draw_wireframe) {
		if (wireframe_edge_factor() >= 0.8) discard;
		
		frag_col = mix(vec4(1,1,0,1), vec4(0,0,0,1), wireframe_edge_factor());
	}
}

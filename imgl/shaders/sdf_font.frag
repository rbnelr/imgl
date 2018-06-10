#version 330 core // version 3.3

in		vec2	vs_uv;
in		vec4	vs_col;

out		vec4	frag_col;

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

	if (0 != 0) {
		
		float edge = 180 / 255.0;
		float edge_aa = 5 / 255.0;

		float alpha = saturate(map(sdf, edge -edge_aa, edge +edge_aa, 0,1));

		frag_col = vec4(1,1,1,alpha) * vs_col;
	}
	else if (0 != 0) {
		frag_col = vec4(1,1,1,sdf) * vs_col;
	}
	else if (1 != 0) {
		vec3 col = mix(vec3(1,0,0), vec3(0,1,0), sdf);
		float lines = abs(mod(sdf, 0.1) -0.05) / 0.05;

		frag_col = vec4(col * lines, 1);
	}

	//
	if (draw_wireframe) {
		if (wireframe_edge_factor() >= 0.8) discard;
		
		frag_col = mix(vec4(1,1,0,1), vec4(0,0,0,1), wireframe_edge_factor());
	}
}

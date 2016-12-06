#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

void main(){

	float val =
		texture2D(myTextureSampler, vec2(UV.s, 0)).a;//vec2(UV.s, 0)).a - .25; // : 0;

	float width = 1024.0;

	if(abs(UV.t - val) < 0.0004) {
		float col = UV.s;
		color = vec3(1, 1-col, col);
	} else {
		color = vec3(0, 0, 0);
	}

}

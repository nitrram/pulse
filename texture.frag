#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

void main(){

	float val = texture2D(myTextureSampler, UV.st).a;

	if(abs(UV.t - val) < 0.0004) {
		float col = UV.s;
		color = vec3(1, 1-col, col);
	} else {
		color = vec3(0, 0, 0);
	}
}

#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

void main(){

	float val = texture2D(myTextureSampler, vec2(UV.x, 0)).a;

	if(abs(UV.y - val) < 0.001) {
		float col = UV.x;
		color = vec3(col, 1-col, col);
	} else {
		color = vec3(0, 0, 0);
	}
}

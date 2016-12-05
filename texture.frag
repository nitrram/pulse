#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

void main(){

	float val =
		texture2D(myTextureSampler, UV.st).a - .5;//vec2(UV.s, 0)).a - .25; // : 0;

	float width = 1024.0;

	if(abs(UV.y - val) < 0.0014) {
		float col = UV.x;
		color = vec3(1, 1-col, col);
		//color = vec3(255, 0, 0);
	} else {
		color = vec3(0, 0, 0);
	}

//	color = vec3(255,0,0);
	/* if(UV.y < -0.2 && UV.x > 0.3) { */
	/*	color = vec3(200,25,120); */
	/* } */
	/* else { */
	/*	color = vec3(0, 0, 0); */
	/* } */
}

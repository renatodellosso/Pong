#version 400 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D textID; //Did we ever set this?
uniform vec3 textColor;

void main() {
	//vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
	//vec4 sampled = texture(text, TexCoords);

	//sampled.r = textureSize(text, 0).x;
	//sampled.g = textureSize(text, 0).y;

	//sampled.a = (sampled.b + sampled.r + sampled.g)/3;

	//sampled.r = TexCoords.x;
	//sampled.g = TexCoords.y;

	//color = vec4(textColor, 1.0) * sampled;

	color =  texture(textID, TexCoords);
}
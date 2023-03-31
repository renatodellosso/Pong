//Target OpenGL Version: 4.0.0
#version 400

//in declare input variables
in vec3 pos;

void main() {
	gl_Position = vec4(pos, 1.0);
}
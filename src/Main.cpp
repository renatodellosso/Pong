#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <glad/glad.h> //Make to sure to glad.c to included in project!
#include <GLFW/glfw3.h>

using namespace std;

GLFWwindow* window;

void error(int error, const char* desc);
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateScreen();
string readFile(string file);
void initShaders();
GLuint genVAO(float *vertices);
void draw(float *vertices);

GLuint shaderProgram; //Unsigned int
GLuint vao; //VertexAttribObject, stores our vertex config

struct Rect {
	float x, y, width, height;

	float** vertices() {
		static float** verts = 0; //2D array

		verts = new float*[2];

		verts[0] = new float[9]{ 
			x, y, 0,
			x + width, y, 0,
			x, y + height, 0
		};

		verts[1] = new float[9] {
			x + width, y, 0,
				x + width, y + height, 0,
				x, y + height, 0
		};

		return verts;
	}
};

float paddleWidth = 0.03f, paddleHeight = 0.4f;

Rect leftPaddle = { -1.0f, -paddleHeight/2, paddleWidth, paddleHeight }, rightPaddle = { 1.0f - paddleWidth, -paddleHeight/2, paddleWidth, paddleHeight };

int main() {
	if (!glfwInit()) {
		//Failed to init GLFW
		return 1;
	}

	glfwSetErrorCallback(error);

	//Enforce minimum OpenGL versions
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Create window and context
	window = glfwCreateWindow(1920, 1080, "Pong", NULL, NULL);

	if (!window) {
		//Failed to create window
		return 2;
	}

	//Make the window the current context
	glfwMakeContextCurrent(window);

	//Init GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		//Failed to init GLAD
		return 3;
	}

	glfwSetKeyCallback(window, keyCallback);

	//Retrieve window size
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glfwSwapInterval(1);

	initShaders();

	while (!glfwWindowShouldClose(window))
	{
		//Main loop
		updateScreen();
		glfwPollEvents();
	}

	//Clean up GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


void error(int error, const char* desc) {
	cout << "Error: " << error << " " << desc << endl;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

//Taken from https://stackoverflow.com/a/2602258
string readFile(string file) {
	ifstream t(file); //Create inputstream from file
	stringstream buffer; //Create stringstream to read file
	buffer << t.rdbuf(); //Read file into stringstream

	return buffer.str(); //Return string from stringstream
}

void draw(float* vertices) {
	//Generate the VAO for our current vertices
	GLuint vao = genVAO(vertices);

	//Use our shader program
	glUseProgram(shaderProgram);

	//Bind our VAO
	glBindVertexArray(vao);

	//Draw the triangle, 0 is the starting index of our array, 3 is the number of vertices
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void updateScreen() {
	//Clear previous frame
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//Draw paddles
	draw(leftPaddle.vertices()[0]);
	draw(leftPaddle.vertices()[1]);
	draw(rightPaddle.vertices()[0]);
	draw(rightPaddle.vertices()[1]);

	//float vertices[] = {
	//	-0.5f, -0.5f, 0.0f,
	//	0.5f, -0.5f, 0.0f,
	//	0.0f,  0.5f, 0.0f
	//};

	//draw(vertices);

	////Cannot reinitialize an array, must set individual values or make a new array
	////Invert each value in vertices
	//for (int i = 0; i < sizeof(vertices) / sizeof(vertices[0]); i++)
	//	vertices[i] = -vertices[i];

	//draw(vertices);

	glfwSwapBuffers(window); //Updates screen, we write to one buffer, while we display the other
}

void initShaders() {
	//cout << "Initting shaders...\n";

	//Vertex Shader
	//cout << "Initting vertex shader...\n";
	//Compile shaders, taken from https://learnopengl.com/Getting-started/Hello-Triangle
	string vertexShaderCode = readFile("src/shaders/vertex.vert");
	//String to char array from https://www.geeksforgeeks.org/convert-string-char-array-cpp/, make to use length+1 to leave room for terminating character!
	char* vertexShaderSource = new char[vertexShaderCode.length() + 1];
	strcpy_s(vertexShaderSource, vertexShaderCode.length()+1, vertexShaderCode.c_str());

	//cout << endl << vertexShaderSource << endl;

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	//Check for errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cout << "Error: Vertex shader compilation failed. Details:\n" << infoLog << endl;
	}

	//Fragment Shader
	//cout << "Initting fragment shader...\n";
	string fragmentShaderCode = readFile("src/shaders/fragment.frag");

	char* fragmentShaderSource = new char[fragmentShaderCode.length() + 1];
	strcpy_s(fragmentShaderSource, fragmentShaderCode.length()+1, fragmentShaderCode.c_str());

	//cout << endl << fragmentShaderSource << endl;

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//Error checking
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cout << "Error: Fragment shader compilation failed. Details:\n" << infoLog << endl;
	}

	//Create combined shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	//Error checking
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		cout << "Error: Shader linking failed. Details:\n" << infoLog << endl;
	}

	//Use shader program
	glUseProgram(shaderProgram);

	//We don't need the shader objects anymore
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//cout << "Finished initting shaders\n";
}

GLuint genVAO(float *vertices) {
	//Create buffer
	GLuint vbo;
	glGenBuffers(1, &vbo);

	//Bind the buffer and its data
	glBindBuffer(GL_ARRAY_BUFFER, vbo); //Set buffer type
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices, GL_STATIC_DRAW); //Set the buffer's data

	GLuint vao;

	//Generate VAO
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//Configure how vbo is interpreted, 0 is the first value, since we said position is at location 0 in our vertex shader
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	return vao;
}
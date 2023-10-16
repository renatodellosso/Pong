#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <map>

#include <glad/glad.h> //Make to sure to include glad.c in project!
#include <GLFW/glfw3.h>
//#include <glm/glm.hpp> //From https://github.com/g-truc/glm

//FreeType: https://freetype.org/freetype2/docs/tutorial/step1.html
#include <freetype/config/ftheader.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H

#include <stb/stb_image.h>

using namespace std;

GLFWwindow* window;

void error(int error, const char* desc);
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateScreen();
string readFile(string file);
int initShaders();
GLuint genVAO(float *vertices);
void draw(float *vertices);
void moveBall();
void handleKeys();
void resetGame();

const int SCREEN_WIDTH = 1920, SCREEN_HEIGHT = 1080; //Screen size

struct Texture {
	int width = 0, height = 0, colorChannels;
	unsigned char* bytes;
	GLuint textureID = 0;

	Texture(string file, int width, int height, int channels) {
		this->width = width;
		this->height = height;
		this->colorChannels = channels;

		bytes = stbi_load(file.c_str(), &width, &height, &channels, 0);
	}

	Texture() { } //Default constructor

	void generate() {
		//Create the texture
		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);

		//Configure the texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//Generate the texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

		//Clean up
		stbi_image_free(bytes);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Generate mipmap
		glGenerateMipmap(GL_TEXTURE_2D);
	}
};

Texture testTexture = Texture("resources/acererak.png", 2000, 1319, 3);

GLuint shaderProgram, textShader; //Unsigned int
GLuint vao; //VertexAttribObject, stores our vertex config

struct Vector2 {
	float x = 0, y = 0;

	Vector2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	Vector2() {}
};

//Range from 0 to 1920 and 0 to 1080 (screen coords), not sure we have to do this
//glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 0.0f, 1080.0f);

struct Rect {
	float x = 0, y = 0, width = 0, height = 0;

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

	void drawSelf() {
		float** verts = vertices();
		draw(verts[0]);
		draw(verts[1]);
	}

	//Create a constructor to populate variables and halve the width
	Rect(float x, float y, float width, float height) {
		this->x = x;
		this->y = y;
		this->width = width/2; //Not sure why we have to halve width, but we do
		this->height = height;
	}

	Rect() { } //Default constructor, we have to have this
};

struct Ball : Rect {
	float velX = 0, velY = 0;

	//Create a constructor to populate variables
	Ball(float x, float y, float width, float height, float velX, float velY) {
		this->x = x;
		this->y = y;
		this->width = width/2;
		this->height = height;
		this->velX = velX;
		this->velY = velY;
	}

	Ball() { } //Default constructor, we have to have this
};

const float PADDLE_WIDTH = 0.03f, PADDLE_HEIGHT = 0.4f, PADDLE_SPEED = 10.0f, BALL_SIZE = 0.1f, BALL_SPEED_INITIAL = 0.5f, BALL_SPEED_INCREASE = 0.1f;
float ballSpeed = BALL_SPEED_INITIAL;

Rect leftPaddle = { -1.0f, -PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT }, rightPaddle = { 1.0f - PADDLE_WIDTH, -PADDLE_HEIGHT/2, PADDLE_WIDTH, PADDLE_HEIGHT };
Ball ball = { 0, 0, BALL_SIZE, BALL_SIZE, ((float)(rand()) / (float)RAND_MAX) * 2 - 1, ((float)(rand()) / (float)RAND_MAX) * 2 - 1 }; //Set velocities to random float -1 to 1, but will be overridden in 
																																	  //resetGame()

float deltaTime = 0, lastTime = 0;

const int W = 87, S = 83, UP = 265, DOWN = 264;
float leftDir = 0, rightDir = 0;

int leftScore = 0, rightScore = 0;

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

	//Create window and context, getPrimaryMonitor makes it fullscreen
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong", glfwGetPrimaryMonitor(), NULL);

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);

	int shadersInitted = initShaders();
	if (shadersInitted != 0) {
		cout << "Failed to init shaders";
		return 7;
	}

	lastTime = glfwGetTime(); //Gets the time since init

	resetGame();

	while (!glfwWindowShouldClose(window))
	{
		deltaTime = glfwGetTime() - lastTime; //Time since last frame
		lastTime = glfwGetTime();

		//Main loop
		glfwPollEvents();
		handleKeys();
		moveBall();
		updateScreen();
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
	//cout << "Key: " << key << " Scancode: " << scancode << " Action: " << action << " Mods: " << mods << endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	
	if (key == W) {
		if(action == GLFW_RELEASE) leftDir = 0;
		else leftDir = 1;
	}
	else if (key == S) {
		if (action == GLFW_RELEASE) leftDir = 0;
		else leftDir = -1;
	}
	else if (key == UP) {
		if (action == GLFW_RELEASE) rightDir = 0;
		else rightDir = 1;
	}
	else if (key == DOWN) {
		if (action == GLFW_RELEASE) rightDir = 0;
		else rightDir = -1;
	}
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
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgram);

	//Draw paddles
	leftPaddle.drawSelf();
	rightPaddle.drawSelf();
	ball.drawSelf();

	//Rect(-1.0f, -1.0f, 4.0f, 2.0f).drawSelf(); //White rectangle covering entire screen

	glfwSwapBuffers(window); //Updates screen, we write to one buffer, while we display the other
}

//OpenGL flags are of type GLenum
GLuint loadShader(string file, GLenum type) {
	//cout << "Loading shader: " << file << ", Type: " << type << endl;

	//Compile shaders, taken from https://learnopengl.com/Getting-started/Hello-Triangle
	string code = readFile(file);
	//String to char array from https://www.geeksforgeeks.org/convert-string-char-array-cpp/, make to use length+1 to leave room for terminating character!
	char* source = new char[code.length() + 1];
	strcpy_s(source, code.length() + 1, code.c_str());

	//cout << endl << source << endl;

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	//Check for errors
	int success;
	char infoLog[512];
	glGetProgramiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		cout << "Error: Shader compilation failed. Source File: " << file << " Details:\n" << infoLog << endl;
	}

	return shader;
}

int createShaderProgram(GLuint& program, string vertFile, string fragFile) {
	GLuint vert = loadShader(vertFile, GL_VERTEX_SHADER);
	GLuint frag = loadShader(fragFile, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	//Error checking
	int success;
	char infoLog[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		cout << "Error: Shader linking failed. Program: " << program << ", Vert File: " << vertFile << ", Frag File: " << fragFile << " Details:\n" << infoLog << endl;
		return 7;
	}

	glDeleteShader(vert);
	glDeleteShader(frag);

	return 0;
}

int initShaders() {
	//Load normal shader
	if(createShaderProgram(shaderProgram, "src/shaders/vertex.vert", "src/shaders/fragment.frag") != 0)
		return 7;

	//Load text shader
	if(createShaderProgram(textShader, "src/shaders/text.vert", "src/shaders/text.frag") != 0)
		return 7;

	return 0;
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

GLuint genTextVAO() {
	unsigned int vao, vbo;

	//Create and configure VAO and VBO
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return vao;
}

void resetGame() {
	ball.x = 0;
	ball.y = 0;
	ball.velX = ((float)(rand()) / (float)RAND_MAX) * 2 - 1;
	ball.velY = ((float)(rand()) / (float)RAND_MAX) * 2 - 1;

	float velTotal = ball.velX + ball.velY;
	ball.velX /= velTotal;
	ball.velY /= velTotal;

	//Cap velocity
	const float MAX_VEL = 1.0f;
	ball.velX = min(MAX_VEL, max(-MAX_VEL, ball.velX));
	ball.velY = min(MAX_VEL, max(-MAX_VEL, ball.velY));

	ballSpeed = BALL_SPEED_INITIAL;

	leftPaddle.y = -PADDLE_HEIGHT / 2;
	rightPaddle.y = -PADDLE_HEIGHT / 2;
}

void moveBall() {
	ball.x += ball.velX * ballSpeed * deltaTime;
	ball.y += ball.velY * ballSpeed * deltaTime;

	if (abs(ball.y) >= 1 || ball.y + ball.height >= 1)
		ball.velY *= -1;
	
	bool bounceX = abs(ball.x) >= 1;
	
	//Check if ball is colliding with paddle
	if (ball.y + ball.height > leftPaddle.y && ball.y < leftPaddle.y + leftPaddle.height && ball.x <= leftPaddle.x + leftPaddle.width) bounceX = true;
	else if (ball.y + ball.height > rightPaddle.y && ball.y < rightPaddle.y + rightPaddle.height && ball.x + ball.width >= rightPaddle.x) bounceX = true;

	if (bounceX) {
		ball.velX *= -1;
		ballSpeed += BALL_SPEED_INCREASE;
	}

	//Check if ball is out of bounds
	if (ball.x <= -1) {
		rightScore++;
		resetGame();
	}
	else if (ball.x >= 1) {
		leftScore++;
		resetGame();
	}
}

void movePaddle(Rect& paddle, float dir) {
	float speed = PADDLE_SPEED * deltaTime;
	
	if ((paddle.y + paddle.height < 1 && dir > 0) ||
				(paddle.y > -1 && dir < 0))
		paddle.y += dir * speed;
}

void handleKeys() {
	float left = leftDir, right = rightDir;
	
	left *= PADDLE_SPEED * deltaTime;
	right *= PADDLE_SPEED * deltaTime;

	movePaddle(leftPaddle, left);
	movePaddle(rightPaddle, right);
}
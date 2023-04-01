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
void moveBall();
void handleKeys();
void resetGame();

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
	float velX, velY;

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
	window = glfwCreateWindow(1920, 1080, "Pong", glfwGetPrimaryMonitor(), NULL);

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
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//Draw paddles
	leftPaddle.drawSelf();
	rightPaddle.drawSelf();
	ball.drawSelf();

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

	if (abs(ball.y) + ball.height >= 1)
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
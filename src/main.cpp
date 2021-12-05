#include <fstream>
#include <string>
#include <iostream>
#include <vector>

// OpenGL related libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Pre-declaration of noiseValue function
float noiseValue(float, float);

// File paths to shaders
const char *vertexPath = "shaders/vertex_shader.txt";
const char *fragmentPath = "shaders/fragment_shader.txt";

// Map is square of vertices of side length mapSize
const unsigned int mapSize = 512;

// Dimensions of glfw window
const unsigned int windowWidth = 1000;
const unsigned int windowHeight = 600;

const float fieldView = glm::radians(45.0f);
const float aspectRatio = (float)windowWidth / (float)windowHeight;
const float nearPlane = 0.1f;
const float farPlane = 1000.0f;

// Point in 3D space with coordinates (x, y, z)
struct Vertex
{
	float x, y, z;
};

struct Camera
{
	glm::vec3 position;
	glm::vec3 direction;
	float yaw; // Angle in radians
	float pitch; // Angle in radians
} camera;

// Logs errors to the terminal
void checkErrors()
{
	unsigned int error = glGetError();
	while (error)
	{
		std::cout << error << '\n';
		error = glGetError();
	}
}

// Loads shader file at given filePath
std::string loadShaderFile(const char *filePath)
{
	std::ifstream shaderFile(filePath);
	std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	return shaderSource;
}

int main()
{
	// Initialise glfw
	glfwInit();

	// 
	glfwSwapInterval(1);
	
	// Sets version for opengl
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Window object initalisation
	GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Perlin Noise", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Sets up window dimensions
	glViewport(0, 0, windowWidth, windowHeight);

	glEnable(GL_DEPTH_TEST);

	// Background colour
	glClearColor(0.2f, 0.2f, 0.7f, 1.0f);

	// Initialises vertex array and allocates memory
	const unsigned int vertexCount = mapSize * mapSize;
	Vertex *vertices = new Vertex[vertexCount];

	// Index array
	std::vector<unsigned int> indices;

	for (int i = 0; i < mapSize; i++)
	{
		for (int j = 0; j < mapSize; j++)
		{
			// Initialises vertices as lattice points along xz plane
			vertices[mapSize * i + j] = { (float)i, 0.0f, (float)j };
			if (i < mapSize - 1 && j < mapSize - 1)
			{
				/* 
				Adds the indices for both triangles in the square.
				Square of v----v
						  |   /|
						  |  / |
						  | /  |
						  v----v
				has vertices (clockwise, starting top left) of (i,j),
				(i, j + 1), (i + 1, j), (i + 1, j + 1). Adds those indices
				to the vector for each square in the array 
				*/
				indices.push_back(mapSize * i + j);
				indices.push_back(mapSize * i + j + 1);
				indices.push_back(mapSize * (i + 1) + j);
				indices.push_back(mapSize * (i + 1) + j);
				indices.push_back(mapSize * i + j + 1);
				indices.push_back(mapSize * (i + 1) + j + 1);
			}
		}
	}

	// Initialises vertex buffer
	unsigned int vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);

	// Initialises vertex array
	unsigned int vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	// Sets up attribute pointer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Initialises element buffer
	unsigned int elementBuffer;
	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// Reads in the source code for both shaders
	std::string vertexSourceString = loadShaderFile(vertexPath);
	const char *vertexSource = vertexSourceString.c_str();
	std::string fragmentSourceString = loadShaderFile(fragmentPath);
	const char *fragmentSource = fragmentSourceString.c_str();

	// Compile shaders
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	// Initialise program and attach shaders 
	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	// Delete shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glUseProgram(program);

	// Generates perspective matrix
	glm::mat4 perspectiveMatrix(1.0f);
	perspectiveMatrix = glm::perspective(fieldView, aspectRatio, nearPlane, farPlane);

	glm::mat4 projectionMatrix(1.0f);

	// Sets initial camera position
	camera.position = glm::vec3(mapSize / 2.0f - 0.5f, 10, mapSize / 2.0f - 0.5);

	checkErrors();

	float xOffset = 0.0f;
	float zOffset = 0.0f;

	bool updateVertices = true;

	// Loop while program is running
	while (!glfwWindowShouldClose(window))
	{
		// Clears window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Time between frames
		double deltaTime = glfwGetTime();
		glfwSetTime(0.0);

		float movementSpeed = 10.0f * deltaTime;

		// Move grid values based on keypress to simulate movement in 4 directions (along xz plane)
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			xOffset += movementSpeed * glm::cos(glm::radians(camera.yaw));
			zOffset += movementSpeed * glm::sin(glm::radians(camera.yaw));
			updateVertices = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			xOffset -= movementSpeed * glm::cos(glm::radians(camera.yaw));
			zOffset -= movementSpeed * glm::sin(glm::radians(camera.yaw));
			updateVertices = true;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			xOffset += movementSpeed * glm::cos(glm::radians(camera.yaw - 90.0f));
			zOffset += movementSpeed * glm::sin(glm::radians(camera.yaw - 90.0f));
			updateVertices = true;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			xOffset -= movementSpeed * glm::cos(glm::radians(camera.yaw - 90.0f));
			zOffset -= movementSpeed * glm::sin(glm::radians(camera.yaw - 90.0f));
			updateVertices = true;
		}

		// Moves camera up or down
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.position += glm::vec3(0.0f, movementSpeed, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.position += glm::vec3(0.0f, -movementSpeed, 0.0f);

		// Turns camera with arrow keys
		float turnSpeed = 100.0f * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.pitch += turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.pitch -= turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.yaw += turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.yaw -= turnSpeed;

		// Limits camera rotation
		if (camera.pitch < -89.9f) camera.pitch = -89.9f;
		if (camera.pitch > 89.9f) camera.pitch = 89.9f;

		// Calculates normalised direction vector that camera is pointing
		camera.direction.x = glm::cos(glm::radians(camera.yaw)) * glm::cos(glm::radians(camera.pitch));
		camera.direction.y = glm::sin(glm::radians(camera.pitch));
		camera.direction.z = glm::sin(glm::radians(camera.yaw)) * glm::cos(glm::radians(camera.pitch));
		camera.direction = glm::normalize(camera.direction);

		// Calculates projection matrix based on camera vectors
		glm::vec3 up = glm::normalize(glm::cross(glm::normalize(glm::cross(camera.direction, glm::vec3(0.0f, 1.0f, 0.0f))), camera.direction));
		projectionMatrix = perspectiveMatrix * glm::lookAt(camera.position, camera.position + camera.direction, up);

		// Sets projections matrix in shaders
		glUniformMatrix4fv(glGetUniformLocation(program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		// Calculates perlin noise values for each coordinate in vertex square
		if (updateVertices)
		{
			for (int i = 0; i < mapSize; i++)
			{
				for (int j = 0; j < mapSize; j++)
				{
					float xPosition = (float)i + xOffset;
					float zPosition = (float)j + zOffset;

					// Multiple frequencies are summed to add varying detail
					float overtone = 64 * noiseValue(xPosition / 256, zPosition / 256);
					float octave1 = 32 * noiseValue(xPosition / 64, zPosition / 64);
					float octave2 = 16 * noiseValue(xPosition / 32, zPosition / 32);
					float octave3 = 8 * noiseValue(xPosition / 16, zPosition / 16);
					float octave4 = 4 * noiseValue(xPosition / 8, zPosition / 8);
					float octave5 = 2 * noiseValue(xPosition / 4, zPosition / 4);

					// Sums frequencies and raises to power of 1.2 to add excentuated peaks
					vertices[i * mapSize + j].y = pow(overtone + octave1 + octave2 + octave3 + octave4 + octave5, 1.2f) - 140;
				}
			}
			// Adds vertex data to vertex buffer
			glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);

			updateVertices = false;
		}
		// Draws map
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

		checkErrors();

		// Swaps buffers so shown on screen
		glfwSwapBuffers(window);

		glfwPollEvents();

		// Outputs framerate to terminal
		std::cout << 1.0f / deltaTime << '\n';
	}
}

/*
Perlin noise algorithm from here to EOF.
Each lattice point is associated with a gradient (as dictated by the permutation table
and a hash function) and the height of a point is based on the interpolation between
the gradients of the surrounding lattice points.
*/

const int permutationTable[512] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7,
	225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247,
	120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134,
	139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220,
	105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80,
	73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
	164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38,
	147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189,
	28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101,
	155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232,
	178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12,
	191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181,
	199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236,
	205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7,
	225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247,
	120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134,
	139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220,
	105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80,
	73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
	164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38,
	147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189,
	28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101,
	155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232,
	178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12,
	191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181,
	199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236,
	205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
};

// Linear interpolation of w between values a and b
float lerp(float w, float a, float b) {
	return a * (1.0f - w) + b * w;
}

// Eases t to reduce artefacts by smoothing transition between values
float fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

// Returns dot product of offset of point into square and 1 of 8 direction vectors
float gradientDotDistance(int hash, float xOffset, float zOffset)
{
	switch (hash)
	{
	case 0:
		return xOffset + zOffset;
	case 1:
		return -xOffset + zOffset;
	case 2:
		return xOffset -zOffset;
	case 3:
		return -xOffset - zOffset;
	case 4:
		return xOffset;
	case 5:
		return zOffset;
	case 6:
		return -xOffset;
	case 7:
		return -zOffset;
	}
}

// Finds noise value (y value) for point at coordinate (x, z)
float noiseValue(float x, float z)
{
	// gridX and gridZ are between 0 and 255
	int gridX = (int)floor(x) & 255;
	int gridZ = (int)floor(z) & 255;

	// Makes x and z integers
	x -= floor(x);
	z -= floor(z);

	// Applies easing function to x and z
	float u = fade(x);
	float v = fade(z);

	// Gets gradients from permutation table for 4 points of square in which the point lies
	int gradientBottomLeft = permutationTable[permutationTable[gridX] + gridZ];
	int gradientBottomRight = permutationTable[permutationTable[gridX + 1] + gridZ];
	int gradientTopLeft = permutationTable[permutationTable[gridX] + gridZ + 1];
	int gradientTopRight = permutationTable[permutationTable[gridX + 1] + gridZ + 1];


	// Hashes the 4 corners and finds the dot products
	float dotBottomLeft = gradientDotDistance(gradientBottomLeft & 7, x, z);
	float dotBottomRight = gradientDotDistance(gradientBottomRight & 7, x - 1.0f, z);
	float dotTopLeft = gradientDotDistance(gradientTopLeft & 7, x, z - 1.0f);
	float dotTopRight = gradientDotDistance(gradientTopRight & 7, x - 1.0f, z - 1.0f);

	// Calculation to return noiseValue
	return 0.5 * lerp(v, lerp(u, dotBottomLeft, dotBottomRight), lerp(u, dotTopLeft, dotTopRight)) + 0.5f;
}
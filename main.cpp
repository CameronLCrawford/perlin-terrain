#include <fstream>
#include <string>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

float noiseValue(float, float);

const char *vertexPath = "vertex_shader.txt";
const char *fragmentPath = "fragment_shader.txt";

const unsigned int mapSize = 512;

const unsigned int windowWidth = 1000;
const unsigned int windowHeight = 600;

const float fieldView = glm::radians(45.0f);
const float aspectRatio = (float)windowWidth / (float)windowHeight;
const float nearPlane = 0.1f;
const float farPlane = 1000.0f;

struct Vertex
{
	float x, y, z;
};

struct Camera
{
	glm::vec3 position;
	glm::vec3 direction;
	float yaw;
	float pitch;
} camera;

void checkErrors()
{
	unsigned int error = glGetError();
	while (error)
	{
		std::cout << error << '\n';
		error = glGetError();
	}
}

std::string loadShaderFile(const char *filePath)
{
	std::ifstream shaderFile(filePath);
	std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	return shaderSource;
}

int main()
{
	glfwInit();

	glfwSwapInterval(1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Perlin Noise", NULL, NULL);

	glfwMakeContextCurrent(window);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glViewport(0, 0, windowWidth, windowHeight);

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.2f, 0.2f, 0.7f, 1.0f);

	const unsigned int vertexCount = mapSize * mapSize;
	Vertex *vertices = new Vertex[vertexCount];

	std::vector<unsigned int> indices;

	for (int i = 0; i < mapSize; i++)
	{
		for (int j = 0; j < mapSize; j++)
		{
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

	unsigned int vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);

	unsigned int vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	unsigned int elementBuffer;
	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	/* Reads in the source code for both shaders */
	std::string vertexSourceString = loadShaderFile(vertexPath);
	const char *vertexSource = vertexSourceString.c_str();
	std::string fragmentSourceString = loadShaderFile(fragmentPath);
	const char *fragmentSource = fragmentSourceString.c_str();

	/* Compile shaders */
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	/* Initialise program and attach shaders */
	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	/* Delete shaders */
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glUseProgram(program);

	glm::mat4 perspectiveMatrix(1.0f);
	perspectiveMatrix = glm::perspective(fieldView, aspectRatio, nearPlane, farPlane);

	glm::mat4 projectionMatrix(1.0f);

	camera.position = glm::vec3(mapSize / 2.0f - 0.5f, 10, mapSize / 2.0f - 0.5);

	checkErrors();

	float xOffset = 0.0f;
	float zOffset = 0.0f;

	bool updateVertices = true;

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		double deltaTime = glfwGetTime();
		glfwSetTime(0.0);

		float movementSpeed = 10.0f * deltaTime;
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
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.position += glm::vec3(0.0f, movementSpeed, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.position += glm::vec3(0.0f, -movementSpeed, 0.0f);
		float turnSpeed = 100.0f * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.pitch += turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.pitch -= turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.yaw += turnSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.yaw -= turnSpeed;

		if (camera.pitch < -89.9f) camera.pitch = -89.9f;
		if (camera.pitch > 89.9f) camera.pitch = 89.9f;

		camera.direction.x = glm::cos(glm::radians(camera.yaw)) * glm::cos(glm::radians(camera.pitch));
		camera.direction.y = glm::sin(glm::radians(camera.pitch));
		camera.direction.z = glm::sin(glm::radians(camera.yaw)) * glm::cos(glm::radians(camera.pitch));
		camera.direction = glm::normalize(camera.direction);
		glm::vec3 up = glm::normalize(glm::cross(glm::normalize(glm::cross(camera.direction, glm::vec3(0.0f, 1.0f, 0.0f))), camera.direction));
		projectionMatrix = perspectiveMatrix * glm::lookAt(camera.position, camera.position + camera.direction, up);

		glUniformMatrix4fv(glGetUniformLocation(program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		if (updateVertices)
		{
			for (int i = 0; i < mapSize; i++)
			{
				for (int j = 0; j < mapSize; j++)
				{
					float xPosition = (float)i + xOffset;
					float zPosition = (float)j + zOffset;
					float overtone = 64 * noiseValue(xPosition / 256, zPosition / 256);
					float octave1 = 32 * noiseValue(xPosition / 64, zPosition / 64);
					float octave2 = 16 * noiseValue(xPosition / 32, zPosition / 32);
					float octave3 = 8 * noiseValue(xPosition / 16, zPosition / 16);
					float octave4 = 4 * noiseValue(xPosition / 8, zPosition / 8);
					float octave5 = 2 * noiseValue(xPosition / 4, zPosition / 4);
					vertices[i * mapSize + j].y = pow(overtone + octave1 + octave2 + octave3 + octave4 + octave5, 1.2f) - 140;
				}
			}
			glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);
			updateVertices = false;
		}

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		checkErrors();

		glfwSwapBuffers(window);
		glfwPollEvents();

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

float lerp(float w, float a, float b) {
	return a * (1.0f - w) + b * w;
}

float fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

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

float noiseValue(float x, float z)
{
	int gridX = (int)floor(x) & 255;
	int gridZ = (int)floor(z) & 255;
	x -= floor(x);
	z -= floor(z);
	float u = fade(x);
	float v = fade(z);

	int gradientBottomLeft = permutationTable[permutationTable[gridX] + gridZ];
	int gradientBottomRight = permutationTable[permutationTable[gridX + 1] + gridZ];
	int gradientTopLeft = permutationTable[permutationTable[gridX] + gridZ + 1];
	int gradientTopRight = permutationTable[permutationTable[gridX + 1] + gridZ + 1];

	float dotBottomLeft = gradientDotDistance(gradientBottomLeft & 7, x, z);
	float dotBottomRight = gradientDotDistance(gradientBottomRight & 7, x - 1.0f, z);
	float dotTopLeft = gradientDotDistance(gradientTopLeft & 7, x, z - 1.0f);
	float dotTopRight = gradientDotDistance(gradientTopRight & 7, x - 1.0f, z - 1.0f);

	return 0.5 * lerp(v, lerp(u, dotBottomLeft, dotBottomRight), lerp(u, dotTopLeft, dotTopRight)) + 0.5f;
}
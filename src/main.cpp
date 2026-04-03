#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>

using namespace std;
using namespace glm;

using Clock = std::chrono::high_resolution_clock;
const float G = 6.67430e-11f; 
const float softening = 0.02f;

struct Star {
	vec3 position;
	double mass;
	vec3 velocity;
	bool fixed = false;
	
	Star(vec3 pos, vec3 vel, double m) : position(pos), velocity(vel), mass(m) {}
};

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
	auto loader = [](const char* source, GLenum type) -> GLuint {
		ifstream file(source);
		if (!file.is_open()) {
			cerr << "Failed to open shader file: " << source << endl;
			return 0;
		}
		
		stringstream ss;
		ss << file.rdbuf();
		string code = ss.str();
		const char* sourceStr = code.c_str();
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &sourceStr, nullptr);
		glCompileShader(shader);
		
		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << endl;
			return 0;
		}
		return shader;
	};

	GLuint vertexShader = loader(vertexSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = loader(fragmentSource, GL_FRAGMENT_SHADER);

	if (!vertexShader || !fragmentShader) {
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
};

// creates a window, initializes OpenGL context, and compiles shader programs
struct Compile {
	GLuint program;
	GLuint bgProgram;
	GLFWwindow *window;
	int width = 800, height = 600;
	
	Compile () {
		if(!glfwInit()) {
			cerr << "GLFW failed to initialize" << endl;
			exit(EXIT_FAILURE);
		}
		
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		window = glfwCreateWindow(width, height, "Stellar", nullptr, nullptr);
		if (!window) {
			cerr << "Failed to create GLFW window" << endl;
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		glfwMakeContextCurrent(window);
		
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			cerr << "Failed to initialize GLAD" << endl;
			glfwDestroyWindow(window);
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		program = createShaderProgram("./shader.vert", "./shader.frag");
		bgProgram = createShaderProgram("./fbm.vert", "./fbm.frag");
		
		if (program == 0 || bgProgram == 0) {
			cerr << "Failed to create shader programs." << endl;
			glfwDestroyWindow(window);
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
	}
};

Compile compile;

vector<Star> createStars(size_t numStars, std::mt19937 &rng) {
	vector<Star> stars;
	stars.reserve(numStars);
	uniform_real_distribution<float> posDist(-1.0, 1.0f);
	uniform_real_distribution<float> velDist(-0.01f, 0.01f);
	uniform_real_distribution<double> massDist(0.5f, 3.0f);
	uniform_real_distribution<float> fixedProb(0.0, 1.0);

	for (size_t i = 0; i < numStars; ++i) {
		vec3 position(posDist(rng), posDist(rng), posDist(rng));
		vec3 velocity(velDist(rng), velDist(rng), velDist(rng));
		double mass = massDist(rng);
		stars.emplace_back(position, velocity, mass);
		
		if(fixedProb(rng) < 0.010f) {
			stars.back().fixed = true;
			stars.back().velocity = vec3(0.0f);
		}
	}
	return stars;
}

int main() {
	mat4 view = lookAt(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
	
	const size_t numStars = 500;
	mt19937 rng(12345);
	vector<Star> stars = createStars(numStars, rng);
	
	vector<GLuint> vaoVbo(2);
	GLuint colorVbo, sizeVbo;
	GLuint bgVao, bgVbo;

	// background setup
	glGenVertexArrays(1, &bgVao);
	glGenBuffers(1, &bgVbo);
	glBindVertexArray(bgVao);
	glBindBuffer(GL_ARRAY_BUFFER, bgVbo);
	float tri[] = { -1.0f, -1.0f,  3.0f, -1.0f,  -1.0f, 3.0f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// star VAO/VBO Setup
	glGenVertexArrays(1, &vaoVbo[0]);
	glGenBuffers(1, &vaoVbo[1]);
	glGenBuffers(1, &colorVbo);
	glGenBuffers(1, &sizeVbo);

	glBindVertexArray(vaoVbo[0]);
	
	glBindBuffer(GL_ARRAY_BUFFER, vaoVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, numStars * sizeof(vec3), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	
	vector<vec3> positions(numStars);
	for(size_t i = 0; i < numStars; ++i) {
		positions[i] = stars[i].position;
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, numStars * sizeof(vec3), positions.data());
	
	//assigning colors based on mass; more massive stars are bluer, less massive are redder
	vector<vec3> colors(numStars);
	for (size_t i = 0; i < numStars; ++i) {
		double m = stars[i].mass;
		if (m > 2.2)      colors[i] = vec3(0.6f, 0.8f, 1.0f);     
		else if (m > 1.6) colors[i] = vec3(1.0f, 1.0f, 1.0f); 
		else if (m > 1.2) colors[i] = vec3(1.0f, 0.95f, 0.6f); 
		else if (m > 0.8) colors[i] = vec3(1.0f, 0.6f, 0.25f); 
		else              colors[i] = vec3(1.0f, 0.3f, 0.2f); 
	}    
	glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
	glBufferData(GL_ARRAY_BUFFER, numStars * sizeof(vec3), colors.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	
	vector<float> sizes(numStars);
	const float minM = 0.5f, maxM = 3.0f;
	for (size_t i = 0; i < numStars; ++i) {
		float t = clamp((float)((stars[i].mass - minM) / (maxM - minM)), 0.0f, 1.0f); 
		sizes[i] = 2.0f + 10.0f * t; 
	}
	glBindBuffer(GL_ARRAY_BUFFER, sizeVbo);
	glBufferData(GL_ARRAY_BUFFER, numStars * sizeof(float), sizes.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);

	glEnable(GL_PROGRAM_POINT_SIZE);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	
	auto last = Clock::now();
	vector<vec3> accelerations(numStars, vec3(0.0f));
	
	while(!glfwWindowShouldClose(compile.window)) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(compile.bgProgram);
		GLint timeLoc = glGetUniformLocation(compile.bgProgram, "uTime");
		if (timeLoc != -1) {
			static auto start = Clock::now();
			float t = chrono::duration<float>(Clock::now() - start).count();
			glUniform1f(timeLoc, t);
		}
		GLint resLoc = glGetUniformLocation(compile.bgProgram, "uResolution");
		if (resLoc != -1) {
			int w, h;
			glfwGetFramebufferSize(compile.window, &w, &h);
			glUniform2f(resLoc, (float)w, (float)h);
		}
		glBindVertexArray(bgVao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);
		
		auto now = Clock::now();
		double dt = chrono::duration<double>(now - last).count();
		last = now;
		
		fill(accelerations.begin(), accelerations.end(), vec3(0.0f));
		
		// N-Body gravity O(n^2)
		// added a 0.02f softening factor of 0.02f to prevent stars' gravitational force from becoming infinite if they are too close
		for (size_t i = 0; i < numStars; ++i) {
			for (size_t j = i + 1; j < numStars; ++j) {
				vec3 r = stars[j].position - stars[i].position; 
				float dist2 = dot(r, r) + (softening * softening);
				if (dist2 <= 0.0f) continue; 
				float invDist = 1.0f / sqrt(dist2); //inverse distance
				float invDist3 = invDist * invDist * invDist;
				
				// gravitational acceleration
				vec3 a_i = G * (float)stars[j].mass * invDist3 * r;
				vec3 a_j = -G * (float)stars[i].mass * invDist3 * r;
				
				accelerations[i] += a_i;
				accelerations[j] += a_j;
			}
		}
		
		// updating velocity and position of the stars based on current velocity, acceleration and delta time
		for(size_t i = 0; i < numStars; ++i) {
			if (!stars[i].fixed) {
                stars[i].velocity += accelerations[i] * static_cast<float>(dt);
            }
		
			stars[i].position += stars[i].velocity * static_cast<float>(dt);
			
			for (int j = 0; j < 3; ++j) {
				if (stars[i].position[j] > 1.0f) {
					stars[i].position[j] = 1.0f;
					stars[i].velocity[j] *= -1.0f;
				} else if (stars[i].position[j] < -1.0f) {
					stars[i].position[j] = -1.0f;
					stars[i].velocity[j] *= -1.0f;
				}
			}
			positions[i] = stars[i].position;
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, vaoVbo[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, numStars * sizeof(vec3), positions.data());
		
		glUseProgram(compile.program);
		GLint viewL = glGetUniformLocation(compile.program, "view");
		GLint projL = glGetUniformLocation(compile.program, "projection");
		GLint sizeL = glGetUniformLocation(compile.program, "pointSize");

		if (viewL != -1) glUniformMatrix4fv(viewL, 1, GL_FALSE, &view[0][0]);
		if (projL != -1) glUniformMatrix4fv(projL, 1, GL_FALSE, &projection[0][0]);

		glBindVertexArray(vaoVbo[0]);
		glDepthMask(GL_FALSE);

		if (sizeL != -1) glUniform1f(sizeL, 8.0f);
		glDrawArrays(GL_POINTS, 0, (GLsizei)numStars);

		if (sizeL != -1) glUniform1f(sizeL, 2.0f);
		glDrawArrays(GL_POINTS, 0, (GLsizei)numStars);

		glDepthMask(GL_TRUE);
		glBindVertexArray(0);
		
		glfwSwapBuffers(compile.window);
		glfwPollEvents();
	}
	
	// cleanup resources
	glDeleteBuffers(1, &bgVbo);
	glDeleteVertexArrays(1, &bgVao);
	glDeleteProgram(compile.bgProgram);
	glDeleteBuffers(1, &vaoVbo[1]);
	glDeleteBuffers(1, &colorVbo);
	glDeleteBuffers(1, &sizeVbo);
	glDeleteVertexArrays(1, &vaoVbo[0]);
	glDeleteProgram(compile.program);
	
	glfwDestroyWindow(compile.window);
	glfwTerminate();
	
	return 0;
}
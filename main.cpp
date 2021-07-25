/*
 * main.cpp
 *
 *  Created on: Dec 15, 2020
 *      Author: Bryan Rykowski
 *     Project: CS330_7-1
 */


#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SOIL2/SOIL2.h"
#include "mesh.h"

using namespace std;

#define WINDOW_TITLE "Assignment 7-1"

/*
 * Formatting macro for shader source code.
 */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

std::vector<mesh*> meshes;

GLint shaderProgramFlat, shaderProgramLit;
GLint WindowWidth = 800, WindowHeight = 600;
GLuint texture;

/*
 * Track state of mouse buttons.
 */
bool leftButtonDown = false;
bool rightButtonDown = false;

bool lightFlag = true;
bool orthoFlag = false;		// Track which projection mode is selected.
bool wireFlag = false;

GLfloat lastMouseX = WindowWidth / 2, lastMouseY = WindowHeight / 2;	// Set initial location of mouse reference.
GLfloat mouseXOffset, mouseYOffset;		// Track the mouse delta.
GLfloat yaw = 0.0f, pitch = 0.0f;		// Orbit variables.
GLfloat zoomLevel = 0.0f;				// Distance of camera from focus point.
GLfloat sensitivity = 0.5f;				// Speed of camera orbit.
GLfloat zoomSpeed = 0.1f;				// Speed of camera zoom.
bool mouseDetected = true;				// Use to reset last mouse position when mouse is redetected.

const glm::vec3 cameraPosition(0.0f, 0.0f, 0.0f);		// Location camera is pointed at.
const glm::vec3 cameraUpY(0.0f, 1.0f, 0.0f);			// Camera up vector.
glm::vec3 CameraForwardZ(0.0f, 0.0f, 1.0f);		// Camera location.
glm::vec3 front(0.0f, 0.0f, 10.0f);				// Temporary camera location.

glm::vec3 lightColor0( 1.0f, 0.9f, 0.9f);		// RGB color of the light.
glm::vec3 lightPos0( 0.0f, 1.0f, 2.0f);			// XYZ position of the light.
GLfloat lightFactor0 = 1.0f;					// Brightness of the light.

glm::vec3 lightColor1( 0.0f, 0.0f, 1.0f);		// RGB color of the light.
glm::vec3 lightPos1( -2.0f, -1.0f, -0.5f);		// XYZ position of the light.
GLfloat lightFactor1 = 0.7f;					// Brightness of the light.

glm::vec3 viewPosition( 0.0f, 0.0f, 1.0f);		// Position of camera for reflection calculation.

/*
 * Forward declerations.
 */
void UResizeWindow(int, int);
void URenderGraphics();
void UCreateShaders();
void UCreateBuffers();
void UGenerateTexture();
void UMouseButton(int, int, int, int);
void UMouseMove(int, int);
void UKeyDown(unsigned char, int, int);
void UKeyUp(unsigned char, int, int);

/*
 * Vertex shader source code. Uses matrices to transform position of vertices.
 */
const GLchar * vertexShaderSourceFlat = GLSL(440,
	layout (location = 0) in vec3 position;
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 textureCoordinate;
	
	out vec2 mobileTextureCoordinate;
	
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;
	
	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0f);	// Apply all the matrix transforms to each vertex.
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y);    // Flip the texture on the y axis.
	}
);

const GLchar * vertexShaderSourceLit = GLSL(440,
	layout (location = 0) in vec3 position;
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 textureCoordinate;
	
	out vec3 Normal;
	out vec3 FragmentPos;
	out vec2 mobileTextureCoordinate;
	
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;
	
	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0f);	// Apply all the matrix transforms to each vertex.
		FragmentPos = vec3(model * vec4(position, 1.0f));
		Normal = mat3(transpose(inverse(model))) * normal;
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y);    // Flip the texture on the y axis.
	}
);

const GLchar * fragmentShaderSourceFlat = GLSL(440,
	in vec3 Normal;
	in vec3 FragmentPos;
	in vec2 mobileTextureCoordinate;

	out vec4 gpuColor;

    uniform sampler2D uTexture;
	
	void main()
	{
		gpuColor = texture(uTexture, mobileTextureCoordinate);    // Sample the texture at supplied coordinate
	}
);

const GLchar * fragmentShaderSourceLit = GLSL(440,
	in vec3 Normal;
	in vec3 FragmentPos;
	in vec2 mobileTextureCoordinate;

	out vec4 gpuColor;

	uniform sampler2D uTexture;
	
	uniform vec3 lightColor0;
	uniform vec3 lightPos0;
	uniform float lightFactor0;
	uniform vec3 lightColor1;
	uniform vec3 lightPos1;
	uniform float lightFactor1;
	uniform vec3 viewPosition;
	
	void main()
	{
		/*
		 * Calculate the ambient light contribution.
		 */
		float ambientStrength = 0.1f;
		vec3 ambient = ambientStrength * (lightColor0 + lightColor1);

		/*
		 * Calculate diffuse contribution of first light.
		 */
		vec3 norm0 = normalize(Normal);
		vec3 lightDirection0 = normalize(lightPos0 - FragmentPos);
		float impact0 = max( dot(norm0, lightDirection0), 0.0);
		vec3 diffuse0 = impact0 * lightColor0 * lightFactor0;

		/*
		 * Calculate specular contribution of first light.
		 */
		float specularIntensity0 = 0.8f;
		float highlightSize0 = 8.0f;
		vec3 viewDir0 = normalize(viewPosition - FragmentPos);
		vec3 reflectDir0 = reflect(-lightDirection0, norm0);

		float specularComponent0 = pow( max( dot( viewDir0, reflectDir0), 0.0), highlightSize0);
		vec3 specular0 = specularIntensity0 * specularComponent0 * lightColor0 * lightFactor0;

		/*
		 * Calculate diffuse contribution of second light.
		 */
		vec3 norm1 = normalize(Normal);
		vec3 lightDirection1 = normalize(lightPos1 - FragmentPos);
		float impact1 = max( dot(norm1, lightDirection1), 0.0);
		vec3 diffuse1 = impact1 * lightColor1 * lightFactor1;

		/*
		 * Calculate specular contribution of second light.
		 */
		float specularIntensity1 = 1.0f;
		float highlightSize1 = 16.0f;
		vec3 viewDir1 = normalize(viewPosition - FragmentPos);
		vec3 reflectDir1 = reflect(-lightDirection1, norm1);

		float specularComponent1 = pow( max( dot( viewDir1, reflectDir1), 0.0), highlightSize1);
		vec3 specular1 = specularIntensity1 * specularComponent1 * lightColor1 * lightFactor1;

		vec4 texColor = texture(uTexture, mobileTextureCoordinate);    // Sample the texture at supplied coordinate

		vec3 phong = (ambient + diffuse0 + diffuse1 + specular0 + specular1) * vec3(texColor.r, texColor.g, texColor.b);	// Mix the lights and texture sample.

		gpuColor = vec4( phong, 1.0f);
	}
);

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);										// Initialize GLUT.
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);	// Set GLUT mode flags.
	glutInitWindowSize(WindowWidth, WindowHeight);				// Define GLUT's window size.
	glutCreateWindow(WINDOW_TITLE);								// Tell gut to create the defined window.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE_ARB);									// Enable OpenGL's depth testing for drawing in 3D.
	glDepthFunc(GL_LEQUAL);
	
	glutReshapeFunc(UResizeWindow);	// Register a callback for GLUT's reshape function.

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to intitialize GLEW" << std::endl;
		return -1;
	}

	UCreateShaders();	// Construct the shaders program.
	glUseProgram(shaderProgramLit);
	
	UCreateBuffers();	// Define the data buffers.

	UGenerateTexture();	// Load in an image as a texture
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// Define the color that glClear() will use.

	glutDisplayFunc(URenderGraphics);	// Register a callback for GLUT's display function.
	glutMotionFunc(UMouseMove);			// Register a callback for GLUT's motion function. (Handles mouse movement while a button is pressed)
	glutMouseFunc(UMouseButton);		// Register a callback for GLUT's mouse function. (Handles mouse button state changes)
	glutKeyboardFunc(UKeyDown);			// Register a callback for GLUT's keyboard function. (Handles key down events)

	std::cout << "Controls:\n" << "s : Toggle between Phong shading and flat shading\n" << "p : Toggle between perspective and orthographic projection" << std::endl;
	std::cout << "w : Toggle between filled and wireframe mode\n" << "left mouse button + move : pitch and yaw\n" << "right mouse button + move : zoom in/out" << std::endl;

	glutMainLoop();		// Enter GLUT's event processing loop.

	/*
	 * Free the memory being used for the buffers.
	 */

	for (mesh* mesh : meshes)
	{
		delete mesh;
	}

	return 0;
}

void UGenerateTexture()
{
    glGenTextures(1, &texture);             // Get a pointer to an empty texture.
    glBindTexture(GL_TEXTURE_2D, texture);  // Configure it as a 2D texture.

    int width, height;

    unsigned char* image = SOIL_load_image("brushed_steel.jpg", &width, &height, 0 , SOIL_LOAD_RGB);    // Load an image as an array of bytes.

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);  // Upload the image data into the active texture.
    glGenerateMipmap(GL_TEXTURE_2D);    // Generate a mipmap for the texture.
    SOIL_free_image_data(image);        // Free the memory used by the intermediate image.

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);    // Unbind the texture.
}

/*
 * GLUT callback that handles a mouse button being pressed or released.
 */
void UMouseButton(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)	// Update left mouse button state.
	{
		if (state == GLUT_DOWN)
		{
			leftButtonDown = true;
			mouseDetected = true;	// Redetect the mouse.
		}
		else
		{
			leftButtonDown = false;
		}
	}
	
	if (button == GLUT_RIGHT_BUTTON)	// Update right mouse button state.
	{
		if (state == GLUT_DOWN)
		{
			rightButtonDown = true;
			mouseDetected = true;	// Redetect the mouse.
		}
		else
		{
			rightButtonDown = false;
		}
	}
}

/*
 * GLUT callback that handles active mouse movement.
 */
void UMouseMove(int x, int y)
{
	if (mouseDetected)	// Reset the last mouse coordinates to prevent jumpy behavior.
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	/*
	 * Calculate the delta from the last mouse position.
	 */
	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y;

	/*
	 * Reset the calculation for the next iteration.
	 */
	lastMouseX = x;
	lastMouseY = y;

	/*
	 * If the left mouse button is being held, adjust the camera orbit. If the right mouse button is being
	 * held, adjust the zoom. This prioritizes orbit adjustment.
	 */
	if (leftButtonDown)
	{
		yaw -= mouseXOffset * sensitivity;
		pitch += mouseYOffset * sensitivity;
	}
	else if (rightButtonDown)
	{
		zoomLevel += mouseYOffset * zoomSpeed;
	}

	/*
	 * Constrain the pitch to the range of -89.0 to 89.0
	 */
	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}

	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	/*
	 * Constrain the zoom level to the range of -10.0 to 10.0
	 */
	if (zoomLevel < -10.0f)
	{
		zoomLevel = -10.0f;
	}

	if (zoomLevel > 10.0f)
	{
		zoomLevel = 10.0f;
	}

	/*
	 * Use the pitch, roll, and zoomLevel to place the camera at the correct orbital location.
	 */
	front.x = (10.001f + zoomLevel) * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	front.y = (10.001f + zoomLevel) * sin(glm::radians(pitch));
	front.z = (10.001f + zoomLevel) * cos(glm::radians(yaw)) * cos(glm::radians(pitch));

	URenderGraphics();

}

/*
 * GLUT Keypress callback function.
 */
void UKeyDown(unsigned char key, int x, int y)
{
	// Check if the key pressed was p. If it was toggle the projection mode.
	if (key == 'p')
	{
		if (orthoFlag)
		{
			orthoFlag = false;
			std::cout << "Projection Mode: Perspective" << std::endl;
		}
		else
		{
			orthoFlag = true;
			std::cout << "Projection Mode: Orthographic" << std::endl;
		}
	}

	// Check if the key pressed was w. If it was toggle the display mode.
	if (key == 'w')
	{
		if (wireFlag)
		{
			wireFlag = false;
			std::cout << "Display Mode: Textured" << std::endl;
		}
		else
		{
			wireFlag = true;
			std::cout << "Display Mode: Wireframe" << std::endl;
		}
	}

	// Check if the key pressed was s. If it was toggle the shading mode.
	if (key == 's')
	{
		if (lightFlag)
		{
			lightFlag = false;
			std::cout << "Shading Mode: Flat" << std::endl;
		}
		else
		{
			lightFlag = true;
			std::cout << "Shading Mode: Lit" << std::endl;
		}
	}
}

/*
 * GLUT Reshape callback function.
 */
void UResizeWindow(int Width, int Height)
{
	glViewport(0, 0, Width, Height);	// Redefine the viewport at the new dimensions.

	WindowWidth = Width;
	WindowHeight = Height;
}

/*
 * GLUT Display callback function.
 */
void URenderGraphics()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear the color and depth buffers.


	CameraForwardZ = front;		// Copy the translation applied in UMouseMove.
	
	glm::mat4 model;	// Create a 4x4 matrix for model transforms.
	
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));		// Apply translation to model matrix.
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));	// Apply rotation to model matrix.
	model = glm::scale(model, glm::vec3(1.3f, 1.5f, 0.5f));			// Apply scaling to model matrix.

	glm::mat4 view;		// Create a 4x4 matrix for view transforms.

	view = glm::lookAt(cameraPosition - CameraForwardZ, cameraPosition, cameraUpY);		// Point the camera at cameraPosition and move it to the offset described by cameraForwardZ.

	viewPosition = cameraPosition + glm::vec3(-CameraForwardZ.x, CameraForwardZ.y, CameraForwardZ.z);	// Line up the position of the camera for calculating reflections.
	
	glm::mat4 projection;	// Create a 4x4 matrix for projection math.

	/*
	 * Choose projection mode based on flag.
	 */
	if (!orthoFlag)
	{
		GLfloat width = WindowWidth, height = WindowHeight;

		if (height == 0) {height = 1;}

		projection = glm::perspective(glm::radians(45.0f), (GLfloat)width / (GLfloat)height, 0.01f, 50.0f);	// Apply perspective transforms to projection matrix.
	}
	else
	{
		GLfloat width = WindowWidth, height = WindowHeight;

		if (height == 0) {height = 1;}

		GLfloat aspect = (GLfloat)width / (GLfloat)height;

		float orthoZoom = (zoomLevel + 11.0f) / 4;
		
		// Apply an orthographic transform.
		projection = glm::ortho(-aspect * orthoZoom, aspect * orthoZoom, -1.0f * orthoZoom, 1.0f * orthoZoom, -100.0f, 100.0f);
	}
	
	/*
	 * Get pointers to the flat shader uniform variables.
	 */
	GLuint flatModelLoc = glGetUniformLocation(shaderProgramFlat, "model");
	GLuint flatViewLoc = glGetUniformLocation(shaderProgramFlat, "view");
	GLuint flatProjLoc = glGetUniformLocation(shaderProgramFlat, "projection");

	/*
	 * Copy the data from the local matrices to the flat shader matrices.
	 */
	glUniformMatrix4fv(flatModelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(flatViewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(flatProjLoc,1, GL_FALSE, glm::value_ptr(projection));
	
	/*
	 * Get pointers to the lit shader uniform variables.
	 */
	GLuint litModelLoc = glGetUniformLocation(shaderProgramLit, "model");
	GLuint litViewLoc = glGetUniformLocation(shaderProgramLit, "view");
	GLuint litProjLoc = glGetUniformLocation(shaderProgramLit, "projection");

	GLuint lightColor0Loc = glGetUniformLocation(shaderProgramLit, "lightColor0");
	GLuint lightPos0Loc = glGetUniformLocation(shaderProgramLit, "lightPos0");		
	GLuint lightFactor0Loc = glGetUniformLocation(shaderProgramLit, "lightFactor0");
	GLuint lightColor1Loc = glGetUniformLocation(shaderProgramLit, "lightColor1");
	GLuint lightPos1Loc = glGetUniformLocation(shaderProgramLit, "lightPos1");
	GLuint lightFactor1Loc = glGetUniformLocation(shaderProgramLit, "lightFactor1");
	GLuint viewPositionLoc = glGetUniformLocation(shaderProgramLit, "viewPosition");

	/*
	 * Copy the data from the local matrices to the lit shader matrices.
	 */
	glUniformMatrix4fv(litModelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(litViewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(litProjLoc,1, GL_FALSE, glm::value_ptr(projection));

	glUniform3f(lightColor0Loc, lightColor0.x, lightColor0.y, lightColor0.z);
	glUniform3f(lightColor1Loc, lightColor1.r, lightColor1.g, lightColor1.b);
	glUniform1f(lightFactor0Loc, lightFactor0);
	glUniform3f(lightPos0Loc, lightPos0.x, lightPos0.y, lightPos0.z);
	glUniform3f(lightPos1Loc, lightPos1.x, lightPos1.y, lightPos1.z);
	glUniform1f(lightFactor1Loc, lightFactor1);
	glUniform3f(viewPositionLoc, viewPosition.x, viewPosition.y, viewPosition.z);

	/*
	 * Choose the shader to use.
	 */
	if (lightFlag && !wireFlag)
	{
		glUseProgram(shaderProgramLit);
	}
	else
	{
		glUseProgram(shaderProgramFlat);
	}
	
	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	/*
	 * Choose wireframe or filled mode.
	 */
	if (!wireFlag)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	for (mesh* mesh : meshes)
	{
		mesh->draw();
	}

	glBindVertexArray(0);	// Release VBO as the active array.

	glutSwapBuffers();		// Switch the front and back buffers.
}

void UCreateShaders()
{
	GLint vertexShaderFlat = glCreateShader(GL_VERTEX_SHADER);			// Get a pointer to an empty vertex shader.
	glShaderSource(vertexShaderFlat, 1, &vertexShaderSourceFlat, NULL);		// Provide the vertex shader source code.
	glCompileShader(vertexShaderFlat);									// Compile the vertex shader

	GLint vertexShaderLit = glCreateShader(GL_VERTEX_SHADER);			// Get a pointer to an empty vertex shader.
	glShaderSource(vertexShaderLit, 1, &vertexShaderSourceLit, NULL);		// Provide the vertex shader source code.
	glCompileShader(vertexShaderLit);									// Compile the vertex shader

	GLint fragmentShaderFlat = glCreateShader(GL_FRAGMENT_SHADER);		// Get a pointer to an empty fragment shader.
	glShaderSource(fragmentShaderFlat, 1, &fragmentShaderSourceFlat, NULL);	// Provide the fragment shader source code.
	glCompileShader(fragmentShaderFlat);								// Compile the fragment shader

	GLint fragmentShaderLit = glCreateShader(GL_FRAGMENT_SHADER);		// Get a pointer to an empty fragment shader.
	glShaderSource(fragmentShaderLit, 1, &fragmentShaderSourceLit, NULL);	// Provide the fragment shader source code.
	glCompileShader(fragmentShaderLit);

	shaderProgramFlat = glCreateProgram();				// Get a pointer to an empty shader program.
	glAttachShader(shaderProgramFlat, vertexShaderFlat);	// Attach the compiled vertex shader to the program.
	glAttachShader(shaderProgramFlat, fragmentShaderFlat);	// Attach the compiled fragment shader to the program.
	glLinkProgram(shaderProgramFlat);					// Link the executables together.

	shaderProgramLit = glCreateProgram();				// Get a pointer to an empty shader program.
	glAttachShader(shaderProgramLit, vertexShaderLit);	// Attach the compiled vertex shader to the program.
	glAttachShader(shaderProgramLit, fragmentShaderLit);	// Attach the compiled fragment shader to the program.
	glLinkProgram(shaderProgramLit);					// Link the executables together.
	
	/*
	 * Get debug information about shader compilation.
	 */

	GLint status ,len;
	GLsizei discard;
	
	glGetShaderiv(vertexShaderFlat, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "Flat Vertex Shader Failed To Compile" << std::endl;
		glGetShaderiv(vertexShaderFlat, GL_INFO_LOG_LENGTH, &len);
		if (len > 0)
		{
			GLchar log[len];
			glGetShaderInfoLog(vertexShaderFlat, len, &discard, &log[0]);
			printf("Flat Vertex Shader Error Log: (%d)\n%s\n", len, log);
		}
	}
	
	glGetShaderiv(vertexShaderLit, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "Lit Vertex Shader Failed To Compile" << std::endl;
		glGetShaderiv(vertexShaderLit, GL_INFO_LOG_LENGTH, &len);
		if (len > 0)
		{
			GLchar log[len];
			glGetShaderInfoLog(vertexShaderLit, len, &discard, &log[0]);
			printf("Lit Vertex Shader Error Log: (%d)\n%s\n", len, log);
		}
	}
	
	glGetShaderiv(fragmentShaderFlat, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "Flat Fragment Shader Failed To Compile" << std::endl;
		glGetShaderiv(fragmentShaderFlat, GL_INFO_LOG_LENGTH, &len);
		if (len > 0)
		{
			GLchar log[len];
			glGetShaderInfoLog(fragmentShaderFlat, len, &discard, &log[0]);
			printf("Flat Fragment Shader Error Log: (%d)\n%s\n", len, log);
		}
	}
	
	glGetShaderiv(fragmentShaderLit, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "Lit Fragment Shader Failed To Compile" << std::endl;
		glGetShaderiv(fragmentShaderLit, GL_INFO_LOG_LENGTH, &len);
		if (len > 0)
		{
			GLchar log[len];
			glGetShaderInfoLog(fragmentShaderLit, len, &discard, &log[0]);
			printf("Lit Fragment Shader Error Log: (%d)\n%s\n", len, log);
		}
	}

	fflush(stdout);
	
	/*
	 * Free the memory used by the compiled shaders.
	 */
	glDeleteShader(vertexShaderFlat);
	glDeleteShader(vertexShaderLit);
	glDeleteShader(fragmentShaderFlat);
	glDeleteShader(fragmentShaderLit);
}

void UCreateBuffers()
{
	/*
	 * Vertex buffer.
	 */
	std::vector<GLfloat> vertices0 = {
		// Position data x,y,z | Normal data x,y,z | UV data u,v
		// Front vertices
		 0.1f,   1.0f,  0.15f,	 0.0f,  0.1f, -0.9f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.15f,	 0.0f,  0.1f, -0.9f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.15f,	 0.0f,  0.1f, -0.9f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.15f,	 0.0f,  0.1f, -0.9f,  0.37f,  1.0f,  
		 
		 0.15f,  0.6f, -0.05f,	 0.0f,  0.05f, -0.95f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.05f,	 0.0f,  0.05f, -0.95f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.05f,	 0.0f,  0.05f, -0.95f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.05f,	 0.0f,  0.05f, -0.95f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.05f,	 0.0f,  0.05f, -0.95f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.05f,	 0.0f,  -0.1f, -0.9f,  0.569f,  0.7f,  
		 0.0f,  0.4f,  -0.05f,	 0.0f,  -0.1f, -0.9f,  0.5f,  0.7f,    
		-0.13f,  0.4f, -0.05f,	 0.0f,  -0.1f, -0.9f,  0.331f,  0.7f,  

		 0.065f,  0.2f,  0.1f,	 0.0f,  -0.05f, -0.95f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.1f,	 0.0f,  -0.05f, -0.95f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.15f,	 0.0f,  0.05f, -0.95f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.15f,	 0.0f,  0.05f, -0.95f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.15f, 0.0f,  0.0f, -1.0f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.15f, 0.0f,  0.0f, -1.0f,  0.369f,  0.3335f,
		
		 0.09f, -0.667f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.383f,  0.1665f,
		
		 0.06f, -1.0f,  0.05f,	 0.0f,  0.0f, -1.0f,  0.578f,  0.0f,
		-0.06f, -1.0f,  0.05f,	 0.0f,  0.0f, -1.0f,  0.422f,  0.0f,

		// Rear vertices

		 0.1f,   1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.37f,  1.0f,  
		 		 
		 0.15f,  0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.569f,  0.7f,
		 0.0f,  0.4f,  -0.15f,	 0.0f,  0.0f,  1.0f,  0.5f,  0.7f,  
		-0.13f,  0.4f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.331f,  0.7f,

		 0.065f,  0.2f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.05f, 0.0f,  0.0f,  1.0f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.05f, 0.0f,  0.0f,  1.0f,  0.369f,  0.3335f,
				
		 0.09f, -0.667f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.383f,  0.1665f,
				
		 0.06f, -1.0f,  -0.05f,	 0.0f,  0.0f,  1.0f,  0.578f,  0.0f,
		-0.06f, -1.0f,  -0.05f,	 0.0f,  0.0f,  1.0f,  0.422f,  0.0f,
	};

	/*
	 * Index buffer.
	 */
	std::vector<GLuint> indices0 = {
		// Front triangles
		4, 0, 5,
		5, 1, 6,
		6, 2, 7,
		7, 3, 8,
		4, 5, 9,
		5, 12, 9,
		5, 6, 12,
		6, 7, 12,
		7, 10, 12,
		7, 8, 10,
		11, 9, 12,
		12, 10, 13,
		11, 12, 14,
		12, 15, 14,
		13, 15, 12,
		14, 15, 16,
		16, 15, 17,
		17, 18, 16,
		18, 17, 19,
		19, 20, 18,
		20, 19, 21,
		21, 22, 20,
		22, 21, 23,
	};

	meshes.push_back(new mesh(vertices0, indices0));
	
	/*
	 * Vertex buffer.
	 */
	std::vector<GLfloat> vertices1 = {
		// Position data x,y,z | Normal data x,y,z | UV data u,v
		// Front vertices
		 0.1f,   1.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.37f,  1.0f,  
		 
		 0.15f,  0.6f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.569f,  0.7f,  
		 0.0f,  0.4f,  -0.05f,	 0.0f,  0.0f, -1.0f,  0.5f,  0.7f,    
		-0.13f,  0.4f, -0.05f,	 0.0f,  0.0f, -1.0f,  0.331f,  0.7f,  

		 0.065f,  0.2f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.15f,	 0.0f,  0.0f, -1.0f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.15f, 0.0f,  0.0f, -1.0f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.15f, 0.0f,  0.0f, -1.0f,  0.369f,  0.3335f,
		
		 0.09f, -0.667f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.1f,	 0.0f,  0.0f, -1.0f,  0.383f,  0.1665f,
		
		 0.06f, -1.0f,  0.05f,	 0.0f,  0.0f, -1.0f,  0.578f,  0.0f,
		-0.06f, -1.0f,  0.05f,	 0.0f,  0.0f, -1.0f,  0.422f,  0.0f,

		// Rear vertices

		 0.1f,   1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.37f,  1.0f,  
		 		 
		 0.15f,  0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.569f,  0.7f,
		 0.0f,  0.4f,  -0.15f,	 0.0f,  0.0f,  1.0f,  0.5f,  0.7f,  
		-0.13f,  0.4f, -0.15f,	 0.0f,  0.0f,  1.0f,  0.331f,  0.7f,

		 0.065f,  0.2f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.05f,	 0.0f,  0.0f,  1.0f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.05f, 0.0f,  0.0f,  1.0f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.05f, 0.0f,  0.0f,  1.0f,  0.369f,  0.3335f,
				
		 0.09f, -0.667f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.0f,	 0.0f,  0.0f,  1.0f,  0.383f,  0.1665f,
				
		 0.06f, -1.0f,  -0.05f,	 0.0f,  0.0f,  1.0f,  0.578f,  0.0f,
		-0.06f, -1.0f,  -0.05f,	 0.0f,  0.0f,  1.0f,  0.422f,  0.0f,
	};

	/*
	 * Index buffer.
	 */
	std::vector<GLuint> indices1 = {
		// Back Triangles
		28, 29, 24,
		29, 30, 25,
		30, 31, 26,
		31, 32, 27,
		28, 33, 29,
		29, 33, 36,
		29, 36, 30,
		30, 36, 31,
		31, 36, 34,
		31, 34, 32,
		33, 35, 36,
		34, 36, 37,
		35, 38, 36,
		36, 38, 39,
		37, 36, 39,
		39, 38, 40,
		40, 41, 39,
		41, 40, 42,
		42, 43, 41,
		43, 42, 44,
		44, 45, 43,
		45, 44, 46,
		46, 47, 45,
	};

	meshes.push_back(new mesh(vertices1, indices1));

	/*
	 * Vertex buffer.
	 */
	std::vector<GLfloat> vertices2 = {
		// Position data x,y,z | Normal data x,y,z | UV data u,v
		// Front vertices
		 0.1f,   1.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  1.0f,  
		 0.033f,  1.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  1.0f,
		-0.033f,  1.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  1.0f,
		-0.1f,   1.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  1.0f,  
		 
		 0.15f,  0.6f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.8f, 
		 0.075f, 0.6f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.8f,
		 0.0f,   0.6f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.8f,   
		-0.075f, 0.6f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.8f,
		-0.15f,  0.6f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.8f, 

		 0.145f,  0.5f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.75f,
		-0.145f,  0.5f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.75f,

		 0.13f,  0.4f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.7f,  
		 0.0f,  0.4f,  -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.7f,    
		-0.13f,  0.4f, -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.7f,  

		 0.065f,  0.2f,  0.1f,	-1.0f,  0.0f,  0.0f,  0.55f,  0.6f,
		-0.065f,  0.2f,  0.1f,	-1.0f,  0.0f,  0.0f,  0.55f,  0.6f,

		 0.07f,  0.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  0.5f,
		-0.07f,  0.0f,  0.15f,	-1.0f,  0.0f,  0.0f,  0.575f,  0.5f,

		 0.08f, -0.333f,  0.15f,-1.0f,  0.0f,  0.0f,  0.575f,  0.3335f,
		-0.08f, -0.333f,  0.15f,-1.0f,  0.0f,  0.0f,  0.575f,  0.3335f,
		
		 0.09f, -0.667f,  0.1f,	-1.0f,  0.0f,  0.0f,  0.55f,  0.1665f,
		-0.09f, -0.667f,  0.1f,	-1.0f,  0.0f,  0.0f,  0.55f,  0.1665f,
		
		 0.06f, -1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  0.0f,
		-0.06f, -1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  0.0f,

		// Rear vertices

		 0.1f,   1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  1.0f,  
		 0.033f,  1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  1.0f,
		-0.033f,  1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  1.0f,
		-0.1f,   1.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  1.0f,  
		 		 
		 0.15f,  0.6f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.8f, 
		 0.075f, 0.6f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.8f,
		 0.0f,   0.6f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.8f,   
		-0.075f, 0.6f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.8f,
		-0.15f,  0.6f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.8f, 

		 0.145f,  0.5f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.75f,
		-0.145f,  0.5f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.75f,

		 0.13f,  0.4f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.7f,
		 0.0f,  0.4f,  -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.7f,  
		-0.13f,  0.4f, -0.15f,	-1.0f,  0.0f,  0.0f,  0.425f,  0.7f,

		 0.065f,  0.2f,  0.0f,	-1.0f,  0.0f,  0.0f,  0.5f,  0.6f,
		-0.065f,  0.2f,  0.0f,	-1.0f,  0.0f,  0.0f,  0.5f,  0.6f,

		 0.07f,  0.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  0.5f,
		-0.07f,  0.0f,  0.05f,	-1.0f,  0.0f,  0.0f,  0.525f,  0.5f,

		 0.08f, -0.333f,  0.05f,-1.0f,  0.0f,  0.0f,  0.525f,  0.3335f,
		-0.08f, -0.333f,  0.05f,-1.0f,  0.0f,  0.0f,  0.525f,  0.3335f,
				
		 0.09f, -0.667f,  0.0f,	-1.0f,  0.0f,  0.0f,  0.5f,  0.1665f,
		-0.09f, -0.667f,  0.0f,	-1.0f,  0.0f,  0.0f,  0.5f,  0.1665f,
				
		 0.06f, -1.0f,  -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.0f,
		-0.06f, -1.0f,  -0.05f,	-1.0f,  0.0f,  0.0f,  0.475f,  0.0f,
	};

	/*
	 * Index buffer.
	 */
	std::vector<GLuint> indices2 = {
		// Left Triangles
		5, 0, 24,
		24, 29, 5,
		6, 1, 25,
		25, 30, 6,
		7, 2, 26,
		26, 31, 7,
		8, 3, 27,
		27, 32, 8,
		10, 8, 32,
		32, 34, 10,
		13, 10, 34,
		34, 37, 13,
		15, 13, 37,
		37, 39, 15,
		17, 15, 39,
		39, 41, 17,
		19, 17, 41,
		41, 43, 19,
		21, 19, 43,
		43, 45, 21,
		23, 21, 45,
		45, 47, 23,
	};

	meshes.push_back(new mesh(vertices2, indices2));

	/*
	 * Vertex buffer.
	 */
	std::vector<GLfloat> vertices3 = {
		// Position data x,y,z | Normal data x,y,z | UV data u,v
		// Front vertices
		 0.1f,   1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  1.0f,  
		 0.033f,  1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  1.0f,
		-0.033f,  1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  1.0f,
		-0.1f,   1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  1.0f,  
		 		  
		 0.15f,  0.6f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.8f, 
		 0.075f, 0.6f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.8f,
		 0.0f,   0.6f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.8f,   
		-0.075f, 0.6f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.8f,
		-0.15f,  0.6f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.8f, 
 
		 0.145f,  0.5f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.75f,
		-0.145f,  0.5f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.75f,
 
		 0.13f,  0.4f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.7f,  
		 0.0f,  0.4f,  -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.7f,    
		-0.13f,  0.4f, -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.7f,  
 
		 0.065f,  0.2f,  0.1f,	 1.0f,  0.0f,  0.0f,  0.55f,  0.6f,
		-0.065f,  0.2f,  0.1f,	 1.0f,  0.0f,  0.0f,  0.55f,  0.6f,
 
		 0.07f,  0.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  0.5f,
		-0.07f,  0.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.575f,  0.5f,
 
		 0.08f, -0.333f,  0.15f, 1.0f,  0.0f,  0.0f,  0.575f,  0.3335f,
		-0.08f, -0.333f,  0.15f, 1.0f,  0.0f,  0.0f,  0.575f,  0.3335f,
				 
		 0.09f, -0.667f,  0.1f,	 1.0f,  0.0f,  0.0f,  0.55f,  0.1665f,
		-0.09f, -0.667f,  0.1f,	 1.0f,  0.0f,  0.0f,  0.55f,  0.1665f,
				 
		 0.06f, -1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  0.0f,
		-0.06f, -1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  0.0f,

		// Rear vertices

		 0.1f,   1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  1.0f,  
		 0.033f,  1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  1.0f,
		-0.033f,  1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  1.0f,
		-0.1f,   1.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  1.0f,  
		 		  
		 0.15f,  0.6f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.8f, 
		 0.075f, 0.6f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.8f,
		 0.0f,   0.6f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.8f,   
		-0.075f, 0.6f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.8f,
		-0.15f,  0.6f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.8f, 
 
		 0.145f,  0.5f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.75f,
		-0.145f,  0.5f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.75f,
 
		 0.13f,  0.4f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.7f,
		 0.0f,  0.4f,  -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.7f,  
		-0.13f,  0.4f, -0.15f,	 1.0f,  0.0f,  0.0f,  0.425f,  0.7f,
 
		 0.065f,  0.2f,  0.0f,	 1.0f,  0.0f,  0.0f,  0.5f,  0.6f,
		-0.065f,  0.2f,  0.0f,	 1.0f,  0.0f,  0.0f,  0.5f,  0.6f,
 
		 0.07f,  0.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  0.5f,
		-0.07f,  0.0f,  0.05f,	 1.0f,  0.0f,  0.0f,  0.525f,  0.5f,
 
		 0.08f, -0.333f,  0.05f, 1.0f,  0.0f,  0.0f,  0.525f,  0.3335f,
		-0.08f, -0.333f,  0.05f, 1.0f,  0.0f,  0.0f,  0.525f,  0.3335f,
				 
		 0.09f, -0.667f,  0.0f,	 1.0f,  0.0f,  0.0f,  0.5f,  0.1665f,
		-0.09f, -0.667f,  0.0f,	 1.0f,  0.0f,  0.0f,  0.5f,  0.1665f,
				 
		 0.06f, -1.0f,  -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.0f,
		-0.06f, -1.0f,  -0.05f,	 1.0f,  0.0f,  0.0f,  0.475f,  0.0f,
	};

	/*
	 * Index buffer.
	 */
	std::vector<GLuint> indices3 = {
		// Right Triangles
		3, 7, 27,
		27, 31, 7,
		6, 26, 2,
		26, 6, 30,
		1, 5, 25,
		25, 5, 29,
		0, 4, 24,
		24, 4, 28,
		4, 9, 28,
		28, 9, 33,
		9, 11, 33,
		33, 11, 35,
		11, 14, 35,
		35, 14, 38,
		14, 16, 38,
		38, 16, 40,
		16, 18, 40,
		40, 18, 42,
		18, 20, 42,
		42, 20, 44,
		20, 22, 44,
		44, 22, 46,
	};

	meshes.push_back(new mesh(vertices3, indices3));

	/*
	 * Vertex buffer.
	 */
	std::vector<GLfloat> vertices4 = {
		// Position data x,y,z | Normal data x,y,z | UV data u,v
		// Front vertices
		 0.1f,   1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.15f,	 1.0f,  0.0f,  0.0f,  0.37f,  1.0f,  
		 
		 0.15f,  0.6f, -0.05f,	 0.7f,  0.3f,  0.0f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.05f,	 0.7f,  0.3f,  0.0f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.05f,	 0.7f,  0.3f,  0.0f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.05f,	 0.7f,  0.3f,  0.0f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.05f,	 0.7f,  0.3f,  0.0f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.05f,	 0.5f,  0.5f,  0.0f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.05f,	 0.5f,  0.5f,  0.0f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.05f,	 0.3f,  0.7f,  0.0f,  0.569f,  0.7f,  
		 0.0f,  0.4f,  -0.05f,	 0.3f,  0.7f,  0.0f,  0.5f,  0.7f,    
		-0.13f,  0.4f, -0.05f,	 0.3f,  0.7f,  0.0f,  0.331f,  0.7f,  

		 0.065f,  0.2f,  0.1f,	 0.2f,  0.8f,  0.0f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.1f,	 0.2f,  0.8f,  0.0f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.15f,	 0.0f,  1.0f,  0.0f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.15f,	 0.0f,  1.0f,  0.0f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.15f, 0.0f,  0.7f,  0.3f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.15f, 0.0f,  0.7f,  0.3f,  0.369f,  0.3335f,
		
		 0.09f, -0.667f,  0.1f,	 0.0f,  0.3f,  0.7f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.1f,	 0.0f,  0.3f,  0.7f,  0.383f,  0.1665f,
		
		 0.06f, -1.0f,  0.05f,	 0.0f, -1.0f,  0.0f,  0.578f,  0.5f,
		-0.06f, -1.0f,  0.05f,	 0.0f, -1.0f,  0.0f,  0.422f,  0.5f,

		// Rear vertices

		 0.1f,   1.0f,  0.05f,	 0.0f,  1.0f,  1.0f,  0.63f,  1.0f,  
		 0.033f,  1.0f,  0.05f,	 0.0f,  1.0f,  1.0f,  0.5429f,  1.0f,
		-0.033f,  1.0f,  0.05f,	 0.0f,  1.0f,  1.0f,  0.4571f,  1.0f,
		-0.1f,   1.0f,  0.05f,	 0.0f,  1.0f,  1.0f,  0.37f,  1.0f,  
		 
		 0.15f,  0.6f, -0.15f,	 0.3f,  0.7f,  0.7f,  0.695f,  0.8f, 
		 0.075f, 0.6f, -0.15f,	 0.3f,  0.7f,  0.7f,  0.5975f,  0.8f,
		 0.0f,   0.6f, -0.15f,	 0.3f,  0.7f,  0.7f,  0.5f,  0.8f,   
		-0.075f, 0.6f, -0.15f,	 0.3f,  0.7f,  0.7f,  0.4025f,  0.8f,
		-0.15f,  0.6f, -0.15f,	 0.3f,  0.7f,  0.7f,  0.305f,  0.8f, 

		 0.145f,  0.5f, -0.15f,	 0.7f,  0.5f,  0.3f,  0.6885f,  0.75f,
		-0.145f,  0.5f, -0.15f,	 0.7f,  0.5f,  0.3f,  0.3115f,  0.75f,

		 0.13f,  0.4f, -0.15f,	 0.7f,  0.7f,  0.0f,  0.569f,  0.7f,
		 0.0f,  0.4f,  -0.15f,	 0.7f,  0.7f,  0.0f,  0.5f,  0.7f,  
		-0.13f,  0.4f, -0.15f,	 0.7f,  0.7f,  0.0f,  0.331f,  0.7f,

		 0.065f,  0.2f,  0.0f,	 1.0f,  0.8f,  0.0f,  0.5845f,  0.6f,
		-0.065f,  0.2f,  0.0f,	 1.0f,  0.8f,  0.0f,  0.4155f,  0.6f,

		 0.07f,  0.0f,  0.05f,	 1.0f,  1.0f,  0.0f,  0.591f,  0.5f,
		-0.07f,  0.0f,  0.05f,	 1.0f,  1.0f,  0.0f,  0.409f,  0.5f,

		 0.08f, -0.333f,  0.05f, 1.0f,  0.7f,  0.3f,  0.604f,  0.3335f,
		-0.08f, -0.333f,  0.05f, 1.0f,  0.7f,  0.3f,  0.369f,  0.3335f,
		
		 0.09f, -0.667f,  0.0f,	 1.0f,  0.3f,  0.7f,  0.617f,  0.1665f,
		-0.09f, -0.667f,  0.0f,	 1.0f,  0.3f,  0.7f,  0.383f,  0.1665f,
		
		 0.06f, -1.0f,  -0.05f,	 0.0f, -1.0f,  0.0f,  0.578f,  0.45f,
		-0.06f, -1.0f,  -0.05f,	 0.0f, -1.0f,  0.0f,  0.422f,  0.45f,
	};

	/*
	 * Index buffer.
	 */
	std::vector<GLuint> indices4 = {
		// Bottom Triangles
		23, 22, 47,
		47, 22, 46
	};

	meshes.push_back(new mesh(vertices4, indices4));

	glBindVertexArray(0);
}

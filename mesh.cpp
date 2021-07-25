#include "mesh.h"

/*
 * Create an attribute array on the graphics card, upload vertex and index data, and define the layout of the data.
 */
mesh::mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices)
{
	indices_count = indices.size();
	
	glGenVertexArrays(1, &VAO);		// Generate a vertex array object, storing a pointer to it in VAO.
	glGenBuffers(1, &VBO);			// Generate a buffer, storing a pointer to it in VBO.
	glGenBuffers(1, &EBO);			// Generate a buffer, storing a pointer to it in EBO.

	glBindVertexArray(VAO);		// Make VAO the active array.

	glBindBuffer(GL_ARRAY_BUFFER, VBO);		// Make VBO the active buffer.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);		// Send vertex data from VBO to GPU.

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);		// Make EBO the active buffer.
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);	// Send index data to GPU.

	glEnableVertexAttribArray(0);	// Enable the first attribute array.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);	// Define the location of position data in the buffer.

	glEnableVertexAttribArray(1);	// Enable the second attribute array.
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (char*)(3 * sizeof(GLfloat)));		// Define the location of color data in the buffer.

	glEnableVertexAttribArray(2);	// Enable the second attribute array.
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (char*)(6 * sizeof(GLfloat)));		// Define the location of color data in the buffer.
	
	glBindVertexArray(0);
}

/*
 * Bind the current mesh's array and draw the triangles.
 */
void mesh::draw()
{
	glBindVertexArray(VAO);		// Activate VAO as the active array.
	glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);	// Draw triangles using both vertex and index buffer.
}

/*
 * Tell the graphics card to free the memory used by the array and buffers.
 */
mesh::~mesh()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}


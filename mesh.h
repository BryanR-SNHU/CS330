#ifndef MESH_H_
#define MESH_H_

#include <GL/glew.h>
#include <vector>

class mesh {
public:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	GLuint indices_count;
	
	mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices);
	void draw();
	virtual ~mesh();
};

#endif /* MESH_H_ */

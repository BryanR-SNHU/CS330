#include "mesh.h"

#ifndef MESHGROUP_H_
#define MESHGROUP_H_

class MeshGroup {
public:
	std::vector<mesh*> meshes;
	GLuint texture;
	
	MeshGroup();
	void draw();
	void add_mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices);
	void set_texture(const char *path);
	virtual ~MeshGroup();
};

#endif /* MESHGROUP_H_ */

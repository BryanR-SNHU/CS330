#include "SOIL2/SOIL2.h"

#include "MeshGroup.h"

MeshGroup::MeshGroup()
{

}

/*
 * Binds the group's texture calls each mesh's draw method.
 */
void MeshGroup::draw()
{
	glBindTexture(GL_TEXTURE_2D, texture);
	
	for (mesh* mesh : meshes)
	{
		mesh->draw();
	}
}

/*
 * Calls a mesh constructor and adds the mesh to the end of the list.
 */
void MeshGroup::add_mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices)
{
	meshes.push_back(new mesh(vertices, indices));
}

/*
 * Uses SOIL to convert an image into a byte array and upload it to the graphics card.
 */
void MeshGroup::set_texture(const char *path)
{
	glGenTextures(1, &texture);             // Get a pointer to an empty texture.
    glBindTexture(GL_TEXTURE_2D, texture);  // Configure it as a 2D texture.

    int width, height;

    unsigned char* image = SOIL_load_image( path, &width, &height, 0 , SOIL_LOAD_RGB);    // Load an image as an array of bytes.

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);  // Upload the image data into the active texture.
    glGenerateMipmap(GL_TEXTURE_2D);    // Generate a mipmap for the texture.
    SOIL_free_image_data(image);        // Free the memory used by the intermediate image.

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);    // Unbind the texture.
}

MeshGroup::~MeshGroup()
{
	for (mesh* mesh : meshes)
	{
		delete mesh;
	}
}


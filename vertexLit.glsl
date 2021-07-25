#version 440
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
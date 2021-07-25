#version 440
in vec3 Normal;
in vec3 FragmentPos;
in vec2 mobileTextureCoordinate;

out vec4 gpuColor;

uniform sampler2D uTexture;

void main()
{
	gpuColor = texture(uTexture, mobileTextureCoordinate);    // Sample the texture at supplied coordinate
}
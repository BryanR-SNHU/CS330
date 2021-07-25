#version 440
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
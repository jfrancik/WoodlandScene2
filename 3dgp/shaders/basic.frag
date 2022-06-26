#version 330

uniform int level;

uniform mat4 matrixView;
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;

// Texture
uniform sampler2D texture0;
uniform sampler2D textureNormal;

// Fog Colour
uniform vec3 fogColour = vec3(0.40, 0.40, 0.5);

uniform bool bNormalMap = false;

in vec4 color;
in vec4 position;
in vec3 normal;
in vec2 texCoord0;
in float fogFactor;
in mat3 matrixTangent;

out vec4 outColor;

vec3 normalNew;

struct DIRECTIONAL
{	
	vec3 direction;
	vec3 diffuse;
};
uniform DIRECTIONAL lightDir;

vec4 DirectionalLight(DIRECTIONAL light)
{
	// Calculate Directional Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(mat3(matrixView) * light.direction);
	float NdotL = dot(normalNew, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * NdotL;
	return color;
}

void main(void) 
{
	if (bNormalMap)
	{
		normalNew = 2.0 * texture(textureNormal, texCoord0).xyz - vec3(1.0, 1.0, 1.0);
		normalNew = normalize(matrixTangent * normalNew);
	}
	else
		normalNew = normal;

	outColor = color;
	outColor += DirectionalLight(lightDir);
	if (level >= 3) outColor *= texture(texture0, texCoord0);
	if (level >= 5) outColor = mix(vec4(fogColour, 1), outColor, fogFactor);
	if (level == 2) outColor = vec4(0, 0, 0, 1);
}

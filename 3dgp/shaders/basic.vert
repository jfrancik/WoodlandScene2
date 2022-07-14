// VERTEX SHADER

#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;
uniform mat4 matrixInvView;

// Uniforms: Material Colours
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;

// Uniform: Fog Density
uniform float fogDensity = 0.02;

// Bone Transforms
#define MAX_BONES 100
uniform mat4 bones[MAX_BONES];

// Instancing
uniform bool instancing = false;

in vec3 aVertex;
in vec3 aNormal;
in vec2 aTexCoord;
in vec3 aTangent;
in vec3 aBiTangent;
in ivec4 aBoneId;		// Bone Ids
in  vec4 aBoneWeight;	// Bone Weights
in vec3 aOffset;

out vec4 color;
out vec4 position;
out vec3 normal;
out vec2 texCoord0;
out float fogFactor;
out mat3 matrixTangent;

mat4 rotationMatrix(vec3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    
    return mat4(c, 0, s, 0,
                0, 1, 0, 0,
               -s, 0, c, 0,
                0, 0, 0, 1);
}

void main(void) 
{
	mat4 matrixBone;
	if (aBoneWeight[0] == 0.0)
		matrixBone = mat4(1);
	else
		matrixBone = (bones[aBoneId[0]] * aBoneWeight[0] +
					  bones[aBoneId[1]] * aBoneWeight[1] +
					  bones[aBoneId[2]] * aBoneWeight[2] +
					  bones[aBoneId[3]] * aBoneWeight[3]);

	// calculate position
	position = matrixModelView * matrixBone * vec4(aVertex, 1.0);

	if (instancing)
		position = matrixView * (matrixInvView * position + vec4(aOffset, 0));

	gl_Position = matrixProjection * position;

	// calculate normal
	normal = normalize(mat3(matrixModelView) * mat3(matrixBone) * aNormal);

	// calculate UV
	texCoord0 = aTexCoord;

	// calculate tangent local system transformation
	vec3 tangent = normalize(mat3(matrixModelView) * aTangent);
	vec3 biTangent = normalize(mat3(matrixModelView) * aBiTangent);
	matrixTangent = mat3(tangent, biTangent, normal);

	// calculate the fog factor
	fogFactor = exp2(-fogDensity * length(position));

	// calculate light - start with pitch black
	color = vec4(0);

	// ambient light
	color += vec4(materialAmbient, 1);
}

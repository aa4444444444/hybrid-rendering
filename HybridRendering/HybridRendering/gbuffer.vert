#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Required by Mesh::setupMesh()
layout (location = 3) in vec3 aTangent; // Unused in some cases
layout (location = 4) in vec3 aBitangent; // Unused in some cases

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out vec4 CurrClipPos;
out vec4 PrevClipPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// TODO: Do stuff with prevModel when objects start moving
// uniform mat4 prevModel;
uniform mat4 prevView;
uniform mat4 prevProjection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec4 currPos = projection * view * worldPos;
    vec4 prevPos = prevProjection * prevView * worldPos;

    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;
    Normal = normalMatrix * aNormal;
    CurrClipPos = currPos;
    PrevClipPos = prevPos;

    gl_Position = currPos;
}
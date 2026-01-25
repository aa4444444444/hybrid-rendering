#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// How/What we want to render
// 0 ==> Texture diffuse/specular
// 1 ==> Position
// 2 ==> Normals
// 3 ==> Albedo
// 4 ==> Specular
uniform int renderingMode;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;

    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);

    if(renderingMode == 1){
        gAlbedoSpec = vec4(FragPos, 1.0f);
    } else if(renderingMode == 2){
        gAlbedoSpec = vec4(Normal, 1.0f);
    } else if(renderingMode == 3){
        gAlbedoSpec = vec4(texture(texture_diffuse1, TexCoords).rgb, 1.0f);
    } else if(renderingMode == 4){
        gAlbedoSpec = vec4(texture(texture_specular1, TexCoords).rrr, 1.0f);
    } else { // Default to rendering texture 
        // and the diffuse per-fragment color
        gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;

        // store specular intensity in gAlbedoSpec's alpha component
        gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
    }
    
}
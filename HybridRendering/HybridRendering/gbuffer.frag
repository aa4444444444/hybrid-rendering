#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec4 FragMotionVec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 CurrClipPos;
in vec4 PrevClipPos;

// How/What we want to render
// 0 ==> Texture diffuse/specular
// 1 ==> Position
// 2 ==> Normals
// 3 ==> Albedo
// 4 ==> Specular
// 5 ==> Motion
uniform int renderingMode;

uniform vec3 CameraPosition;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

vec2 computeMotionVec(vec4 prevPos, vec4 currPos) {
    vec2 current = (currPos.xy / currPos.w);
    vec2 previous = (prevPos.xy / prevPos.w);
    // Now [-1,1]

    current = current * 0.5 + 0.5;
    previous = previous * 0.5 + 0.5;
    // Now [0, 1]

    // Calculate velocity (current -> previous)
    return (previous - current);
}

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;

    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);

    float Depth = distance(CameraPosition, FragPos);
    float DepthDerivative = max(abs(dFdx(Depth)), abs(dFdy(Depth)));

    // also store the motion / depth vector
    vec2 motion = computeMotionVec(PrevClipPos, CurrClipPos);

    FragMotionVec = vec4(motion, Depth, DepthDerivative);

    if(renderingMode == 1){
        gAlbedoSpec = vec4(FragPos, 1.0f);
    } else if(renderingMode == 2){
        gAlbedoSpec = vec4(Normal, 1.0f);
    } else if(renderingMode == 3){
        gAlbedoSpec = vec4(texture(texture_diffuse1, TexCoords).rgb, 1.0f);
    } else if(renderingMode == 4){
        float spec = texture(texture_specular1, TexCoords).r;
        gAlbedoSpec = vec4(vec3(spec), 1.0);
    } else if(renderingMode == 5){
        vec2 visMotion = (PrevClipPos.xy / PrevClipPos.w) - (CurrClipPos.xy / CurrClipPos.w);
        visMotion = abs(visMotion) * 50.0;
        gAlbedoSpec = vec4(visMotion, 0.0, 1.0);
    } else if(renderingMode == 6) {
        float ndcDepth = CurrClipPos.z / CurrClipPos.w;
        float depth = ndcDepth * 0.5 + 0.5;
        gAlbedoSpec = vec4(vec3(depth), 1.0);
    } else { // Default to rendering texture 
        vec4 diffuseSample = texture(texture_diffuse1, TexCoords);

        // Alpha mask cutout (matches glTF MASK mode, alphaCutoff = 0.5)
        if (diffuseSample.a < 0.5) discard;

        // and the diffuse per-fragment color
        gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;

        // store specular intensity in gAlbedoSpec's alpha component
        gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
    }

}
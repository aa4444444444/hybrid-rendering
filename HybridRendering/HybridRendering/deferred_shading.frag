#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

const int NR_LIGHTS = 1;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2DArray shadowMaps; // array so that we have one per light

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
    float MaxDistance;
    float Radius;
};

uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

// How/What we want to render
// 0 ==> Default
// 1 ==> Shadows
uniform int renderingMode;

void main()
{             
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate distance between light source and current fragment
        float distance = length(lights[i].Position - FragPos);
        if(distance < lights[i].MaxDistance)
        {
            // diffuse
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
            vec3 specular = lights[i].Color * spec * Specular;
            // attenuation
            float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;

            float shadow = texture(shadowMaps, vec3(TexCoords, i)).r; // Since this only has an R channel
            
            // In Shadow

            lighting += (diffuse + specular) * shadow;
            
        }
    }

    if (renderingMode == 0) {
        FragColor = vec4(lighting, 1.0);
    } else {
        // Shadows
        float Shadow = texture(shadowMaps, vec3(TexCoords, 0)).r;
        FragColor = vec4(Shadow, Shadow, Shadow, 1.0f);
    }
    
}
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

#define MAX_N_POINT_LIGHTS 100
uniform Material material;
uniform DirLight dirLight;
uniform int nPointLights;
uniform PointLight pointLight[MAX_N_POINT_LIGHTS];

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 TexColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * TexColor.rgb;
    vec3 diffuse = light.diffuse * diff * TexColor.rgb;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec4 TexColor){

    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(viewDir, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * TexColor.rgb;
    vec3 diffuse = light.diffuse * diff * TexColor.rgb;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);

    vec4 TexColor = texture(material.texture_diffuse1, TexCoords);
        if(TexColor.a < 0.1)
            discard;

    vec3 result = vec3(0.0);
    result += CalcDirLight(dirLight, normal, viewDir, TexColor);

    for(int i=0; i < nPointLights && i < MAX_N_POINT_LIGHTS; i++){
        result += CalcPointLight(pointLight[i], normal, FragPos, viewDir, TexColor);
    }

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.85){
        BrightColor = vec4(result, 1.0);
    }else{
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    FragColor = vec4(result, 1.0);
}

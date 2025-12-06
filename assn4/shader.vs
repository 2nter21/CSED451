#version 330 core
#define NR_POINT_LIGHTS 4
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int shadingMode; // 0=gouraud,1=phong,2=phong+normalmap

// Directional light (common)
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// Point light
struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform vec3 cameraPos;

// outputs
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Tangent;
out vec3 Color_Gouraud; // for Gouraud shading

// helper: phong calc (for gouraud)
vec3 CalcDirectional(vec3 normal, vec3 viewDir, vec3 lightDir) {
    vec3 ambient = dirLight.ambient;
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = dirLight.diffuse * diff;
    vec3 reflectDir = reflect(lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = dirLight.specular * spec;
    return ambient + diffuse + specular;
}

vec3 CalcPoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 lightPos) {
    vec3 lightDir = normalize(fragPos - lightPos); // note: using fragPos - lightPos so matches fragment calc
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance*distance));
    vec3 ambient = light.ambient;
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff;
    vec3 reflectDir = reflect(lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;
    return (ambient + diffuse + specular) * attenuation;
}

void main() {
    mat4 mv = view * model;
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    // normal matrix 3x3
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTex;
    Tangent = mat3(model) * aTangent; // transform tangent (approx.)

    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Gouraud: compute per-vertex lighting here and send Color_Gouraud
    if (shadingMode == 0) {
        vec3 N = normalize(Normal);
        vec3 viewDir = normalize(cameraPos - FragPos);

        // directional
        vec3 result = CalcDirectional(N, viewDir, dirLight.direction);
        // point
        for(int i = 0; i < NR_POINT_LIGHTS; i++) {
            result += CalcPoint(pointLights[i], N, FragPos, viewDir, pointLights[i].position);
        }

        Color_Gouraud = result; // used directly in fragment
    }
}

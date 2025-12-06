#version 330 core
#define NR_POINT_LIGHTS 4
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangent;
in vec3 Color_Gouraud;

out vec4 FragColor;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform int shadingMode; // 0=gouraud,1=phong,2=phong+normalmap
uniform vec3 objectColor;
uniform vec3 cameraPos;

// Directional light struct
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular; // fixed typo if copy
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLights[NR_POINT_LIGHTS];

// Helper functions (same formulas as vertex shader)
vec3 calcDirLight(vec3 N, vec3 V) {
    vec3 L = normalize(-dirLight.direction);
    vec3 ambient = dirLight.ambient;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = dirLight.diffuse * diff;
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = dirLight.specular * spec;
    return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos) {
    vec3 L = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = light.ambient;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = light.diffuse * diff;
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = light.specular * spec;
    return (ambient + diffuse + specular) * attenuation;
}

void main() {
    vec3 color = texture(diffuseMap, TexCoord).rgb * objectColor;
    if (shadingMode == 0) {
        // Gouraud: fragment simply uses interpolated color
        vec3 lit = Color_Gouraud;
        FragColor = vec4(lit * color, 1.0);
        return;
    }

    // For Phong modes: we need normal at fragment
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);

    if (shadingMode == 2) {
        // normal mapping: compute TBN
        vec3 T = normalize(Tangent);
        vec3 B = normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);
        vec3 nmap = texture(normalMap, TexCoord).rgb;
        nmap = nmap * 2.0 - 1.0;
        N = normalize(TBN * nmap);
    }

    vec3 result = vec3(0.0);
    result += calcDirLight(N, V);
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += calcPointLight(pointLights[i], N, V, FragPos);

    vec3 finalColor = result * color;
    FragColor = vec4(finalColor, 1.0);
}

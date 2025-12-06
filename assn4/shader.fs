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
uniform bool isShadow;

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

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

uniform SpotLight spotLight;

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

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    vec3 ambient = light.ambient * attenuation; 
    vec3 diffuse = light.diffuse * diff * attenuation * intensity;
    vec3 specular = light.specular * spec * attenuation * intensity;
    return (ambient + diffuse + specular);
}

void main() {
    if (isShadow) {
        FragColor = vec4(0.3, 0.3, 0.3, 1.0);
        return;
    }
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
    result += CalcSpotLight(spotLight, N, FragPos, V);
    vec3 finalColor = result * color;
    FragColor = vec4(finalColor, 1.0);
}

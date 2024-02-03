#version 150 core

// Kolor ostateczny
out vec4 outColor;

// Dane wejsciowe
in vec4 Position;
in vec2 UV;
in vec3 Normal;
in vec3 lightCoef;
in vec4 fragPosLight;

uniform vec3 cameraPos;
uniform sampler2D tex;
uniform samplerCube tex_skybox;

uniform int isTexture;
uniform int lightModel = 1;
uniform int lightSwitch = 1;
uniform int flower = 1;
uniform int lightAnimation = 0;
uniform float lightTime = 1;
uniform int colorSphere = 1;
uniform int lightType   = 0;
uniform int withReflection = 0;
uniform int withRefraction = 0;

uniform vec3 lightDirection;
// Dane potrzebne do wyliczenia cienia
uniform sampler2D tex_shadowMap;

// Struktura parametrow swiatla
struct LightParam
{
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Attenuation;
    vec3 Position;
};
// Struktura parametrow swiatla
struct DirectionalLightParam
{
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Direction; // Direction dla kierunkowego
};

uniform DirectionalLightParam myDirectionalLight;
uniform LightParam lights[];


// Struktura parametrow materialu
struct MaterialParam
{
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    float Shininess;
};
// Przykladowy material
uniform MaterialParam myMaterial;


float calcDirectionalShadow(vec4 fragPosLight, vec3 fragNormal, vec3 lightDirection)
{

	// Brak cienia
	// return 0;

	// Korekcja perspektywiczna (dla oswietlenia kierunkowego niepotrzebna)
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    // przeksztalcenie wartosci [-1,+1] na [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // pobranie z tekstury shadowMap odleglosci od zrodla swiatla fragmentu
    // do fragmentu oswietlonego na drodze do aktualnego fragmentu
    float closestDepth = texture(tex_shadowMap, projCoords.xy).r;

    // obliczenie aktualnej odleglosci od zrodla swiatla
    float currentDepth = projCoords.z;

    //float bias = 0.01;
	//float shadow = 0.0;

	float bias = max(0.02 * (1.0 - dot(fragNormal, lightDirection)), 0.0001);

	float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
	return shadow;

}
// ------------------------------------------------------------
// Oswietlenie punktowe
vec3 calculatePointLight(LightParam light, MaterialParam material, int lightModel)
{

    // Ambient
    vec3 ambientPart = light.Ambient * material.Ambient;

    // Diffuse
    vec3 L = normalize(light.Position - Position.xyz);
    float diff = max(dot(L, Normal), 0);
    vec3 diffusePart = diff * light.Diffuse * material.Diffuse;

    // Specular
    vec3 E = normalize(cameraPos - Position.xyz);

    float spec;
    if(lightModel == 1 )
    {
        vec3 R = reflect(-E, Normal);
        spec = pow(max(dot(R,L), 0), material.Shininess);
    }
    else
    {
        vec3 H = normalize(L + E);
        spec = pow(max(dot(H, Normal), 0 ), material.Shininess);
    }

    vec3 specularPart = spec * light.Specular * material.Specular;

    // Wspolczynnik tlumienia
    float LV = distance(Position.xyz, light.Position);
    float latt = 1.0 / (light.Attenuation.x + light.Attenuation.y * LV + light.Attenuation.z * LV * LV);

    // Glowny wzor
    vec3 lightCoef = ambientPart + latt * (diffusePart + specularPart);
    return lightCoef;
}
// Oswietlenie kierunkowe
vec3 calculateDirectionalLight(DirectionalLightParam light, MaterialParam material, int lightModel)
{
    // Ambient
    //vec3 ambientPart = light.Ambient * material.Ambient;

    // Diffuse
    vec3 L = normalize(-light.Direction);
    //vec3 L = normalize(light.Direction);
    float diff = max(dot(L, Normal), 0);
    vec3 diffusePart = diff * light.Diffuse * material.Diffuse;

    // Specular
    vec3 E = normalize(cameraPos - Position.xyz);

    float spec;
    if (lightModel == 1)
    {
        vec3 R = reflect(-E, Normal);
        spec = pow(max(dot(R, L), 0), material.Shininess);
    }
    else
    {
        vec3 H = normalize(L + E);
        spec = pow(max(dot(H, Normal), 0), material.Shininess);
    }

    vec3 specularPart = spec * light.Specular * material.Specular;
    //vec3 lightCoef = ambientPart + diffusePart + specularPart;
    vec3 lightCoef = diffusePart + specularPart;
    return lightCoef;
}

vec4 calculateLightsSum(MaterialParam material, int shadingModel)
{
    vec4 sum = vec4((calculatePointLight(lights[0], material, shadingModel)+ calculatePointLight(lights[1], material, shadingModel)),1.0);
    return sum;
}

void main()
{
    // Cienie
	float shadowPart = calcDirectionalShadow(fragPosLight, Normal, lightDirection);

    vec3 Dir = normalize(Position.xyz - cameraPos);

    // odleglosc od poczatku ukladu wspolrzednych
    float dist = length(Position);

    vec4 texColor = texture(tex, UV);



    // szalone kolory
    vec3 crazyColor = vec3(
                          sin(dist*3.0)/2.0 + 0.5,
                          sin(Position.y)/2.0 + 0.5,
                          0.5
                      );

    vec4 fragColor = vec4(0.0);

    if (isTexture == 1)
    {
        if (texColor.a >= 0.5)
        {
            fragColor = texColor;
        }
        else
        {
            discard;
        }

    }
    else if (colorSphere == 0)
    {
        fragColor = vec4(lights[1].Diffuse.xyz, 1.0);
    }
    else if (colorSphere == 1)
    {
        fragColor = vec4(lights[0].Diffuse.xyz, 1.0);
    }
    else
    {
        fragColor = vec4(crazyColor, 1.0);
    }

    // 1. Odbicie (reflection)
    vec3 Reflection = reflect(Dir, Normal);
    vec3 ReflectionColor = texture(tex_skybox, Reflection).rgb;

    // 2. Refrakcja (refraction)
    float refractionCoeff = 1.0 / 1.2;
    vec3 Refraction = refract(Dir, Normal, refractionCoeff);
    vec3 RefractionColor = texture(tex_skybox, Refraction).rgb;

    vec4 finalColor;

    if(withReflection == 1 )
    {
        if(withRefraction == 1 )
            finalColor = 0.25 * fragColor + 0.25 * vec4(ReflectionColor, 1.0) + 0.25 * vec4(RefractionColor, 1.0);
        else
            finalColor = vec4(ReflectionColor, 1.0);
    }
    else if(withRefraction == 1 )
    {
        finalColor = vec4(RefractionColor, 1.0);
    }
    else
        finalColor= fragColor;



    if(flower == 1)
    {
        outColor = finalColor;
    }
    else if (lightSwitch == 1 )
    {
        if (lightType == 0)
        {
            outColor = calculateLightsSum(myMaterial,lightModel) * finalColor;
        }
        else
        {
            vec4 lightPart = vec4(calculateDirectionalLight(myDirectionalLight, myMaterial, lightModel) * finalColor.xyz, 1.0);
            outColor = (vec4(myDirectionalLight.Ambient,1) + (1 - shadowPart) * lightPart) * finalColor;
        }
    }
    else
    {
        outColor = finalColor;
    }

}

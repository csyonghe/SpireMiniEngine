module CheckerBoardPattern implements IMaterialPattern
{
    require vec2 vertUV;

    param Texture2D albedoMap;
    param float uvScale;
    require SamplerState textureSampler;

    public vec3 normal = vec3(0.0, 0.0, 1.0);
    
    public float roughness = 0.8;
    public float metallic = 0.2;
    public float specular = 0.2;
    public vec3 albedo
    {
        float checkerBoard = albedoMap.Sample(textureSampler, vertUV*uvScale).x * 0.3 + 0.7;
        return vec3(checkerBoard * 0.4);
    }

    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}


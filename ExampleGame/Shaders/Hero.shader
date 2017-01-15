module HeroPattern implements IMaterialPattern
{
    param Texture2D albedoMap;
    
    require vec2 vertUV;
    require SamplerState textureSampler;
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 0.4;
    public float metallic = 0.8;
    public float specular = 0.6;
    public vec3 albedo = albedoMap.Sample(textureSampler, vertUV).xyz;
}

//using "DefaultGeometry.shader";
module MaterialGeometry
{
    require vec3 coarseVertPos;
    require vec3 coarseVertNormal;
    public using NoTessellation;
    public vec3 displacement = vec3(0.0);
}
module MaterialPattern
{
    require vec2 vertUV;
    require vec3 WorldSpaceToTangentSpace(vec3 v);
    require vec3 cameraPos;
    require vec3 pos;
    
    @MaterialUniform Texture2D albedoMap;
    @MaterialUniform Texture2D normalMap;
    @MaterialUniform Texture2D displacementMap;
    require SamplerState textureSampler;

    vec3 viewDirTan = WorldSpaceToTangentSpace(normalize(cameraPos - pos));
    using pom = ParallaxOcclusionMapping(
        heightTexture: displacementMap,
        viewDirTangentSpace: WorldSpaceToTangentSpace(normalize(cameraPos - pos)),
        uv: vertUV,
        parallaxScale: 0.02
    );
    
    vec2 uv = pom.uvOut;
    
    public vec3 albedo = albedoMap.Sample(textureSampler, uv).xyz * 0.7;
    public vec3 normal = normalize(normalMap.Sample(textureSampler, uv).xyz * 2.0 - 1.0);
    public float roughness = 0.5;
    public float metallic = 0.3;
    public float specular = 1.0;
    public float selfShadow(vec3 lightDir)
    {
        return pom.selfShadow(WorldSpaceToTangentSpace(lightDir));        
    }
}
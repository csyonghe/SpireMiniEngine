using "DefaultGeometry.shader";

module MaterialPattern implements IMaterialPattern
{
    param Texture2D maskTexture;
    param Texture2D dirtTexture;
    param Texture2D diffuseTexture;
    param Texture2D normalTexture;
    param Texture2D maskTexture2;
    param Texture2D diffuseTexture2;
    param Texture2D normalTexture2;
    require SamplerState textureSampler;
    require vec3 vTangent;
    require vec3 vBiTangent;
    require vec3 pos;
    require vec3 cameraPos;
    require vec3 WorldSpaceToTangentSpace(vec3 v);
    require vec2 vertUV;
    vec2 uv = vertUV;//vec2(dot(vBiTangent, pos), dot(vTangent, pos)) * 0.001;

    vec2 tiledUV = uv * 4.0;
    vec2 tiledUV2 = uv * 2.0;
    vec3 t0 = diffuseTexture.Sample(textureSampler, tiledUV).xyz;
    vec3 t1 = dirtTexture.Sample(textureSampler, uv).xyz;

    vec3 viewDirTan = WorldSpaceToTangentSpace(normalize(cameraPos - pos));

    float getHeight1(vec2 uvCoord){ return 1.0-0.0625; }
    using p1 = ParallaxOcclusionMapping(getHeight1, viewDirTan, uv, 0.2);

    float getHeight2(vec2 uvCoord){ return 0.0; }
    using p2 = ParallaxOcclusionMapping(getHeight2, viewDirTan, tiledUV2, 0.02);

    float mask1 = clamp(maskTexture2.Sample(textureSampler, p1.uvOut).g, 0.5, 1.0);
    float mask2 = maskTexture2.Sample(textureSampler, uv).r;
    vec3 mask = maskTexture.Sample(textureSampler, tiledUV).xyz;

    vec3 dirt2 = dirtTexture.Sample(textureSampler, p2.uvOut).xyz;
    vec3 diffuse2 = diffuseTexture2.Sample(textureSampler, p2.uvOut).xyz;

    vec3 albedo2 = dirt2 * diffuse2 * mask1 * 0.75;
    float albedoMix = 1.0 - mask2;

    public vec3 albedo = mix(clamp(t0 * t1 * 2.0, 0.0, 1.0), albedo2, albedoMix);

    float v = mask.g * mask.g * t1.r;
    public float specular = mix(v * v, diffuse2.r, albedoMix);
    public float roughness = mix(0.5, 0.05, mask.r * mask2);
    public vec3 normal
    {
        vec3 rs = mix(normalTexture.Sample(textureSampler, tiledUV).xyz,
            normalTexture2.Sample(textureSampler, p2.uvOut).xyz, albedoMix) * 2.0 - 1.0;
        rs.y = -rs.y;
        return rs;
    }
}
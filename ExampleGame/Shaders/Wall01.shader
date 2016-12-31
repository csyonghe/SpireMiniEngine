module Wall01Pattern implements IMaterialPattern
{
    param Texture2D maskTexture;
    param Texture2D dirtTexture;
    param Texture2D diffuseTexture;
    param Texture2D normalTexture;
    require SamplerState textureSampler;
    require vec3 vTangent;
    require vec3 vBiTangent;
    require vec3 pos;
    
    vec2 uv = vec2(dot(vTangent, pos), dot(vBiTangent, pos)) * 0.002;

    vec2 tiledUV = uv * 4.0;
    vec3 t0 = diffuseTexture.Sample(textureSampler, tiledUV).xyz;
    vec3 t1 = dirtTexture.Sample(textureSampler, uv).xyz;
    public vec3 albedo = clamp(t0 * t1 * 2.0, 0.0, 1.0);

    vec3 mask = maskTexture.Sample(textureSampler, tiledUV).xyz;

    float v = mask.g * mask.g * t1.r;
    public float specular = v * v;
    public float roughness = mix(0.5, 0.05, mask.r);
    public vec3 normal
    {
        vec3 rs = normalTexture.Sample(textureSampler, tiledUV).xyz * 2.0 - 1.0;
        rs.y = -rs.y;
        return rs;
    }
}
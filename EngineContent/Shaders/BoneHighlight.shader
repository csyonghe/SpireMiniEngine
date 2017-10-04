[Transparent]
module BoneHighlightPattern implements IMaterialPattern
{
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 0.7;
    public float metallic = 0.4;
    public float specular = 0.2;
    param vec3 highlightColor;
    param float alpha;
    require float time;
    param int highlightId;
    require uint boneIds;
    require vec4 boneWeights;
    public vec3 albedo
    {
        return highlightColor;
    }
    public float opacity
    {
        float result = 0.0;
        for (int i = 0; i < 4; i++)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            float boneWeight = boneWeights[i];
            if (boneId == 255) continue;
            if (boneId == highlightId)
            {
                result = mix(0.4, 1.0, (sin(time * 4.0)*0.5 + 0.5)) * alpha * boneWeight;
                break;
            }   
        }
        return result;
    } 
    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}

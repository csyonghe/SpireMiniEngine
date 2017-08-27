[Transparent]
module SkeletonVisualizePattern implements IMaterialPattern
{
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 0.7;
    public float metallic = 0.4;
    public float specular = 0.2;
    param vec3 solidColor;
    param vec3 highlightColor;
    param float alpha;
    param int highlightId;
    require uint boneIds;
    public vec3 albedo
    {
        vec3 result = solidColor;
        for (int i = 0; i < 4; i++)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;
            if (boneId == highlightId)
                result = highlightColor;
        }
        return result;
    }
    public float opacity
    {
        float result = alpha;
        for (int i = 0; i < 4; i++)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;
            if (boneId == highlightId)
            {
                result += 0.2;
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

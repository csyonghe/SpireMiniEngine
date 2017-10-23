[Transparent]
module GizmoPattern implements IMaterialPattern
{
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 1.0;
    public float metallic = 0.0;
    public float specular = 0.0;
    param vec3 solidColor;
    param float alpha;
    public vec3 albedo
    {
        return solidColor;
    }
    public float opacity
    {
        return alpha;
    } 
    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}

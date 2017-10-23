module ReflectivePattern implements IMaterialPattern
{
    require vec2 vertUV;
    
    public vec3 albedo = vec3(1.0);
    public float metallic = 1.0;
    public float specular = 1.0;
    public float roughness = 0.1;
}


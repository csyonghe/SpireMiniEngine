module ToneMappingPassParams
{
    public param vec4 hdrExposure;
    public param Texture2D litColor;
    public param SamplerState nearestSampler;
}
shader ToneMappingPostPass targets StandardPipeline
{
	public @MeshVertex vec2 vertPos;
	public @MeshVertex vec2 vertUV;

    [Binding: "0"]
    public using ToneMappingPassParams;
    
    vec3 hdr(vec3 L)
    {
        L = L * hdrExposure.x;
        L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
        L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
        L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
        return L;
        //return L / (L + 0.427) * 1.0535;
    }
    
    public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

    public out @Fragment vec4 outputColor
    {
        vec3 src = litColor.Sample(nearestSampler, vertUV).xyz;
        return vec4(hdr(src), 1.0);
    }
}
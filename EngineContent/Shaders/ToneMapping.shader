module ToneMappingPassParams
{
    public param vec4 hdrExposure;
    public param Texture2D litColor;
    public param Texture3D colorLUT;
    public param SamplerState nearestSampler;
    public param SamplerState linearSampler;
}
shader ToneMappingPostPass targets StandardPipeline
{
	public @MeshVertex vec2 vertPos;
	public @MeshVertex vec2 vertUV;

    [Binding: "0"]
    public using ToneMappingPassParams;
    /*
    vec3 hdr(vec3 L)
    {
        L = L * hdrExposure.x;
        L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
        L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
        L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
        return L;
        //return L / (L + 0.427) * 1.0535;
    }
    */

    mat3 ACESInputMat = mat3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );

    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    mat3 ACESOutputMat = mat3(
        1.60475, -0.53108, -0.07367,
        -0.10208,  1.10813, -0.00605,
        -0.00327, -0.07276,  1.07602
    );

    float3 RRTAndODTFit(float3 v)
    {
        float3 a = v * (v + 0.0245786) - 0.000090537;
        float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
        return a / b;
    }

    float3 transform(mat3 m, float3 v)
    {
        return float3(dot(m.x, v), dot(m.y, v), dot(m.z, v));
    }

    float3 ACESFitted(float3 color)
    {
        color = transform(ACESInputMat, color);

        // Apply RRT and ODT
        color = RRTAndODTFit(color);

        color = transform(ACESOutputMat, color);

        // Clamp to [0, 1]
        color = clamp(color, vec3(0.0), vec3(1.0));

        return color;
    }

    public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

    public out @Fragment vec4 outputColor
    {
        vec3 src = litColor.Sample(nearestSampler, vertUV).xyz;
        vec3 ldr = ACESFitted(src * hdrExposure.x);
        vec3 mapped = colorLUT.Sample(linearSampler, ldr).xyz;
        return vec4(mapped, 1.0);
    }
}
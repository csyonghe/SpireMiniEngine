module OutlinePassParams
{
    public param vec4 borderColor;
    public param vec2 pixelSize;
    public param float width;
    public param Texture2D srcColor;
    public param Texture2D srcDepth;
    public param SamplerState nearestSampler;
}
shader OutlinePostPass targets StandardPipeline
{
	public @MeshVertex vec2 vertPos;
	public @MeshVertex vec2 vertUV;

    [Binding: "0"]
    public using OutlinePassParams;
        
    public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

    int getVal(vec2 uv)
    {
        float val = srcDepth.Sample(nearestSampler, uv).x;
        return val == 1.0 ? 1 : 0;
    }
    public out @Fragment vec4 outputColor
    {
        vec4 src = srcColor.Sample(nearestSampler, vertUV);
        int thisVal = getVal(vertUV);
        vec4 bColor = vec4(0.0);
        if (thisVal)
        {
            if (getVal(vertUV - vec2(pixelSize.x, 0.0) * width) == 0 ||
                getVal(vertUV - vec2(0.0, pixelSize.y) * width) == 0 ||
                getVal(vertUV + vec2(pixelSize.x, 0.0) * width) == 0 ||
                getVal(vertUV + vec2(0.0, pixelSize.y) * width) == 0)
                bColor = borderColor;
        }
        return src * (1-bColor.w) + bColor * bColor.w;
    }
}
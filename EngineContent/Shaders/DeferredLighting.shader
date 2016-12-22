shader DeferredLighting : StandardPipeline
{
	param Texture2D albedoTex;
	param Texture2D pbrTex;
	param Texture2D normalTex;
	param Texture2D depthTex;
	param SamplerState nearestSampler;

	[Binding: "1"]
	public using ForwardBasePassParams;

	public @MeshVertex vec2 vertPos;
	public @MeshVertex vec2 vertUV;

	public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

	public vec3 normal = normalTex.Sample(nearestSampler, vertUV).xyz * 2.0 - 1.0;
	public vec3 pbr = pbrTex.Sample(nearestSampler, vertUV).xyz;
	public float roughness = pbr.x;
	public float metallic = pbr.y;
	public float specular = pbr.z;
	public vec3 albedo = albedoTex.Sample(nearestSampler, vertUV).xyz;
	public float selfShadow(vec3 x) { return 1.0; }
    vec3 lightParam = vec3(roughness, metallic, specular);
	float z = depthTex.Sample(nearestSampler, vertUV).r;
    float x = vertUV.x*2-1;
    float y = vertUV.y*2-1;
	vec4 position = invViewProjTransform * vec4(x, y, z, 1.0f);
	vec3 pos = position.xyz / position.w;
	using lighting = Lighting();

    public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
}

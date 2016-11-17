shader DeferredLighting
{
	public @MeshVertex vec2 vertPos;
	public @MeshVertex vec2 vertUV;
	public using SystemUniforms;

	@MaterialUniform Texture2D albedoTex;
	@MaterialUniform Texture2D pbrTex;
	@MaterialUniform Texture2D normalTex;
	@MaterialUniform Texture2D depthTex;
	@MaterialUniform SamplerState textureSampler;

    public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

	public vec3 normal = normalTex.Sample(textureSampler, vertUV).xyz;
	public vec3 pbr = pbrTex.Sample(textureSampler, vertUV);
	public float roughness = pbr.x;
	public float metallic = pbr.y;
	public float specular = pbr.z;
	public vec3 albedo = albedoTex.Sample(textureSampler, vertUV).xyz;

    vec3 lightParam = vec3(roughness, metallic, specular);
	float z = texture(depthTex, vertUV).r*2-1;
    float x = vertUV.x*2-1;
    float y = vertUV.y*2-1;
	vec4 position = invViewProjTransform * vec4(x, y, -z, 1.0f);
	vec3 pos = position.xyz / position.w;
	using lighting = Lighting();

    public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
    //public out @fs vec4 outputColor = vec4(pos, 1.0);
}

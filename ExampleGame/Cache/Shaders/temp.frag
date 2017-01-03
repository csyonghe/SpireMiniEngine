#version 440
struct SkinningResult
{
vec3 pos;
vec3 tangent;
vec3 binormal;
};
layout(set = 0, binding = 1) uniform sampler2D DeferredLightingParams_albedoTex;
layout(set = 0, binding = 2) uniform sampler2D DeferredLightingParams_pbrTex;
layout(set = 0, binding = 3) uniform sampler2D DeferredLightingParams_normalTex;
layout(set = 0, binding = 4) uniform sampler2D DeferredLightingParams_depthTex;
layout(std140, set = 1, binding = 0) uniform bufForwardBasePassParams
{
mat4 viewTransform;
mat4 viewProjectionTransform;
mat4 invViewTransform;
mat4 invViewProjTransform;
vec3 cameraPos;
float time;
} ForwardBasePassParams;
layout(std140, set = 2, binding = 0) uniform bufLighting
{
vec3 lightDir;
vec3 lightColor;
int shadowMapId;
int numCascades;
mat4[8] lightMatrix;
vec4[2] zPlanes;
} Lighting;
layout(set = 2, binding = 1) uniform sampler2DArrayShadow Lighting_shadowMapArray;
float DeferredLighting_selfShadow_vec3(vec3 p0_x);
float Lighting_Pow4_float(float p0_x);
vec2 Lighting_LightingFuncGGX_FV_float_float(float p0_dotLH, float p1_roughness);
float Lighting_LightingFuncGGX_D_float_float(float p0_dotNH, float p1_roughness);
float DeferredLighting_selfShadow_vec3(vec3 p0_x)
{
return 1.000000000000e+00;
}
float Lighting_Pow4_float(float p0_x)
{
return ((p0_x * p0_x) * (p0_x * p0_x));
}
vec2 Lighting_LightingFuncGGX_FV_float_float(float p0_dotLH, float p1_roughness)
{
float alpha;
float F_a;
float F_b;
float dotLH5;
float vis;
float k;
float k2;
float invK2;
alpha = (p1_roughness * p1_roughness);
dotLH5 = (Lighting_Pow4_float((1.000000000000e+00 - p0_dotLH)) * (1.000000000000e+00 - p0_dotLH));
F_a = 1.000000000000e+00;
F_b = dotLH5;
k = (alpha / 2.000000000000e+00);
k2 = (k * k);
invK2 = (1.000000000000e+00 - k2);
vis = (1.000000000000e+00 / (((p0_dotLH * p0_dotLH) * invK2) + k2));
return vec2((F_a * vis), (F_b * vis));
}
float Lighting_LightingFuncGGX_D_float_float(float p0_dotNH, float p1_roughness)
{
float alpha;
float alphaSqr;
float pi;
float denom;
float D;
alpha = (p1_roughness * p1_roughness);
alphaSqr = (alpha * alpha);
pi = 3.141590118408e+00;
denom = (((p0_dotNH * p0_dotNH) * (alphaSqr - 1.000000000000e+00)) + 1.000000000000e+00);
D = (alphaSqr / ((pi * denom) * denom));
return D;
}
in vec2 vertUV_CoarseVertex;
out vec4 outputColor_Fragment;
void main()
{
vec2 vertUV;
vec3 normal;
vec3 pbr;
vec3 albedo;
vec3 lightParam;
vec4 t3B;
vec4 position;
vec3 pos;
float roughness_in;
float metallic_in;
vec3 H;
float shadow;
vec3 viewPos;
int i = 0;
int t54 = 0;
vec4 t55;
int t56 = 0;
vec4 lightSpacePosT;
vec3 lightSpacePos;
float val;
float D;
vec2 FV_helper;
float FV;
float highlight_GGXstandard;
vec4 outputColor;
vertUV = vertUV_CoarseVertex;
normal = ((texture(DeferredLightingParams_normalTex, vertUV).xyz * 2.000000000000e+00) - 1.000000000000e+00);
pbr = texture(DeferredLightingParams_pbrTex, vertUV).xyz;
albedo = texture(DeferredLightingParams_albedoTex, vertUV).xyz;
lightParam = vec3(pbr.x, pbr.y, pbr.z);
t3B = texture(DeferredLightingParams_depthTex, vertUV);
position = (ForwardBasePassParams.invViewProjTransform * vec4(((vertUV.x * 2) - 1), ((vertUV.y * 2) - 1), t3B.x, 1.000000000000e+00));
pos = (position.xyz / position.w);
roughness_in = lightParam.x;
metallic_in = lightParam.y;
H = normalize((normalize((ForwardBasePassParams.cameraPos - pos)) + Lighting.lightDir));
shadow = DeferredLighting_selfShadow_vec3(Lighting.lightDir);
if (bool(Lighting.numCascades))
{
viewPos = (ForwardBasePassParams.viewTransform * vec4(pos, 1.000000000000e+00)).xyz;
i = 0;
for (; (i < Lighting.numCascades); (i = (i + 1)))
{
t54 = (i >> 2);
t55 = Lighting.zPlanes[t54];
t56 = (i & 3);
if (bool(((-viewPos.z) < t55[t56])))
{
lightSpacePosT = (Lighting.lightMatrix[i] * vec4(pos, 1.000000000000e+00));
lightSpacePos = (lightSpacePosT.xyz / lightSpacePosT.w);
val = texture(Lighting_shadowMapArray, vec4(vec3(lightSpacePos.xy, (i + Lighting.shadowMapId)), lightSpacePos.z));
shadow = (shadow * val);
break;
}
}
}
D = Lighting_LightingFuncGGX_D_float_float(clamp(dot(normal, H), 9.999999776483e-03, 9.900000095367e-01), roughness_in);
FV_helper = Lighting_LightingFuncGGX_FV_float_float(clamp(dot(Lighting.lightDir, H), 9.999999776483e-03, 9.900000095367e-01), roughness_in);
FV = ((metallic_in * FV_helper.x) + ((1.000000000000e+00 - metallic_in) * FV_helper.y));
highlight_GGXstandard = (((clamp(dot(normal, Lighting.lightDir), 9.999999776483e-03, 9.900000095367e-01) * D) * FV) * lightParam.z);
outputColor = vec4((Lighting.lightColor * (((albedo * ((clamp(dot(Lighting.lightDir, normal), 0.000000000000e+00, 1.000000000000e+00) * shadow) + 4.000000059605e-01)) * (1.000000000000e+00 - metallic_in)) + (mix(albedo, vec3(1.000000000000e+00), (1.000000000000e+00 - metallic_in)) * (highlight_GGXstandard * shadow)))), 1.000000000000e+00);
outputColor_Fragment = outputColor;
}
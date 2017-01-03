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
layout(location = 0) in vec2 vertPos_MeshVertex;
layout(location = 1) in vec2 vertUV_MeshVertex;
out vec2 vertUV_CoarseVertex;
void main()
{
vec2 vertPos;
vec2 vertUV;
vertPos = vertPos_MeshVertex;
vertUV = vertUV_MeshVertex;
vertUV_CoarseVertex = vertUV;
gl_Position = vec4(vertPos.xy, 0.000000000000e+00, 1.000000000000e+00);
}
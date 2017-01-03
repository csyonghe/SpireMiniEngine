#version 440
layout(std140, set = 0, binding = 0) uniform bufUberUIShaderParams
{
mat4 orthoMatrix;
} UberUIShaderParams;
layout(std430, set = 0, binding = 1) buffer bufUberUIShaderParams_uniformInput
{
uvec4 UberUIShaderParams_uniformInput[];
};
layout(std430, set = 0, binding = 2) buffer bufUberUIShaderParams_textContent
{
uint UberUIShaderParams_textContent[];
};
in vec2 vert_pos_rootVert;
in vec2 vert_uv_rootVert;
in int vert_primId_rootVert;
flat out int vert_primId_vs;
out vec2 vert_pos_vs;
out vec2 vert_uv_vs;
void main()
{
vec2 vert_pos;
int vert_primId = 0;
vec2 vert_uv;
vert_pos = vert_pos_rootVert;
vert_pos_vs = vert_pos;
vert_primId = vert_primId_rootVert;
vert_primId_vs = vert_primId;
vert_uv = vert_uv_rootVert;
vert_uv_vs = vert_uv;
gl_Position = (UberUIShaderParams.orthoMatrix * vec4(vert_pos, 0.000000000000e+00, 1.000000000000e+00));
}
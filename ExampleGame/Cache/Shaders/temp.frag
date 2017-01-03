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
vec4 erf(vec4 p_x);
float boxShadow(vec2 p_lower, vec2 p_upper, vec2 p_point, float p_sigma);
vec4 erf(vec4 p_x)
{
vec4 s;
vec4 a;
s = sign(p_x);
a = abs(p_x);
p_x = (1.000000000000e+00 + ((2.783930003643e-01 + ((2.303889989853e-01 + (7.810799777508e-02 * (a * a))) * a)) * a));
p_x = (p_x * p_x);
return (s - (s / (p_x * p_x)));
}
float boxShadow(vec2 p_lower, vec2 p_upper, vec2 p_point, float p_sigma)
{
vec4 query;
vec4 integral;
query = vec4((p_point - p_lower), (p_point - p_upper));
integral = (5.000000000000e-01 + (5.000000000000e-01 * erf((query * (sqrt(5.000000000000e-01) / p_sigma)))));
return ((integral.z - integral.x) * (integral.w - integral.y));
}
flat in int vert_primId_vs;
in vec2 vert_pos_vs;
in vec2 vert_uv_vs;
out vec4 color_fs;
void main()
{
int vert_primId = 0;
vec2 vert_pos;
vec2 vert_uv;
vec4 color;
uvec4 params0;
int t3A = 0;
uvec4 params1;
int t3F = 0;
int clipBoundX = 0;
int clipBoundY = 0;
int clipBoundX1 = 0;
int clipBoundY1 = 0;
int shaderType = 0;
uint pColor = 0;
vec4 inputColor;
int textWidth = 0;
int textHeight = 0;
int startAddr = 0;
ivec2 fetchPos;
int relAddr = 0;
int ptr = 0;
uint word = 0;
int tA0 = 0;
uint ptrMod = 0;
int bitPtr = 0;
float alpha;
vec2 origin;
vec2 size;
float shadowSize;
float shadow;
vert_primId = vert_primId_vs;
vert_pos = vert_pos_vs;
vert_uv = vert_uv_vs;
color = vec4(0.000000000000e+00);
t3A = (int(vert_primId) * 2);
params0 = UberUIShaderParams_uniformInput[t3A];
t3F = ((int(vert_primId) * 2) + 1);
params1 = UberUIShaderParams_uniformInput[t3F];
clipBoundX = int((params0.x & 65535));
clipBoundY = int((params0.x >> 16));
clipBoundX1 = int((params0.y & 65535));
clipBoundY1 = int((params0.y >> 16));
shaderType = int(params0.z);
pColor = params0.w;
inputColor = (vec4(float((pColor & 255)), float(((pColor >> 8) & 255)), float(((pColor >> 16) & 255)), float(((pColor >> 24) & 255))) * (1.000000000000e+00 / 2.550000000000e+02));
if (bool((shaderType == 0)))
{
if (bool((vert_pos.x < clipBoundX)))
{
discard;
}
if (bool((vert_pos.y < clipBoundY)))
{
discard;
}
if (bool((vert_pos.x > clipBoundX1)))
{
discard;
}
if (bool((vert_pos.y > clipBoundY1)))
{
discard;
}
color = inputColor;
}
else
{
if (bool((shaderType == 1)))
{
if (bool((vert_pos.x < clipBoundX)))
{
discard;
}
if (bool((vert_pos.y < clipBoundY)))
{
discard;
}
if (bool((vert_pos.x > clipBoundX1)))
{
discard;
}
if (bool((vert_pos.y > clipBoundY1)))
{
discard;
}
textWidth = int(params1.x);
textHeight = int(params1.y);
startAddr = int(params1.z);
fetchPos = ivec2((vert_uv * ivec2(textWidth, textHeight)));
relAddr = ((fetchPos.y * textWidth) + fetchPos.x);
ptr = (startAddr + (relAddr >> 1));
tA0 = (ptr >> 2);
word = UberUIShaderParams_textContent[tA0];
ptrMod = (ptr & 3);
word = (word >> (ptrMod << 3));
bitPtr = (relAddr & 1);
word = (word >> (bitPtr << 2));
alpha = (float((word & 15)) * (1.000000000000e+00 / 1.500000000000e+01));
color = inputColor;
color[3] = (color.w * alpha);
}
else
{
if (bool((shaderType == 2)))
{
if (bool(((((vert_pos.x > clipBoundX) && (vert_pos.x < clipBoundX1)) && (vert_pos.y > clipBoundY)) && (vert_pos.y < clipBoundY1))))
{
discard;
}
origin[0] = float((params1.x & 65535));
origin[1] = float((params1.x >> 16));
size[0] = float((params1.y & 65535));
size[1] = float((params1.y >> 16));
shadowSize = float(params1.z);
shadow = boxShadow(origin, (origin + size), vert_pos, shadowSize);
color = vec4(inputColor.xyz, (inputColor.w * shadow));
}
else
{
discard;
}
}
}
color_fs = color;
color_fs = color;
}
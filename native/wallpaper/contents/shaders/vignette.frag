#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float falloff;
    float brightness;
    float viewWidth;
    float viewHeight;
};
layout(binding = 1) uniform sampler2D source;

void main()
{
    const vec2 rtSize = vec2(640.0, 360.0);
    float ar = viewWidth / max(1.0, viewHeight);
    float rar = rtSize.x / rtSize.y;
    vec2 cover = vec2(ar < rar ? ar / rar : 1.0, ar > rar ? rar / ar : 1.0);
    vec2 uv = (qt_TexCoord0 - 0.5) * cover + 0.5;
    vec2 px = (floor(uv * rtSize) + 0.5) / rtSize;
    vec3 col = texture(source, vec2(px.x, 1.0 - px.y)).rgb;

    vec2 c = (px - 0.5) * vec2(rtSize.x / rtSize.y, 1.0) * 2.0;
    float rf = length(c) * falloff;
    float rf2 = rf * rf + 1.0;
    col *= 1.0 / (rf2 * rf2);

    vec3 lin = mix(col / 12.92, pow((col + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), col));
    lin *= 1.0 + 3.0 * brightness;
    col = mix(lin * 12.92, 1.055 * pow(lin, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), lin));
    fragColor = vec4(col, 1.0) * qt_Opacity;
}

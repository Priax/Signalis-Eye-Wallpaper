VARYING vec2 vUv;
VARYING vec3 vWorld;
VARYING vec3 vNormal;


void MAIN()
{
    vec3 L = lightPos - vWorld;
    float d2 = dot(L, L) / (range * range);
    float atten = (1.0 / (1.0 + 25.0 * d2)) * clamp((1.0 - d2) * 5.0, 0.0, 1.0);
    float ndl = max(0.0, dot(normalize(vNormal), normalize(L)));
    vec3 c = texture(map, vUv).rgb * diffuse * intensity * atten * ndl;
    FRAGCOLOR = vec4(c, 1.0);
}

VARYING vec2 vUv;

void MAIN()
{
    vec4 c = texture(map, vec2(vUv.x, mix(vMin, 1.0, vUv.y)));
    FRAGCOLOR = vec4(c.rgb * c.a, 1.0);
}

VARYING vec2 vUv;
VARYING vec3 vWorld;

void MAIN()
{
    vUv = UV0;
    vec3 pos = VERTEX;
    if (sway > 0.0) {
        float worldZ = (MODEL_MATRIX * vec4(VERTEX, 1.0)).z;
        pos.x += sin(worldZ) * cos(time) * UV0.y * sway;
    }
    vWorld = (MODEL_MATRIX * vec4(pos, 1.0)).xyz;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}

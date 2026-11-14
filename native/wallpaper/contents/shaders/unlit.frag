VARYING vec2 vUv;
VARYING vec3 vWorld;

bool behindMask()
{
    vec3 o = CAMERA_POSITION;
    vec3 d = vWorld - o;
    float len = length(d);
    d /= len;
    vec3 oc = o - holeCenter;
    float b = dot(oc, d);
    float disc = b * b - (dot(oc, oc) - holeRadius * holeRadius);
    if (disc < 0.0)
        return false;
    float t = -b - sqrt(disc);
    if (t <= 0.0 || t >= len - 0.01)
        return false;
    return dot(normalize(o + t * d - holeCenter), holeDir) > holeCos;
}

void MAIN()
{
    vec4 c = texture(map, vUv);
    if (cutoff > 0.0 && c.a < cutoff)
        discard;
    if (holeMode > 0.0 && behindMask()) {
        if (holeMode < 1.5)
            discard;
        c = vec4(0.0, 0.0, 0.0, 1.0);
    }
    FRAGCOLOR = vec4(c.rgb, blended > 0.5 ? c.a : 1.0);
}

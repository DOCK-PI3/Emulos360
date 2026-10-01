VARYING vec2 avatarUv0;
VARYING vec2 avatarUv1;
VARYING vec2 avatarUv2;
VARYING vec2 avatarUv3;
VARYING vec2 avatarUv4;
VARYING vec2 avatarUv5;
vec2 avatarUv(int channel) {
    if (channel == 0) return avatarUv0;
    if (channel == 1) return avatarUv1;
    if (channel == 2) return avatarUv2;
    if (channel == 3) return avatarUv3;
    if (channel == 4) return avatarUv4;
    return avatarUv5;
}
vec3 faceLayer(vec4 texel, vec3 primary, vec3 secondary, vec3 skin) {
    return texel.r * primary + texel.g * secondary + texel.b * skin;
}
void MAIN() {
    vec4 a = texture(avatarMap0, avatarUv(channel0));
    vec4 b = texture(avatarMap1, avatarUv(channel1));
    vec4 c = texture(avatarMap2, avatarUv(channel2));
    vec3 rgb;
    float alpha = 1.0;
    if (isHead) {
        vec4 eye = texture(avatarMap3, avatarUv(channel3));
        vec4 mouth = texture(avatarMap4, avatarUv(channel4));
        vec4 shadow = texture(avatarMap5, avatarUv(channel5));
        rgb = mix(skinTone.rgb, faceLayer(a, featureOne.rgb, featureTwo.rgb, skinTone.rgb), a.a);
        rgb = mix(rgb, shadow.r * eyeShadowTone.rgb, shadow.a);
        rgb = mix(rgb, faceLayer(mouth, mouthTone.rgb, vec3(1.0), skinTone.rgb), mouth.a);
        rgb = mix(rgb, faceLayer(eye, irisTone.rgb, vec3(1.0), skinTone.rgb), eye.a);
        rgb = mix(rgb, faceLayer(b, facialHairTone.rgb, vec3(1.0), skinTone.rgb), b.a);
        rgb = mix(rgb, faceLayer(c, eyebrowTone.rgb, vec3(1.0), skinTone.rgb), c.a);
    } else {
        rgb = mix(a.rgb, b.r * customOne.rgb + b.g * customTwo.rgb + b.b * customThree.rgb, b.a);
        rgb = mix(rgb, c.rgb, c.a);
        alpha = a.a;
        if (alpha < 0.1) discard;
    }
    BASE_COLOR = vec4(rgb, alpha);
    ROUGHNESS = 0.8;
    SPECULAR_AMOUNT = 0.18;
}

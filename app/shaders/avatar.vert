VARYING vec2 avatarUv0;
VARYING vec2 avatarUv1;
VARYING vec2 avatarUv2;
VARYING vec2 avatarUv3;
VARYING vec2 avatarUv4;
VARYING vec2 avatarUv5;
void MAIN() {
    avatarUv0 = UV0;
    avatarUv1 = UV1;
    avatarUv2 = COLOR.xy;
    avatarUv3 = COLOR.zw;
    avatarUv4 = TANGENT.xy;
    avatarUv5 = BINORMAL.xy;
    COLOR = vec4(1.0);
}

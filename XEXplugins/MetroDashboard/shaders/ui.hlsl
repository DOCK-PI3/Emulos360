struct Input { float2 position : POSITION; float4 color : COLOR0; float2 uv : TEXCOORD0; };
struct Output { float4 position : POSITION; float4 color : COLOR0; float2 uv : TEXCOORD0; };
Output vs(Input i) { Output o; o.position=float4(i.position,0,1); o.color=i.color; o.uv=i.uv; return o; }
sampler2D atlas : register(s0);
float4 solid(float4 color : COLOR0) : COLOR0 { return color; }
float4 textured(Output i) : COLOR0 { return tex2D(atlas,i.uv)*i.color; }

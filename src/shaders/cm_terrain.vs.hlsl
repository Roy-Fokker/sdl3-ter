struct Input
{
	// Position is using TEXCOORD semantic because of rules imposed by SDL
	// Per https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
	float3 Position : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
};

struct Output
{
	float2 TexCoord : TEXCOORD0;
	float4 Position : SV_Position;
};

struct Push_Data
{
	float4x4 projection;
	float4x4 view;
};
ConstantBuffer<Push_Data> ubo : register(b0, space1);

// space0 is because of reason explained in
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

Output main(Input input)
{
	Output output;
	output.TexCoord = input.TexCoord;

	float4 clr = Texture.SampleLevel(Sampler, input.TexCoord, 0.0f);
	input.Position.y = clr.r * 128.f - 64.f;

	float4 pos = float4(input.Position, 1.0f);

	output.Position = mul(ubo.projection, mul(ubo.view, pos));

	return output;
}

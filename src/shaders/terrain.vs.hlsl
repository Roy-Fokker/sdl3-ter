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

Output main(Input input)
{
	Output output;
	output.TexCoord = input.TexCoord;

	float4 pos = float4(input.Position, 1.0f);

	output.Position = mul(ubo.projection, mul(ubo.view, pos));

	return output;
}

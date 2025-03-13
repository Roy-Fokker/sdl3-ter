// space2 is because of reason explained in
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
// Texture2D<float4> Texture : register(t0, space2);
// SamplerState Sampler : register(s0, space2);

struct Input
{
	float2 TexCoord : TEXCOORD0;
	float3 baryWeights : SV_Barycentrics;
};

float4 main(Input input) : SV_Target0
{
	// return Texture.Sample(Sampler, input.TexCoord);

	float3 dbcoordX = ddx(input.baryWeights);
	float3 dbcoordY = ddy(input.baryWeights);

	float3 dbcoord = sqrt(dbcoordX * dbcoordX + dbcoordY * dbcoordY);
	float thickness = 1.5f;

	float3 remap = smoothstep(float3(0.f, 0.f, 0.f), dbcoord * thickness, input.baryWeights);

	float wireframe = min(remap.x, min(remap.y, remap.z));

	return float4(wireframe.xxx, 1.f);
}

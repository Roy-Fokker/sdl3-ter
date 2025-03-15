
Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

struct Vertex
{
	float3 Position; 
	float2 TexCoord;
};

RWStructuredBuffer<Vertex> vertices : register(u0, space1);

[numthreads(16, 16, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{

}
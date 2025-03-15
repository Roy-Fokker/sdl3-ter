
Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

struct Vertex
{
	float3 Position; 
	float2 TexCoord;
};

RWByteAddressBuffer vertices : register(u0, space1);

[numthreads(1024, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
	const int Vertex_Size = 20;

	float w;
	float h;
	Texture.GetDimensions(w, h);

	int vidx = thread_id.x;
	int vbyteoffset = vidx * Vertex_Size;

	int pos_x_offset = vbyteoffset;
	int pos_y_offset = vbyteoffset + 4;
	int pos_z_offset = vbyteoffset + 8;
	int uv_offset = vbyteoffset + 12;

	float u = asfloat(vertices.Load(pos_x_offset));
	u = (u + 512.5) / w;
	float v = asfloat(vertices.Load(pos_z_offset));
	v = (v + 512.5) / w;

	float4 clr = Texture.SampleLevel(Sampler, float2(u, v), 0.0f);

	vertices.Store(pos_y_offset, asuint(clr.r * 50.f));
	vertices.Store2(uv_offset, uint2(asuint(u), asuint(v)));
}
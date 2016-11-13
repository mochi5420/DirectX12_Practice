struct VS_OUT {
	float4	position	: SV_POSITION;
	float4	color		: COLOR;
};

cbuffer cb : register(b0)
{
	float4x4 g_mW : packoffset(c0);


}

VS_OUT VSMain(float3 position : POSITION, float4 color : COLOR)
{
	VS_OUT output = (VS_OUT)0;

	float4 pos = float4(position, 1.0f);
	output.position = mul(g_mW, pos);
	output.color = color;

	return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
	return input.color;
}
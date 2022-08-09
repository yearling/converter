matrix g_view;
matrix g_projection;
matrix g_VP;
matrix g_InvVP;
float3 g_lightDir;
float  g_pad;

struct VS_INPUT
{
	float3 vPosition	: position;
	float4 vColor		: color;
};
struct VS_OUTPUT
{
	float4 vPosition	: SV_POSITION;
	float4 vColor		: COLOR0;
};

VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	matrix vp= mul(g_view,g_projection);
	Output.vPosition = mul(float4(Input.vPosition,1.0), vp);
	Output.vPosition.z =Output.vPosition.z - Output.vPosition.w* 0.00001;
	Output.vColor = Input.vColor;
	return Output;
}

float4 PSMain(VS_OUTPUT Input) :SV_Target
{
	return Input.vColor;
}
	matrix g_view;
	matrix g_projection;
	matrix g_VP;
	matrix g_InvVP;
	float3 g_lightDir;

	matrix g_world;


struct VS_INPUT
{
	float3    vPosition		: position;
	float3    vNormal       : normal;
	float2    vTexCoord     : uv;
	// half3     TangentX		: ATTRIBUTE1;
	// half4     TangentZ		: ATTRIBUTE2;
    // half2     TexCoords[2]  : ATTRIBUTE3;
};
struct VS_OUTPUT
{
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosition	: SV_POSITION;
	float4 vColor		: COLOR0;
	float3 vNormal      : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	matrix vp= mul(g_view,g_projection);
	Output.vPosition = mul(float4(Input.vPosition,1.0), vp);
	Output.vTexcoord = Input.vTexCoord;
	Output.vColor = float4(1.0,1.0,1.0,1.0);
	Output.vNormal = Input.vNormal;
	// matrix matWVP = mul(g_world, g_VP);

	// Output.vPosition = mul(float4(Input.vPosition,1.0), matWVP);
	// Output.vTexcoord[0] = Input.TexCoords[0];
	// Output.vTexcoord[1] = Input.TexCoords[1];

	// float3 TangentX = normalize(Input.TangentX.xyz);
	// float3 TangentZ = normalize(Input.TangentZ.xyz);
	// float3 TangentY = cross(TangentZ, TangentX)* Input.TangentZ.w;
	// Output.TangentToLocal = float3x3(TangentX, TangentY, TangentZ);
	return Output;
}


// Texture2D txDiffuse;
// Texture2D txNormal;
// SamplerState samLinear;

// float3 sRGBToLinear(float3 Color)
// {
// 	Color = max(6.10352e-5, Color); // minimum positive non-denormal (fixes black problem on DX11 AMD and NV)
// 	return Color > 0.04045 ? pow(Color * (1.0 / 1.055) + 0.0521327, 2.4) : Color * (1.0 / 12.92);
// }

// float LinearToSrgbBranchingChannel(float lin)
// {
// 	if (lin < 0.00313067)
// 		return lin * 12.92;
// 	return pow(lin, (1.0 / 2.4)) * 1.055 - 0.055;
// }

// float3 LinearToSrgbBranching(float3 lin)
// {
// 	return half3(
// 		LinearToSrgbBranchingChannel(lin.r),
// 		LinearToSrgbBranchingChannel(lin.g),
// 		LinearToSrgbBranchingChannel(lin.b));
// }
float4 PSMain(VS_OUTPUT Input) :SV_Target
{
	float3 dir_light = float3(1.0,1.0,-1.0);
	dir_light = normalize(dir_light);
	float ndl = dot(dir_light,normalize(Input.vNormal));
	ndl = clamp(ndl,0,1.0);
	return float4(ndl,ndl,ndl,1.0);	
}
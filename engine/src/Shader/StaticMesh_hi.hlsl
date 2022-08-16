
	matrix g_world;
	matrix g_view;
	matrix g_projection;
	matrix g_VP;
	matrix g_InvVP;
	float3 light_dir;
	float show_normal;
    Texture2D g_MeshTexture;
    SamplerState g_sampler; 
struct VS_INPUT
{
	float3    vPosition		: position;
	float4    vNormal       : normal;
	float4    vTangent       : tangent;
	float2    vTexCoord     : uv;
	float2    vTexCoord1     : uv1;
	float4    vColor        :color;
};

struct VS_OUTPUT
{
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosition	: SV_POSITION;
	float4 vColor		: COLOR0;
	float3 vNormal      : NORMAL;
	float4 vTangent      : Tangent;
};

VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	matrix vp= mul(g_view,g_projection);
	matrix wvp = mul (g_world,vp);
	Output.vPosition = mul(float4(Input.vPosition,1.0), wvp);

	Output.vTexcoord = Input.vTexCoord;
	Output.vColor = Input.vColor;
	float4 trans_normal = mul(float4(Input.vNormal.xyz,0.0), g_world);
	Output.vNormal = trans_normal.xyz;
	Output.vNormal = normalize(Output.vNormal);
	Output.vTangent.xyz = normalize(mul(float4(Input.vTangent.xyz,0.0), g_world).xyz);
	Output.vTangent.w =  Input.vTangent.w;
	return Output;
}


// Texture2D txDiffuse;
// Texture2D txNormal;
// SamplerState samLinear;

float3 sRGBToLinear(float3 Color)
{
	Color = max(6.10352e-5, Color); // minimum positive non-denormal (fixes black problem on DX11 AMD and NV)
	return Color > 0.04045 ? pow(Color * (1.0 / 1.055) + 0.0521327, 2.4) : Color * (1.0 / 12.92);
}

float LinearToSrgbBranchingChannel(float lin)
{
	if (lin < 0.00313067)
		return lin * 12.92;
	return pow(lin, (1.0 / 2.4)) * 1.055 - 0.055;
}

float3 LinearToSrgbBranching(float3 lin)
{
	return half3(
		LinearToSrgbBranchingChannel(lin.r),
		LinearToSrgbBranchingChannel(lin.g),
		LinearToSrgbBranchingChannel(lin.b));
}
float3 CoverNormalToColor(float3 in_normal)
{
	float3 normalized_normal = normalize(in_normal);
	float3 linear_normal = (normalized_normal + float3(1.0,1.0,1.0))* 0.5;
	return linear_normal;
}
float4 PSMain(VS_OUTPUT Input) :SV_Target
{
	float3 dir_light = float3(1.0,1.0,-1.0);
	dir_light = normalize(dir_light);
	float ndl = dot(light_dir,normalize(Input.vNormal));
	ndl = clamp(ndl,0.02,1.0);
	float4 diffuse_color = pow(g_MeshTexture.Sample(g_sampler,Input.vTexcoord),2.2);
	// float3 srgb = LinearToSrgbBranching(float3(ndl,ndl,ndl));
	float3 srgb = LinearToSrgbBranching(diffuse_color.xyz*ndl);
	return float4(srgb,1.0);
	// if(show_normal>0.0)
	// {
	// 	float3 normal_color = CoverNormalToColor(Input.vNormal);
	// 	return float4(normal_color,1.0);
	// }
	// return float4(srgb,1.0);	
}

matrix g_world;
matrix g_view;
matrix g_projection;
matrix g_VP;
matrix g_InvVP;
float3 light_dir;
float show_normal;

Buffer<float4> BoneMatrices;


struct VS_INPUT
{
    float3 vPosition : position;
    float3 vNormal : normal;
    float2 vTexCoord : uv;
    float4 vColor : color;
    float4 vWeight : weight;
    float4 vWeigh_extra : weight_extra;
    uint4 vBoneId : boneid;
    uint4 vBoneId_extra : boneid_extra;
    float3 vMorphOffset : morph_position_offset;
};
struct VS_OUTPUT
{
    float2 vTexcoord : TEXCOORD0;
    float4 vPosition : SV_POSITION;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL;
};

float3x4 GetBoneMatrix(int index)
{
    float4 A = BoneMatrices[index * 3];
    float4 B = BoneMatrices[index * 3 + 1];
    float4 C = BoneMatrices[index * 3 + 2];
    return float3x4(A, B, C);
}

VS_OUTPUT VSMain(VS_INPUT Input)
{
    VS_OUTPUT Output;
    matrix vp = mul(g_view, g_projection);
    matrix wvp = mul(g_world, vp);
 
    float3x4 blend_matrix_simple = Input.vWeight.x * GetBoneMatrix(Input.vBoneId.x) +
                   Input.vWeight.y * GetBoneMatrix(Input.vBoneId.y) +
                   Input.vWeight.z * GetBoneMatrix(Input.vBoneId.z) +
                   Input.vWeight.w * GetBoneMatrix(Input.vBoneId.w) +
                   Input.vWeigh_extra.x * GetBoneMatrix(Input.vBoneId_extra.x) +
                   Input.vWeigh_extra.y * GetBoneMatrix(Input.vBoneId_extra.y) +
                   Input.vWeigh_extra.z * GetBoneMatrix(Input.vBoneId_extra.z) +
                   Input.vWeigh_extra.w * GetBoneMatrix(Input.vBoneId_extra.w);

    float3 morphed_position = Input.vPosition + Input.vMorphOffset;
    float4 position_after_blend_simple = float4(mul(blend_matrix_simple, float4(morphed_position, 1.0)), 1.0);
    Output.vPosition = mul(position_after_blend_simple, wvp);
    Output.vTexcoord = Input.vTexCoord;
	// Output.vColor = float4(1.0,1.0,1.0,1.0);
    Output.vColor = Input.vColor;
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
    float3 linear_normal = (normalized_normal + float3(1.0, 1.0, 1.0)) * 0.5;
    return linear_normal;
}
float4 PSMain(VS_OUTPUT Input) : SV_Target
{
    float3 dir_light = float3(1.0, 1.0, -1.0);
    dir_light = normalize(dir_light);
    float ndl = dot(light_dir, normalize(Input.vNormal));
    ndl = clamp(ndl, 0, 1.0);
	//float3 ndl_v = float3(ndl,ndl,ndl)* Input.vColor.xyz;
    float3 srgb = LinearToSrgbBranching(ndl);
    if (show_normal > 0.0)
    {
        float3 normal_color = CoverNormalToColor(Input.vNormal);
        return float4(normal_color, 1.0);
    }
    return float4(srgb, 1.0);
}
#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Math/YMatrix.h"

enum MaterialDomain
{
	MD_Surface = 0,
	MD_DeferredDecal = 1,
	MD_LightFunction = 2,
	MD_Volume = 3,
	MD_PostProcess = 4,
	MD_UserInterface = 5
};

enum BlendMode
{
	BM_Opaque = 0,
	BM_Mask = 1,
	BM_Translucent = 2,
	BM_Addictive = 3
};

enum ShadingModel
{
	SM_Unlit = 0,
	SM_DefaultLit = 1,
	SM_Subsurface = 2,
	SM_PreintegratedSkin = 3,
	SM_ClearCoat = 4,
	SM_SubsurfaceProfile = 5,
	SM_TwoSideFoliage = 6,
	SM_Hair = 7,
	SM_Cloth = 8,
	SM_Eye = 9,
	SM_SingleLayerWater = 10,
	SM_ThinTranslucent = 11,
};

struct SRenderState
{
	MaterialDomain material_domain = MaterialDomain::MD_Surface;
	BlendMode blend_model = BlendMode::BM_Opaque;
	ShadingModel shading_model = ShadingModel::SM_DefaultLit;
	bool two_side = false;
};


typedef std::variant<float, YVector4, YVector2, YVector, std::string, YMatrix> MaterialParam;
struct YMaterial
{
public:
	YMaterial();
	friend class SMaterial;
	friend class SDynamicMaterial;
	virtual void SetFloat(const std::string& name, float value);
	virtual void SetVector4(const std::string& name, const YVector4& value);
	virtual void SetTexture(const std::string& name, const std::string& pic_path);
	virtual void SetSampler(const std::string& name, SamplerType sample_type);
	virtual void SetSampler(const std::string& name, SamplerAddressMode address_mode_x, SamplerAddressMode address_mode_y, SamplerFilterType filter_mode);
	const SRenderState& GetRenderState() const;
protected:
	virtual void SetRenderState(const SRenderState& render_state);
	std::string name_;
	std::unordered_map<std::string, MaterialParam> paramters_;
	SRenderState render_state_;
};

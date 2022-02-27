#include "Engine/YMaterial.h"
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11Device.h"
YMaterial::YMaterial()
{

}

void YMaterial::SetFloat(const std::string& name, float value)
{

}

void YMaterial::SetVector4(const std::string& name, const YVector4& value)
{

}

void YMaterial::SetTexture(const std::string& name, const std::string& pic_path)
{

}

const SRenderState& YMaterial::GetRenderState() const
{

	return render_state_;
}

bool YMaterial::AllocGpuResource()
{
	return true;
}

void YMaterial::SetRenderState(const SRenderState& render_state)
{

}


#include "Engine/YEngine.h"

bool YEngine::Init()
{
	device_= D3D11Device::CreateD3D11Device();
	return true;
}


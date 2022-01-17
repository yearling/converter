#pragma once
#include <memory>
#include "RHI/DirectX11/D3D11Device.h"


class YEngine
{
public:
	static YEngine* GetEngine()
	{
		static YEngine instance;
		return &instance;
	}
	bool Init();

public:
	YEngine(const YEngine&) = delete;
	YEngine(YEngine&&) = delete;
	YEngine& operator=(const YEngine&) = delete;
	YEngine& operator=(YEngine&&) = delete;
private:
	YEngine() = default;
	~YEngine() = default;
	std::unique_ptr<D3D11Device> device_;
};

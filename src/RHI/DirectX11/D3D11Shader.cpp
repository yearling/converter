#include <d3d11shader.h>
#include "RHI/DirectX11/D3D11Shader.h"
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cassert>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "RHI/DirectX11/D3D11Texture.h"
#include <iosfwd>
#include <fstream>
#include <string>
#include "Engine/YLog.h"

class CShaderInclude : public ID3DInclude {
  public:
    CShaderInclude(std::string shaderDir, std::string systemDir = "")
        : m_ShaderDir(std::move(shaderDir)), m_SystemDir(std::move(systemDir)) {}

    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, unsigned int* pBytes) {
        std::string finalPath;
        switch (IncludeType) {
            case D3D_INCLUDE_LOCAL:
                finalPath = m_ShaderDir + TEXT("\\") + pFileName;
                break;
            case D3D_INCLUDE_SYSTEM:
                finalPath = m_SystemDir + TEXT("\\") + pFileName;
                break;
            default:
                assert(0);
        }
        std::ifstream includeFile(finalPath, std::ios::in | std::ios::binary | std::ios::ate);
        if (includeFile.is_open()) {
            long long fileSize = includeFile.tellg();
            char* buf = new char[fileSize];
            includeFile.seekg(0, std::ios::beg);
            includeFile.read(buf, fileSize);
            includeFile.close();
            *ppData = buf;
            *pBytes = (unsigned int)fileSize;
        } else {
            return E_FAIL;
        }
        return S_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData) {
        char* buf = (char*)pData;
        delete[] buf;
        return S_OK;
    }

  private:
    std::string m_ShaderDir;
    std::string m_SystemDir;
};

bool ComplieShaderFromFile(const std::string& ShaderFileName, const std::string& entry_point, const std::string& shader_model,
                           const std::vector<ShaderMacroEntry>& ShaderMacroEntrys, const std::string& IncludePath, TComPtr<ID3DBlob>& blob,
                           /*out*/ std::string& ErrorMsg, bool ColomMajor = true) {
    return true;
//    HRESULT hr = S_OK;
//    DWORD shader_flags = D3DCOMPILE_ENABLE_STRICTNESS;
//    if (!ColomMajor) shader_flags |= D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;
//#    if defined(DEBUG) || defined(_DEBUG)
//    shader_flags |= D3DCOMPILE_DEBUG;
//    shader_flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
//    shader_flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
//#    endif
//    TComPtr<ID3DBlob> err_bob;
//    std::vector<D3D_SHADER_MACRO> D3DShaderMacro;
//    if (ShaderMacroEntrys.size()) {
//        for (const ShaderMacroEntry& Entry : ShaderMacroEntrys) {
//            D3D_SHADER_MACRO Macro;
//            Macro.Name = Entry.MacroName.c_str();
//            Macro.Definition = Entry.Value.c_str();
//            D3DShaderMacro.push_back(std::move(Macro));
//        }
//        D3D_SHADER_MACRO MacroEnd;
//        MacroEnd.Definition = nullptr;
//        MacroEnd.Name = nullptr;
//        D3DShaderMacro.push_back(std::move(MacroEnd));
//    }
//    std::unique_ptr<CShaderInclude> ShaderInclude = IncludePath.empty() ? nullptr : std::make_unique<CShaderInclude>(IncludePath);
//
//    if (FAILED(hr = D3DCompileFromFile(ShaderFileName.c_str(), ShaderMacroEntrys.size() ? &D3DShaderMacro[0] : nullptr, ShaderInclude.get(),
//                                       entry_point.c_str(), shader_model.c_str(), shader_flags, 0, &blob, &err_bob))) {
//        if (err_bob) {
//            ErrorMsg = (char*)err_bob->GetBufferPointer();
//        }
//        return false;
//    }
    return true;
}

bool ComplieShaderFromSource(const std::string& ShaderString, const std::string& entry_point, const std::string& shader_model,
                           const std::vector<ShaderMacroEntry>& ShaderMacroEntrys, const std::string& IncludePath, TComPtr<ID3DBlob>& blob,
                           /*out*/ std::string& ErrorMsg, bool ColomMajor = true) {
    HRESULT hr = S_OK;
    DWORD shader_flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (!ColomMajor) shader_flags |= D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;
#    if defined(DEBUG) || defined(_DEBUG)
    shader_flags |= D3DCOMPILE_DEBUG;
    shader_flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
    shader_flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
#    endif
    TComPtr<ID3DBlob> err_bob;
    std::vector<D3D_SHADER_MACRO> D3DShaderMacro;
    if (ShaderMacroEntrys.size()) {
        for (const ShaderMacroEntry& Entry : ShaderMacroEntrys) {
            D3D_SHADER_MACRO Macro;
            Macro.Name = Entry.MacroName.c_str();
            Macro.Definition = Entry.Value.c_str();
            D3DShaderMacro.push_back(Macro);
        }
        D3D_SHADER_MACRO MacroEnd;
        MacroEnd.Definition = nullptr;
        MacroEnd.Name = nullptr;
        D3DShaderMacro.push_back(MacroEnd);
    }
    std::unique_ptr<CShaderInclude> ShaderInclude = IncludePath.empty() ? nullptr : std::make_unique<CShaderInclude>(IncludePath);

    if (FAILED(hr = D3DCompile(ShaderString.c_str(),ShaderString.size(),nullptr, ShaderMacroEntrys.size() ? &D3DShaderMacro[0] : nullptr, ShaderInclude.get(),
                                       entry_point.c_str(), shader_model.c_str(), shader_flags, 0, &blob, &err_bob))) {
        if (err_bob) {
            ErrorMsg = (char*)err_bob->GetBufferPointer();
            ERROR_INFO("shader compile failed!" ,ShaderString ,"\n", ErrorMsg);
        }
        return false;
    }
    return true;
}

 ID3DShaderBind::ID3DShaderBind(){}

ID3DShaderBind::~ID3DShaderBind() {}

#if 0
bool ID3DShaderBind::PostReflection(TComPtr<ID3DBlob>& Blob, TComPtr<ID3D11ShaderReflection>& ShaderReflector) {
    // nothing
    return true;
}

#endif
void ID3DShaderBind::AddShaderMacros(const std::vector<ShaderMacroEntry>& InShaderMacroEntrys) {
    //ShaderMacroEntrys.insert(ShaderMacroEntrys.end(), InShaderMacroEntrys.begin(),InShaderMacroEntrys.end());
}

void ID3DShaderBind::SetInclude(const std::string& ShaderSrcInclude) { ShaderIncludePath = ShaderSrcInclude; }

bool ID3DShaderBind::AddAlias(const std::string& AliasName) {
#    ifdef DEBUG
    AliasNameForDebug = AliasName;
    TComPtr<ID3D11DeviceChild> DeviceChild = GetInternalResource();
    if (DeviceChild && (!AliasNameForDebug.empty())) {
        HRESULT hr = S_OK;
        if (!AliasNameForDebug.empty()) {
            if (FAILED(hr = DeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, AliasNameForDebug.length() * sizeof(TCHAR),
                                                        AliasNameForDebug.c_str()))) {
                return false;
            }
        }
    }
#    endif
    return true;
}

bool ID3DShaderBind::Update() {
    ID3D11DeviceContext* device_context = g_device->GetDC();
    for (std::unique_ptr<D3DConstantBuffer>& ConstantBuffer : ConstantBuffers) {
        if (!ConstantBuffer->AllocResource()) return false;
        D3D11_MAPPED_SUBRESOURCE MapResource;
        HRESULT hr = device_context->Map(ConstantBuffer->D3DBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResource);
        memcpy_s(MapResource.pData, ConstantBuffer->CBSize, ConstantBuffer->ShadowBuffer.data(), ConstantBuffer->CBSize);
        device_context->Unmap(ConstantBuffer->D3DBuffer, 0);
    }

    return true;
}


bool ID3DShaderBind::BindResource(const std::string& ParaName, int n) { 
    assert(0);
    return true; }


bool ID3DShaderBind::BindResource(int slot, int n) {
    assert(0);
    return true; 
}

bool ID3DShaderBind::BindResource(int slot, float f) {
    assert(slot >= -1);
    const int constant_buffer_index = 0;
    if (slot > -1) {
        D3DConstantBuffer::YCBScalar<float>::SetValue(ConstantBuffers[constant_buffer_index].get(), slot, f);
        return true;
    }
    return false;
}

bool ID3DShaderBind::BindResource(int slot, const float* f, int Num) {
    const int constant_buffer_index = 0;
    assert(slot >= -1);
    if (slot>-1) {
        assert( (2 <= Num && Num <= 4) ||(Num==12 ) ||(Num ==16));
        D3DConstantBuffer::YCBVectorVary<float>::SetValue(ConstantBuffers[constant_buffer_index].get(), slot, Num, f);
        return true;
    }
    return false;
}

bool ID3DShaderBind::BindResource(int slot, const char* c, int Num) {
    const int constant_buffer_index = 0;
    if (slot > -1) {
        D3DConstantBuffer::YCBVectorVary<char>::SetValue(ConstantBuffers[constant_buffer_index].get(), slot, Num, c);
        return true;
    }
    return false;
}

bool ID3DShaderBind::BindSRV(const std::string& ParamName, const D3DShaderResourceView& InSRV) {
    auto find_result  = MapSRV.find(ParamName);
    if (find_result!=MapSRV.end()) {
        find_result->second = InSRV;
        return true;
    } else {
        return false;
    }
}


bool ID3DShaderBind::BindSRV(int slot, D3DTexture* texture) { 
    return true;
}

bool ID3DShaderBind::BindTextureSampler(const std::string& sample_name, D3DTextureSampler* sampler) {
    samplers_map[sample_name]=sampler;
    return true;
}

bool ID3DShaderBind::BindResourceHelp(const std::string& ParaName, ScalarIndex& Index) {
    auto FindResult = MapShaderVariableToScalar.find(ParaName);
    if (FindResult!=MapShaderVariableToScalar.end()) {
        std::cout << "Bind [" << ParaName << "] Shader Failed!!" << std::endl;
        return false;
    }
    Index = FindResult->second;

    return true;
}

bool ID3DShaderBind::BindResource(const std::string& ParaName, float f) {
    ScalarIndex Index;
    if (BindResourceHelp(ParaName, Index)) {
        if (Index.Type != ScalarIndex::eType::FLOAT) {
            assert(0);
            return false;
        }
        D3DConstantBuffer::YCBScalar<float>::SetValue(ConstantBuffers[Index.ConstantBufferIndex].get(), Index.ValueIndex, f);
        return true;
    }
    return false;
}

bool ID3DShaderBind::BindResource(const std::string& ParaName, const float* f, int Num) {
    ScalarIndex Index;
    if (BindResourceHelp(ParaName, Index)) {
        assert(2 <= Num && Num <= 4);
        if (Num == 2) {
            D3DConstantBuffer::YCBVector2::SetValue(ConstantBuffers[Index.ConstantBufferIndex].get(), Index.ValueIndex, f);
        } else if (Num == 3) {
            D3DConstantBuffer::YCBVector3::SetValue(ConstantBuffers[Index.ConstantBufferIndex].get(), Index.ValueIndex, f);
        } else if (Num == 4) {
            D3DConstantBuffer::YCBVector4::SetValue(ConstantBuffers[Index.ConstantBufferIndex].get(), Index.ValueIndex, f);
        }
    }
    return false;
}


//bool ID3DShaderBind::BindResource(const std::string& ParaName, const FMatrix* Mat, int Num) { return true; }

//bool ID3DShaderBind::BindResource(const std::string& ParaName, FVector2D V2) {
//    ScalarIndex Index;
//    if (BindResourceHelp(ParaName, Index)) {
//        if (Index.Type != ScalarIndex::eType::FLOAT2) {
//            assert(0);
//            return false;
//        }
//        D3DConstantBuffer::YCBVector2::SetValue(ConstantBuffers[Index.ConstantBufferIndex].Get(), Index.ValueIndex, (float*)&V2);
//        return true;
//    }
//    return false;
//}
//
//bool ID3DShaderBind::BindResource(const std::string& ParaName, FVector V3) {
//    ScalarIndex Index;
//    if (BindResourceHelp(ParaName, Index)) {
//        if (Index.Type != ScalarIndex::eType::FLOAT3) {
//            assert(0);
//            return false;
//        }
//        D3DConstantBuffer::YCBVector3::SetValue(ConstantBuffers[Index.ConstantBufferIndex].Get(), Index.ValueIndex, (float*)&V3);
//        return true;
//    }
//    return false;
//}
//
//bool ID3DShaderBind::BindResource(const std::string& ParaName, FPlane V4) {
//    ScalarIndex Index;
//    if (BindResourceHelp(ParaName, Index)) {
//        if (Index.Type != ScalarIndex::eType::FLOAT4) {
//            assert(0);
//            return false;
//        }
//        D3DConstantBuffer::YCBVector4::SetValue(ConstantBuffers[Index.ConstantBufferIndex].Get(), Index.ValueIndex, (float*)&V4);
//        return true;
//    }
//    return false;
//}
//bool ID3DShaderBind::BindResource(const std::string& ParaName, const FMatrix& Mat) {
//    ScalarIndex Index;
//    if (BindResourceHelp(ParaName, Index)) {
//        if (Index.Type != ScalarIndex::eType::MATRIX4X4) {
//            assert(0);
//            return false;
//        }
//        D3DConstantBuffer::YCBMatrix4X4::SetValue(ConstantBuffers[Index.ConstantBufferIndex].Get(), Index.ValueIndex, Mat);
//        return true;
//    }
//    return false;
//}
bool ID3DShaderBind::ReflectShader(TComPtr<ID3DBlob> Blob) {
    TComPtr<ID3D11ShaderReflection> ShaderReflector;
    HRESULT hr = D3DReflect(Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&ShaderReflector);
    if (FAILED(hr)) {
        return false;
    }
    D3D11_SHADER_DESC ShaderDesc;
    ShaderReflector->GetDesc(&ShaderDesc);
    // 获取shader中数据
    // Constant Buffer's count: $Global, cbxxx
    // Resource
    unsigned int ResourceCount = ShaderDesc.BoundResources;
    unsigned int ConstantBufferCount = ShaderDesc.ConstantBuffers;
    unsigned int ParsedConstantBufferCount = 0;
    for (unsigned int nResourceIndex = 0; nResourceIndex < ResourceCount; ++nResourceIndex) {
        D3D11_SHADER_INPUT_BIND_DESC BindDesc;
        if (FAILED(ShaderReflector->GetResourceBindingDesc(nResourceIndex, &BindDesc))) {
            return false;
        }

        if (BindDesc.Type == D3D_SIT_CBUFFER) {
            ParsedConstantBufferCount++;
            D3D11_SHADER_BUFFER_DESC ShaderBufferDesc;
            unsigned int CurrentConstantBufferIndex = 0;  // 当前CB的index
            for (unsigned int nCurrentCBIndex = 0; nCurrentCBIndex < ConstantBufferCount; ++nCurrentCBIndex) {
                ID3D11ShaderReflectionConstantBuffer* pConstBufferFromIndex = ShaderReflector->GetConstantBufferByIndex(nCurrentCBIndex);
                D3D11_SHADER_BUFFER_DESC ShaderBufferDescTmp;
                pConstBufferFromIndex->GetDesc(&ShaderBufferDescTmp);
                if (strcmp(ShaderBufferDescTmp.Name, BindDesc.Name) == 0) {
                    CurrentConstantBufferIndex = nCurrentCBIndex;
                    break;
                }
            }

            ID3D11ShaderReflectionConstantBuffer* pConstBuffer = ShaderReflector->GetConstantBufferByName(BindDesc.Name);
            pConstBuffer->GetDesc(&ShaderBufferDesc);
            ConstantBuffers.push_back(std::make_unique<D3DConstantBuffer>(ShaderBufferDesc.Size));
            std::unique_ptr<D3DConstantBuffer>& ConstantBuffer = ConstantBuffers.back();
            ConstantBuffer->CBName = ShaderBufferDesc.Name;
            ConstantBuffer->CBType = (D3DConstantBuffer::eCBType)ShaderBufferDesc.Type;
            ConstantBuffer->BindSlotIndex = BindDesc.BindPoint;
            ConstantBuffer->BindSlotNum = BindDesc.BindCount;
            for (unsigned int j = 0; j < ShaderBufferDesc.Variables; j++) {
                ID3D11ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
                D3D11_SHADER_VARIABLE_DESC VarDesc;
                pVariable->GetDesc(&VarDesc);

                ID3D11ShaderReflectionType* pType = pVariable->GetType();
                D3D11_SHADER_TYPE_DESC TypeDesc;
                pType->GetDesc(&TypeDesc);
                if (TypeDesc.Class == D3D_SHADER_VARIABLE_CLASS::D3D_SVC_SCALAR) {
                    if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_BOOL) {
                        assert(0 && "shader reflection not support");
                    } else if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_INT) {
                        assert(0 && "shader reflection not support");
                    } else if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT) {
                        if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT) {
                            AddScalarVariable(VarDesc.Name, CurrentConstantBufferIndex, VarDesc.StartOffset, ScalarIndex::eType::FLOAT);
                        } else {
                            assert(0 && "shader reflection not support");
                        }
                    } else {
                        assert(0 && "shader reflection not support scalar type");
                    }
                } else if (TypeDesc.Class == D3D_SHADER_VARIABLE_CLASS::D3D_SVC_VECTOR) {
                    if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT) {
                        assert(TypeDesc.Rows == 1);
                        if (TypeDesc.Columns == 2) {
                            AddScalarVariable(VarDesc.Name, CurrentConstantBufferIndex, VarDesc.StartOffset, ScalarIndex::eType::FLOAT2);
                        } else if (TypeDesc.Columns == 3) {
                            AddScalarVariable(VarDesc.Name, CurrentConstantBufferIndex, VarDesc.StartOffset, ScalarIndex::eType::FLOAT3);
                        } else if (TypeDesc.Columns == 4) {
                            AddScalarVariable(VarDesc.Name, CurrentConstantBufferIndex, VarDesc.StartOffset, ScalarIndex::eType::FLOAT4);
                        } else {
                            assert(0);
                        }
                    }
                } else if (TypeDesc.Class == D3D_SHADER_VARIABLE_CLASS::D3D_SVC_MATRIX_COLUMNS) {
                    if (TypeDesc.Type == D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT && TypeDesc.Columns == 4 && TypeDesc.Rows == 4) {
                        AddScalarVariable(VarDesc.Name, CurrentConstantBufferIndex, VarDesc.StartOffset, ScalarIndex::eType::MATRIX4X4);
                    } else {
                        // assert(0 && "shader reflection not support matrix type");
                    }
                } else if (TypeDesc.Class == D3D_SHADER_VARIABLE_CLASS::D3D_SVC_MATRIX_ROWS) {
                    assert(0 && "shader not support matrix row major");
                }
            }
        } else if (BindDesc.Type == D3D_SIT_TEXTURE) {
            D3DShaderResourceView StructureBuffer;;
            StructureBuffer.Name = BindDesc.Name;
            StructureBuffer.BindCount = BindDesc.BindCount;
            StructureBuffer.BindPoint = BindDesc.BindPoint;
            auto find_result = MapSRV.find(StructureBuffer.Name);
            if (find_result==MapSRV.end()) {
                MapSRV[BindDesc.Name] = StructureBuffer;
            } else {
                assert(0 && "Structured buffer should not have the same name");
            }
        } else if (BindDesc.Type == D3D_SIT_SAMPLER) {
            std::string sampler_name = BindDesc.Name;
            auto& find_resut = sampler_slot.find(sampler_name);
            if (find_resut == sampler_slot.end()) {
                sampler_slot.insert({sampler_name,BindDesc.BindPoint});
            }
            else{
                assert(0 && "Sampler should not have the same name");
            }
        }
    }
    assert(ConstantBufferCount == ParsedConstantBufferCount);
    //if (!PostReflection(Blob, ShaderReflector)) {
        //UE_LOG(ShaderLog, Error, TEXT("PostReflection failed!! FileName %s "), *ShaderPath);
        //return false;
    //}
    return true;
}

void ID3DShaderBind::AddScalarVariable(const std::string& Name, unsigned int InConstantBufferIndex, unsigned int InValueIndex, ScalarIndex::eType InType) {
    ScalarIndex Index;
    Index.ConstantBufferIndex = InConstantBufferIndex;
    Index.ValueIndex = InValueIndex;
    Index.Type = InType;
    if (MapShaderVariableToScalar.count(Name)==0) {
        MapShaderVariableToScalar[Name] = std::move(Index);
    } else {
        assert(0 && "should not have same name variable");
    }
}


 D3DVertexShader::D3DVertexShader(){}

D3DVertexShader::~D3DVertexShader() {}

bool D3DVertexShader::CreateShader(const std::string& FileName, const std::string& MainPoint) {
    //check(!VertexShader);
    //HRESULT hr = S_OK;
    //TComPtr<ID3DBlob> VSBlob;
    //std::string ErrorMsg;
    //if (!ComplieShaderFromFile(FileName, MainPoint, "vs_5_0", ShaderMacroEntrys, ShaderIncludePath, VSBlob, ErrorMsg)) {
    //    //UE_LOG(ShaderLog, Error, TEXT("VS Shader file compile failed!! FileName: %s, ErrorMsg is %s"), *FileName, *ErrorMsg);
    //    if (ShaderMacroEntrys.Num()) {
    //        // std::cout << "ShaderMacroEntrys:" << std::endl;
    //        // for (ShaderMacroEntry &Entry : ShaderMacroEntrys)
    //        //{
    //        // std::cout << "\"" << *Entry.MacroName << "\"" << '[' << *Entry.Value << ']' << std::endl;
    //        //}
    //    }
    //    if (!ShaderIncludePath.empty()) {
    //        // std::cout << "ShaderInclude:" << *ShaderIncludePath << std::endl;
    //    }
    //    return false;
    //}

    //ShaderPath = FileName;
    //if (FAILED(hr = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &VertexShader))) {
    //    //UE_LOG(ShaderLog, Error, TEXT("VS Shader create failed!! FileName:%s, MainPoint is %s "), *FileName, *MainPoint);
    //    return false;
    //}

    //if (!ReflectShader(VSBlob)) {
    //    //UE_LOG(ShaderLog, Error, TEXT("YVSShader ReflectShader error, %s"), *ShaderPath);
    //    return false;
    //}

    //assert(VertexShader);
    //if (!AliasNameForDebug.empty()) {
    //    AddAlias(AliasNameForDebug);
    //}
    return true;
}

bool D3DVertexShader::CreateShaderFromSource(const std::string& FileName, const std::string& MainPoint, IVertexFactory* vertex_factory) {
    assert(g_device) ;

    HRESULT hr = S_OK;
    TComPtr<ID3DBlob> VSBlob;
    std::string ErrorMsg;
    if (!ComplieShaderFromSource(FileName, MainPoint, "vs_5_0", ShaderMacroEntrys, ShaderIncludePath, VSBlob, ErrorMsg)) {
        ERROR_INFO("compile shader error\n");
        ERROR_INFO(FileName);
        ERROR_INFO(ErrorMsg);
        return false;
    }
    ID3D11Device* d3d_device = g_device->GetDevice();
    if (FAILED(hr = d3d_device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &VertexShader))) {
        ERROR_INFO("Create Vertex Shader failed!");
        return false;
    }

    if (!ReflectShader(VSBlob)) {
        // UE_LOG(ShaderLog, Error, TEXT("YVSShader ReflectShader error, %s"), *ShaderPath);
        ERROR_INFO("Create Vertex Shader failed!");
        return false;
    }
    if (!CreateInputLayout(VSBlob, vertex_factory)) {
        ERROR_INFO("Create Vertex Shader failed!");
    
    }
   
    return true;
}

bool D3DVertexShader::Update() {
    ID3DShaderBind::Update();
    ID3D11DeviceContext* device_context = g_device->GetDC();
    if (VertexShader) {
        device_context->VSSetShader(VertexShader, nullptr, 0);
        //device_context->IASetInputLayout(InputLayout);
        for (std::unique_ptr<D3DConstantBuffer>& ConstantBuffer : ConstantBuffers) {
            device_context->VSSetConstantBuffers(ConstantBuffer->BindSlotIndex, ConstantBuffer->BindSlotNum, &ConstantBuffer->D3DBuffer);
        }

        for (auto& SRVItem : MapSRV) {
            D3DShaderResourceView& SRVValue = SRVItem.second;
            //device_context->VSSetShaderResources(SRVValue.BindPoint, SRVValue.BindCount, &SRVValue.SRV);
        }

         for (auto& slot : sampler_slot) {
            if (samplers_map.count(slot.first) == 1) {
                D3DTextureSampler* sampler = samplers_map[slot.first];
                if (slot.second != -1 && sampler) {
                    ID3D11SamplerState* state = sampler->GetSampler();
                    device_context->VSSetSamplers(slot.second, 1, &state);
                }
            }
        }
        return true;
    }
    return false;
}

bool D3DVertexShader::BindInputLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> InInputLayoutDesc) {
    InputLayoutDesc.swap(InInputLayoutDesc);
    return true;
}

bool D3DVertexShader::ClearSlot() {
    //TComPtr<ID3D11DeviceContext>& DeviceContext = YYUTDXManager::GetInstance().GetD3DDC();
    ID3D11DeviceContext* device_context = g_device->GetDC();
    for (auto& SRVItem : MapSRV) {
        const D3DShaderResourceView& SRVValue = SRVItem.second;
        std::vector<ID3D11ShaderResourceView*> tmp(SRVValue.BindCount,nullptr);
        device_context->VSSetShaderResources(SRVValue.BindPoint, SRVValue.BindCount, &tmp[0]);
    }
    return true;
}

bool D3DVertexShader::BindSRV(int slot, D3DTexture* texture) {
    if(slot==-1){
        return false;
    }
    D3DTexture2D* texture_2d = dynamic_cast<D3DTexture2D*>(texture);
    ID3D11ShaderResourceView* loc_srv = texture_2d->srv_;
    g_device->GetDC()->VSSetShaderResources(slot,1,&loc_srv);
    return true;
}

#if 0
bool D3DVertexShader::PostReflection(TComPtr<ID3DBlob>& Blob, TComPtr<ID3D11ShaderReflection>& ShaderReflector) {
    D3D11_SHADER_DESC ShaderDesc;
    ShaderReflector->GetDesc(&ShaderDesc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> ReflectInputLayoutDesc;
    for (unsigned lI = 0; lI < ShaderDesc.InputParameters; lI++) {
        D3D11_SIGNATURE_PARAMETER_DESC lParamDesc;
        ShaderReflector->GetInputParameterDesc(lI, &lParamDesc);

        D3D11_INPUT_ELEMENT_DESC lElementDesc;
        lElementDesc.SemanticName = lParamDesc.SemanticName;
        lElementDesc.SemanticIndex = lParamDesc.SemanticIndex;

        lElementDesc.InputSlot = 0;
        lElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        lElementDesc.InstanceDataStepRate = 0;
        lElementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

       	if (lParamDesc.Mask == 1) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        } else if (lParamDesc.Mask <= 3) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (lParamDesc.Mask <= 7) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        } else if (lParamDesc.Mask <= 15) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        ReflectInputLayoutDesc.push_back(lElementDesc);
    }
    if (InputLayoutDesc.size() < ReflectInputLayoutDesc.size()) {
        //UE_LOG(ShaderLog, Error, TEXT("InputLayout is not compatible with shader reflection"));
        return false;
    }

    bool InputLayoutMarch = true;
    // 反射出来的有可能比手动创建的D3D11_INPUT_ELEMENT_DESC少
    for (int i = 0; i < ReflectInputLayoutDesc.size(); ++i) {
        bool bFindSameNameAndIndexDesc = false;
        for (int j = 0; j < InputLayoutDesc.size(); ++j) {
            if (strcmp(InputLayoutDesc[j].SemanticName, ReflectInputLayoutDesc[i].SemanticName) == 0 &&
				InputLayoutDesc[j].SemanticIndex == ReflectInputLayoutDesc[i].SemanticIndex /*&&
				InputLayoutDesc[j].InputSlot == ReflectInputLayoutDesc[i].InputSlot*/)
			{
                bFindSameNameAndIndexDesc = true;
                break;
            }
        }

        if (!bFindSameNameAndIndexDesc) {
            InputLayoutMarch = false;
            break;
        }
    }
    if (!InputLayoutMarch) {
        //UE_LOG(ShaderLog, Error, TEXT("InputLayout is not compatible with shader reflection"));
        return false;
    }
    ID3D11Device* d3d_device = g_device->GetDevice();
    if (FAILED(d3d_device->CreateInputLayout(&InputLayoutDesc[0], ShaderDesc.InputParameters, Blob->GetBufferPointer(),
                                             Blob->GetBufferSize(),
                                         &InputLayout))) {
        //UE_LOG(ShaderLog, Error, TEXT("VS Shader create layout failed!! FileName %s "), *ShaderPath);
        return false;
    }
    return true;
}
#endif

bool D3DVertexShader::ReflectShader(TComPtr<ID3DBlob> Blob) {
    if (!ID3DShaderBind::ReflectShader(Blob)) {
        return false;
    }
    TComPtr<ID3D11ShaderReflection> ShaderReflector;
    HRESULT hr = D3DReflect(Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&ShaderReflector);
    if (FAILED(hr)) {
        return false;
    }
    D3D11_SHADER_DESC ShaderDesc;
    ShaderReflector->GetDesc(&ShaderDesc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> ReflectInputLayoutDesc;
    for (unsigned lI = 0; lI < ShaderDesc.InputParameters; lI++) {
        D3D11_SIGNATURE_PARAMETER_DESC lParamDesc;
        ShaderReflector->GetInputParameterDesc(lI, &lParamDesc);

        D3D11_INPUT_ELEMENT_DESC lElementDesc;
        lElementDesc.SemanticName = lParamDesc.SemanticName;
        lElementDesc.SemanticIndex = lParamDesc.SemanticIndex;

        lElementDesc.InputSlot = 0;
        lElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        lElementDesc.InstanceDataStepRate = 0;
        lElementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

        if (lParamDesc.Mask == 1) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        } else if (lParamDesc.Mask <= 3) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (lParamDesc.Mask <= 7) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        } else if (lParamDesc.Mask <= 15) {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        ReflectInputLayoutDesc.push_back(lElementDesc);
    }
    //if (InputLayoutDesc.size() < ReflectInputLayoutDesc.size()) {
        // UE_LOG(ShaderLog, Error, TEXT("InputLayout is not compatible with shader reflection"));
        //return false;
    //}

   // bool InputLayoutMarch = true;
   // // 反射出来的有可能比手动创建的D3D11_INPUT_ELEMENT_DESC少
   // for (int i = 0; i < ReflectInputLayoutDesc.size(); ++i) {
   //     bool bFindSameNameAndIndexDesc = false;
   //     for (int j = 0; j < InputLayoutDesc.size(); ++j) {
   //         if (strcmp(InputLayoutDesc[j].SemanticName, ReflectInputLayoutDesc[i].SemanticName) == 0 &&
			//	InputLayoutDesc[j].SemanticIndex == ReflectInputLayoutDesc[i].SemanticIndex /*&&
			//	InputLayoutDesc[j].InputSlot == ReflectInputLayoutDesc[i].InputSlot*/)
			//{
   //             bFindSameNameAndIndexDesc = true;
   //             break;
   //         }
   //     }

   //     if (!bFindSameNameAndIndexDesc) {
   //         InputLayoutMarch = false;
   //         break;
   //     }
   // }
   // if (!InputLayoutMarch) {
   //     // UE_LOG(ShaderLog, Error, TEXT("InputLayout is not compatible with shader reflection"));
   //     return false;
   // }
    //ID3D11Device* d3d_device = g_device->GetDevice();
    //if (FAILED(d3d_device->CreateInputLayout(&InputLayoutDesc[0], ShaderDesc.InputParameters, Blob->GetBufferPointer(),
    //                                         Blob->GetBufferSize(), &InputLayout))) {
    //    // UE_LOG(ShaderLog, Error, TEXT("VS Shader create layout failed!! FileName %s "), *ShaderPath);
    //    return false;
    //}
    return true;
}

bool D3DVertexShader::CreateInputLayout(TComPtr<ID3DBlob> blob, IVertexFactory* vertex_factory) { 
     TComPtr<ID3D11ShaderReflection> shader_reflector;
    HRESULT hr = D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&shader_reflector);
    if (FAILED(hr)) {
        return false;
    }
    D3D11_SHADER_DESC shader_desc;
    shader_reflector->GetDesc(&shader_desc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> reflected_input_layout_desc;
    for (unsigned desc_index = 0; desc_index < shader_desc.InputParameters; desc_index++) {
        D3D11_SIGNATURE_PARAMETER_DESC signature_param_desc;
        shader_reflector->GetInputParameterDesc(desc_index, &signature_param_desc);

        D3D11_INPUT_ELEMENT_DESC element_desc;
        element_desc.SemanticName = signature_param_desc.SemanticName;
        element_desc.SemanticIndex = signature_param_desc.SemanticIndex;

        element_desc.InputSlot = signature_param_desc.Register;
        element_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        element_desc.InstanceDataStepRate = 0;
        element_desc.AlignedByteOffset = 0;

        if (signature_param_desc.Mask == 1) {
            if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element_desc.Format = DXGI_FORMAT_R32_UINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element_desc.Format = DXGI_FORMAT_R32_SINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element_desc.Format = DXGI_FORMAT_R32_FLOAT;
        } else if (signature_param_desc.Mask <= 3) {
            if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element_desc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element_desc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (signature_param_desc.Mask <= 7) {
            if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        } else if (signature_param_desc.Mask <= 15) {
            if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        reflected_input_layout_desc.push_back(element_desc);
    }
    //if (InputLayoutDesc.size() < reflected_input_layout_desc.size()) {
        // UE_LOG(ShaderLog, Error, TEXT("InputLayout is not compatible with shader reflection"));
    //    return false;
    //}
    std::vector<VertexStreamDescription>& vertex_stream_descs = vertex_factory->GetVertexDescription();

    auto tell_desc_the_same = [](const VertexStreamDescription& vertex_stream_desc, const D3D11_INPUT_ELEMENT_DESC& reflected_desc) {
        if (reflected_desc.Format == DXGI_FORMAT_R32G32_FLOAT && vertex_stream_desc.data_type == DataType::Float32 &&
            vertex_stream_desc.com_num == 2) {
            return true;
        }
        if (reflected_desc.Format == DXGI_FORMAT_R32G32B32_FLOAT && vertex_stream_desc.data_type == DataType::Float32 && vertex_stream_desc.com_num ==3) {
            return true; 
        }
        if (reflected_desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT && vertex_stream_desc.data_type == DataType::Float32 &&
            vertex_stream_desc.com_num == 4) {
            return true;
        }
        if (reflected_desc.Format == DXGI_FORMAT_R32_UINT && vertex_stream_desc.data_type == DataType::Uint8 &&
            vertex_stream_desc.com_num == 4) {
            return true;
        }
        return false;
    };
    bool input_layout_march = true;
    // 反射出来的有可能比手动创建的D3D11_INPUT_ELEMENT_DESC少
    for (int i = 0; i < reflected_input_layout_desc.size(); ++i) {
        bool find_same_name = false;
        for (int j = 0; j < vertex_stream_descs.size(); ++j) {
            if (strcmp(vertex_stream_descs[j].name.c_str(), reflected_input_layout_desc[i].SemanticName)==0)
			{
                if (tell_desc_the_same(vertex_stream_descs[j], reflected_input_layout_desc[i])) {
                    find_same_name = true;
                    vertex_stream_descs[j].slot = reflected_input_layout_desc[i].InputSlot;
                  
                }
                break;
            }
        }

        if (!find_same_name) {
            input_layout_march = false;
            break;
        }
    }
    if (!input_layout_march) {
        ERROR_INFO("InputLayout is not compatible with shader reflection");
        return false;
    }
    ID3D11Device* d3d_device = g_device->GetDevice();
    TComPtr<ID3D11InputLayout> d3d_input_layout;
    if (FAILED(d3d_device->CreateInputLayout(&reflected_input_layout_desc[0], shader_desc.InputParameters, blob->GetBufferPointer(),
                                             blob->GetBufferSize(), &d3d_input_layout))) {
        // UE_LOG(ShaderLog, Error, TEXT("VS Shader create layout failed!! FileName %s "), *ShaderPath);
        ERROR_INFO("VS Shader create layout failed !!");
        return false;
    }

    DXVertexFactory* dx_vertex_factory = dynamic_cast<DXVertexFactory*>(vertex_factory);
    assert(dx_vertex_factory);
    if (!dx_vertex_factory) {
        return false; 
    }
    dx_vertex_factory->SetInputLayout(d3d_input_layout);
    return true;
}

TComPtr<ID3D11DeviceChild> D3DVertexShader::GetInternalResource() const { return TComPtr<ID3D11DeviceChild>(VertexShader); }

 ID3DShader::ID3DShader(){}

ID3DShader::~ID3DShader() {}

 D3DConstantBuffer::D3DConstantBuffer(unsigned int BufferSizeInBytes)
    : CBSize(BufferSizeInBytes), CBType(eCBType::Num), BindSlotIndex(-1), BindSlotNum(-1) {
    ShadowBuffer.resize(BufferSizeInBytes, 0);
 }

bool D3DConstantBuffer::AllocResource() {
    if (D3DBuffer)
        return true;
    else {
        HRESULT hr = S_OK;
        ID3D11Device* d3d_device = g_device->GetDevice();
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.ByteWidth = CBSize;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(hr = d3d_device->CreateBuffer(&desc, NULL, &D3DBuffer))) {
            return false;
        } else {
            return true;
        }
    }
}


 D3DPixelShader::D3DPixelShader() {}

D3DPixelShader::~D3DPixelShader() {}

bool D3DPixelShader::CreateShader(const std::string& FileName, const std::string& MainPoint) {
    assert(!PixShader);
    ID3D11Device* d3d_device = g_device->GetDevice();
    HRESULT hr = S_OK;
    TComPtr<ID3DBlob> VSBlob;
    std::string ErrorMsg;
    if (!ComplieShaderFromFile(FileName, MainPoint, "ps_5_0", ShaderMacroEntrys, ShaderIncludePath, VSBlob, ErrorMsg)) {
		ERROR_INFO("compile shader error\n");
		ERROR_INFO(FileName);
		ERROR_INFO(ErrorMsg);
        return false;
    }
    ShaderPath = FileName;
    if (FAILED(hr = d3d_device->CreatePixelShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &PixShader))) {
        std::cout << "VS Shader create failed!! \n FileName: " << FileName << "   MainPoint:" << MainPoint << std::endl;
        return false;
    }
    if (!ReflectShader(VSBlob)) {
        //UE_LOG(ShaderLog, Error, TEXT("YPSShader ReflectShader error, %s"), *ShaderPath);
        return false;
    }

    assert(PixShader);
    if (!AliasNameForDebug.empty()) {
        AddAlias(AliasNameForDebug);
    }
    return true;
}

bool D3DPixelShader::CreateShaderFromSource(const std::string& FileName, const std::string& MainPoint) {
    assert(g_device);

    HRESULT hr = S_OK;
    TComPtr<ID3DBlob> VSBlob;
    std::string ErrorMsg;
    if (!ComplieShaderFromSource(FileName, MainPoint, "ps_5_0", ShaderMacroEntrys, ShaderIncludePath, VSBlob, ErrorMsg)) {
        return false;
    }
    ID3D11Device* d3d_device = g_device->GetDevice();
    if (FAILED(hr = d3d_device->CreatePixelShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &PixShader))) {
        ERROR_INFO("Create Vertex Shader failed!");
        return false;
    }

    if (!ReflectShader(VSBlob)) {
        // UE_LOG(ShaderLog, Error, TEXT("YVSShader ReflectShader error, %s"), *ShaderPath);
        ERROR_INFO("Create Vertex Shader failed!");
        return false;
    }

    return true;
}

bool D3DPixelShader::Update() {
    ID3D11DeviceContext* d3d_dc = g_device->GetDC();
    if (ID3DShaderBind::Update() && PixShader) {
        d3d_dc->PSSetShader(PixShader, nullptr, 0);
        for (std::unique_ptr<D3DConstantBuffer>& ConstantBuffer : ConstantBuffers) {
            d3d_dc->PSSetConstantBuffers(ConstantBuffer->BindSlotIndex, 1, &ConstantBuffer->D3DBuffer);
        }
        for (auto& SRVItem : MapSRV) {
            //const D3DShaderResourceView& SRVValue = SRVItem.second;
            //d3d_dc->PSSetShaderResources(SRVValue.BindPoint, SRVValue.BindCount, &SRVValue.SRV);
        }

        for (auto& slot : sampler_slot) {
            if (samplers_map.count(slot.first) == 1) {
                D3DTextureSampler*   sampler = samplers_map[slot.first];
                if (slot.second != -1 && sampler) {
                    ID3D11SamplerState* state = sampler->GetSampler() ;
                    d3d_dc->PSSetSamplers(slot.second, 1, &state);
                }
            }
        }
        return true;
    }


    return false;
}

bool D3DPixelShader::ClearSlot() {
    ID3D11DeviceContext* d3d_dc = g_device->GetDC();
    for (auto& SRVItem : MapSRV) {
        const D3DShaderResourceView& SRVValue = SRVItem.second;
        std::vector<ID3D11ShaderResourceView*> tmp;
        tmp.resize(SRVValue.BindCount,0);
        d3d_dc->PSSetShaderResources(SRVValue.BindPoint, SRVValue.BindCount, &tmp[0]);
    }
    return true;
}

bool D3DPixelShader::BindSRV(int slot, D3DTexture* texture) {
    if (slot == -1) {
        return false;
    }
    D3DTexture2D* texture_2d = dynamic_cast<D3DTexture2D*>(texture);
    ID3D11ShaderResourceView* loc_srv = texture_2d->srv_;
    g_device->GetDC()->PSSetShaderResources(slot, 1, &loc_srv);
    return true;
}

TComPtr<ID3D11DeviceChild> D3DPixelShader::GetInternalResource() const { return TComPtr<ID3D11DeviceChild>(PixShader); }

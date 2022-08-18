#include "SObject/SMaterial.h"
#include "Utility/YJsonHelper.h"
#include <variant>
#include "SObject/SObjectManager.h"
#include "engine/YLog.h"

static MaterialDomain MapStringToMaterialDomin(const std::string& name)
{
	const std::unordered_map<std::string, MaterialDomain> map_table=
	{
		{"surface",MD_Surface},
		{"deferred_decal",MD_DeferredDecal},
		{"light_function",MD_LightFunction},
		{"volume",MD_Volume},
		{"post_process",MD_PostProcess},
		{"user_interface",MD_UserInterface}
	};
	if (map_table.count(name))
	{
		return map_table.at(name);
	}

	return MD_Surface;
}

static std::string MapMaterialDominToString(MaterialDomain material_domain)
{
    const std::unordered_map<MaterialDomain, std::string> map_table =
    {
        {MD_Surface,"surface"},
        {MD_DeferredDecal,"deferred_decal"},
        {MD_LightFunction,"light_function"},
        {MD_Volume,"volume"},
        {MD_PostProcess,"post_process"},
        {MD_UserInterface,"user_interface"}
    };
    if (map_table.count(material_domain))
    {
        return map_table.at(material_domain);
    }

    return "surface";
}


static BlendMode MapStringToBlendMode(const std::string& name)
{
	const std::unordered_map<std::string, BlendMode> map_table =
	{
		{"opaque",BM_Opaque},
		{"mask",BM_Mask},
		{"translucent",BM_Translucent},
		{"addictive",BM_Addictive}
	};
	if (map_table.count(name))
	{
		return map_table.at(name);
	}

	return BM_Opaque;
}

static std::string MapBlendModeToString(BlendMode blend_mode)
{
    const std::unordered_map<BlendMode,std::string> map_table =
    {
        {BM_Opaque,"opaque"},
        {BM_Mask,"mask"},
        {BM_Translucent,"translucent"},
        {BM_Addictive,"addictive"}
    };
    if (map_table.count(blend_mode))
    {
        return map_table.at(blend_mode);
    }

    return "opaque";
}
static ShadingModel MapStringToShadingModel(const std::string& name)
{
	const std::unordered_map<std::string, ShadingModel> map_table =
	{
		{"unlit",SM_Unlit},
		{"lit",SM_DefaultLit},
		{"subsurface",SM_Subsurface},
		{"clear_coat",SM_ClearCoat},
		{"subsurface_profile",SM_SubsurfaceProfile},
		{"twosize_foliage",SM_TwoSideFoliage},
		{"hair",SM_Hair},
		{"cloth",SM_Cloth},
		{"eye",SM_Eye},
		{"single_layer_water",SM_SingleLayerWater},
		{"thin_translucent",SM_ThinTranslucent}
	};
	if (map_table.count(name))
	{
		return map_table.at(name);
	}

	return SM_Unlit;
}

static std::string MapShadingModelToString(ShadingModel shade_mode)
{
    const std::unordered_map<ShadingModel,std::string> map_table =
    {
        {SM_Unlit,"unlit"},
        {SM_DefaultLit,"lit"},
        {SM_Subsurface,"subsurface"},
        {SM_ClearCoat,"clear_coat"},
        {SM_SubsurfaceProfile,"subsurface_profile"},
        {SM_TwoSideFoliage,"twosize_foliage"},
        {SM_Hair,"hair"},
        {SM_Cloth,"cloth"},
        {SM_Eye,"eye"},
        {SM_SingleLayerWater,"single_layer_water"},
        {SM_ThinTranslucent,"thin_translucent"}
    };
    if (map_table.count(shade_mode))
    {
        return map_table.at(shade_mode);
    }

    return "unlit";
}


bool SMaterial::LoadFromJson(const Json::Value& RootJson)
{
	if (RootJson.isMember("render_state"))
	{
		const Json::Value& render_state_json = RootJson["render_state"];
		const std::string material_domain = render_state_json["material_domain"].asString();
		render_state_.material_domain = MapStringToMaterialDomin(material_domain);
		const std::string blend_mode_str = render_state_json["blend_mode"].asString();
		render_state_.blend_model = MapStringToBlendMode(blend_mode_str);
		const std::string shading_model_str = render_state_json["shading_mode"].asString();
		render_state_.shading_model = MapStringToShadingModel(shading_model_str);
		if (RootJson.isMember("two_side"))
		{
			render_state_.two_side = render_state_json["two_side"].asBool();
		}
		else
		{
			render_state_.two_side = false;
		}
	}
	else
	{
		//default
		render_state_.material_domain = MD_Surface;
		render_state_.blend_model = BM_Opaque;
		render_state_.shading_model = SM_Unlit;
		render_state_.two_side = false;

	}

	if (RootJson.isMember("shader_path"))
	{
		shader_path_ = RootJson["shader_path"].asString();
	}
	else
	{
		ERROR_INFO("material ", name_, " does not have SHADER PATH");
		return false;
	}

	if (RootJson.isMember("material_parameters"))
	{
		const Json::Value& paramters_json = RootJson["material_parameters"];
		//assert(paramters_json.isArray());
		if (!paramters_json.isArray() && paramters_json.isNull())
		{
			WARNING_INFO("material ", name_, " material parameters should be an array");
			return true;
		}
		for (int i = 0; i <(int) paramters_json.size(); ++i)
		{
			const Json::Value& param_item_json = paramters_json[i];
			if ((!param_item_json.isMember("name"))||(!param_item_json.isMember("type"))||(!param_item_json.isMember("value")))
			{
				WARNING_INFO("material ", name_, " material parameters lost name or type or value");
				continue;
			}
			const std::string name = param_item_json["name"].asString();
			MaterialParam tmp;
			if (param_item_json["type"].asString() == "float")
			{
				float float_value = param_item_json["value"].asFloat();
				tmp = float_value;
			}
			else if (param_item_json["type"].asString() == "texture")
			{
				tmp = param_item_json["value"].asString();
			}
			else if (param_item_json["type"].asString() == "vector4")
			{
				YVector4 vec4;
				if (YJsonHelper::ConvertJsonToVector4(param_item_json["value"], vec4))
				{
					tmp = vec4;
				}
			}
            else if (param_item_json["type"].asString() == "vector3")
            {
                YVector v;
                if (YJsonHelper::ConvertJsonToVector(param_item_json["value"], v))
                {
                    tmp = v;
                }
            }
            else if (param_item_json["type"].asString() == "vector2")
            {
                YVector2 v;
                if (YJsonHelper::ConvertJsonToVector2(param_item_json["value"], v))
                {
                    tmp = v;
                }
            }
            if (param_item_json["type"].asString() == "int")
            {
                int int_value = param_item_json["value"].asInt();
                tmp = int_value;
            }
			else
			{
				assert(0);
			}
            parameters_[name] = tmp;
        }
	}
	return true;
}

bool SMaterial::SaveToJson(Json::Value& root_json)
{
    Json::Value render_state;
    render_state["material_domain"] = MapMaterialDominToString(render_state_.material_domain);
    render_state["blend_mode"] = MapBlendModeToString(render_state_.blend_model);
    render_state["shading_mode"] = MapShadingModelToString(render_state_.shading_model);
    render_state["two_side"] = render_state_.two_side;
    root_json["render_state"] = render_state;

    root_json["shader_path"] = shader_path_;
    
    Json::Value material_parameters;
    for (auto& item : parameters_)
    {
        Json::Value param_item;
        param_item["name"] = item.first;
        size_t variant_index = item.second.index();
        if (variant_index == 0)
        {
            float f = std::get<0>(item.second);
            param_item["type"] = "float";
            param_item["value"] = f;
        }
        else if (variant_index == 1)
        {
            int i = std::get<1>(item.second);
            param_item["type"] = "int";
            param_item["value"] = i;
        }
        else if (variant_index == 2)
        {
            YVector v = std::get<2>(item.second);
            param_item["type"] = "vector3";
            param_item["value"] = YJsonHelper::ConvertVectorToJson(v);
        }
        else if (variant_index == 3)
        {
            YVector4 v4 = std::get<3>(item.second);
            param_item["type"] = "vector4";
            param_item["value"] = YJsonHelper::ConvertVector4ToJson(v4);
        }
        else if (variant_index == 4)
        {
            YVector2 v2 = std::get<4>(item.second);
            param_item["type"] = "vector2";
            param_item["value"] = YJsonHelper::ConvertVector2ToJson(v2);
        }
        else if (variant_index == 5)
        {
            std::string str_texture = std::get<5>(item.second);
            param_item["type"] = "texture";
            param_item["value"] = str_texture;
        }
        else
        {
            assert(0);
        }
        material_parameters.append(param_item);
    }
    root_json["material_parameters"] = material_parameters;
    return true;
}

void SMaterial::SaveToPackage(const std::string& Path)
{
	
}

bool SMaterial::PostLoadOp()
{
	//create gpu resource
	if (material_)
	{
		material_ = nullptr;
	}

	material_ = std::make_unique<YMaterial>();
	material_->paramters_ = parameters_;
	material_->render_state_ = render_state_;
	material_ ->AllocGpuResource();
	return true;
}

void SMaterial::Update(double deta_time)
{
}

SMaterial::~SMaterial()
{

}

SMaterial::SMaterial()
{

}

SMaterial::SMaterial(SObject* parent):SObject(parent)
{

}

TRefCountPtr<SMaterial> SMaterial::GetDefaultMaterial()
{
	TRefCountPtr<SMaterial> default_mlt =  SObjectManager::ConstructFromPackage<SMaterial>("/engine/material/default_material", nullptr);
	assert(default_mlt);
#if defined(DEBUG) | defined(_DEBUG)
	ERROR_INFO("default material should not create failed , check engine resource!!");
#endif
	return default_mlt;
}

void SMaterial::SetFloat(const std::string& name, float value)
{
	if (parameters_.count(name))
	{
		float* f = std::get_if<float>(&parameters_[name]);
		if (f)
		{
			*f = value;
		}
	}
}

void SMaterial::SetVector4(const std::string& name, const YVector4& value)
{
	if (parameters_.count(name))
	{
		YVector4* f = std::get_if<YVector4>(&parameters_[name]);
		if (f)
		{
			*f = value;
		}
	}
}

void SMaterial::SetTexture(const std::string& name, const std::string& pic_path)
{
	if (parameters_.count(name))
	{
		std::string* f = std::get_if<std::string>(&parameters_[name]);
		if (f)
		{
			*f = pic_path;
		}
	}
}

const std::string& SMaterial::GetShaderPath() const
{
	return shader_path_;
}

void SMaterial::SetShader(const std::string& shader_path)
{
	shader_path_ = shader_path;
}

SDynamicMaterial::SDynamicMaterial()
{

}

SDynamicMaterial::SDynamicMaterial(const SMaterial& material)
{
	parameters_ = material.parameters_;
	render_state_ = material.render_state_;
	shader_path_ = material.shader_path_;
	modify_ = true;
}

void SDynamicMaterial::SetFloat(const std::string& name, float value)
{
	modify_ = true;
	SMaterial::SetFloat(name, value);
}

void SDynamicMaterial::SetVector4(const std::string& name, const YVector4& value)
{
	modify_= true;
	SMaterial::SetVector4(name, value);
}

void SDynamicMaterial::SetTexture(const std::string& name, const std::string& pic_path)
{
	modify_ = true;
	SMaterial::SetTexture(name, pic_path);
}

void SDynamicMaterial::Update(double deta_time)
{
	//update to render scene
	// todo 
	modify_ = false;
}

SDynamicMaterial::~SDynamicMaterial()
{

}

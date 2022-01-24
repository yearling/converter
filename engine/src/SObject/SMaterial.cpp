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
		assert(paramters_json.isArray());
		if (!paramters_json.isArray())
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
			else if (param_item_json["type"].asString() == "texture_2d")
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
			else
			{
				assert(0);
			}
		}
	}
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

#include "SObject/SStaticMesh.h"
#include "Utility/YPath.h"
#include "SObject/SMaterial.h"
#include "SObject/SObjectManager.h"
#include "Utility/YJsonHelper.h"

SStaticMesh::SStaticMesh()
{

}

SStaticMesh::SStaticMesh(SObject* parent)
	:SObject(parent)
{

}

SStaticMesh::~SStaticMesh()
{

}

bool SStaticMesh::LoadFromJson(const Json::Value& RootJson)
{
	if (!RootJson.isMember("runtime_asset"))
	{
		ERROR_INFO("SStaticMesh ", name_, " load failed!,because do not have model asset");
		return false;
	}
	
	std::string runtime_asset_path = RootJson["runtime_asset"].asString();
	if (!YPath::IsAssetAbsolutePath(runtime_asset_path))
	{
        runtime_asset_path = YPath::PathCombine(YPath::GetPath(YPath::ConverAssetPathToFilePath(name_)), runtime_asset_path);
	}
	else
	{
        runtime_asset_path = YPath::ConverAssetPathToFilePath(runtime_asset_path);
	}

	static_mesh_ = std::make_unique<YStaticMesh>();
	if (static_mesh_->LoadRuntimeData(runtime_asset_path))
	{
		LOG_INFO("Static mesh runtime load success! path: ", runtime_asset_path);
	}
	else
	{
		LOG_INFO("Static mesh runtime load failed! path: ", runtime_asset_path);
		return false;
	}

    std::string editor_asset_path = RootJson["editor_asset"].asString();
    if (!YPath::IsAssetAbsolutePath(editor_asset_path))
    {
        editor_asset_path = YPath::PathCombine(YPath::GetPath(YPath::ConverAssetPathToFilePath(name_)), editor_asset_path);
    }
    else
    {
        editor_asset_path = YPath::ConverAssetPathToFilePath(editor_asset_path);
    }

    if (static_mesh_->LoadEditorData(editor_asset_path))
    {
        LOG_INFO("Static mesh runtime editor data load success! path: ", editor_asset_path);
    }
    else
    {
        LOG_INFO("Static mesh runtime editor data load failed! path: ", editor_asset_path);
        return false;
    }

	if (RootJson.isMember("material_slot"))
	{
		const Json::Value& materials_json = RootJson["material_slot"];
		if (materials_json.isArray())
		{
			for (int i = 0; i < (int)materials_json.size(); ++i)
			{
				const Json::Value& material_item_json = materials_json[i];
				int slot_id = material_item_json["slot"].asInt();
				assert(slot_id >= 0);
				if (slot_id < 0) {
					continue;
				}
				if ((int)materials_.size() < slot_id + 1)
				{
					materials_.resize(slot_id + 1);
				}
				std::string material_path = material_item_json["material"].asString();

				TRefCountPtr<SMaterial> new_material= SObjectManager::ConstructFromPackage<SMaterial>(material_path, this);
				if (!new_material){
					//Create Default Material
					new_material = SMaterial::GetDefaultMaterial();
					WARNING_INFO("Create material ", material_path, " failed, use default material");
				}
				materials_[slot_id] = new_material;
			}
		}
		else
		{
			ERROR_INFO("Create static mesh ", name_, " failed! because material_slot is not an array");
			return false;
		}
	}
	else
	{

		WARNING_INFO("SstaticMeshComponent do not have materials, create default material");
		ERROR_INFO("Create static mesh ", name_, " failed! because material_slot is empty");
		//todo default materials
	}

	return true;
}


bool SStaticMesh::SaveToJson(Json::Value& root_json)
{
    const std::string base_path_name = GetName();
    const std::string static_mesh_model_name_runtime = static_mesh_->model_name + "_ra";
    root_json["runtime_asset"] = static_mesh_model_name_runtime; //runtime_asset
    static_mesh_->SaveRuntimeData(YPath::PathCombine(base_path_name,static_mesh_model_name_runtime ));
    const std::string static_mesh_model_name_editor = static_mesh_->model_name + "_ea";
    root_json["editor_asset"] = static_mesh_model_name_editor; //editor_asset
    static_mesh_->SaveEditorData(YPath::PathCombine(base_path_name, static_mesh_model_name_editor));

    Json::Value materials;
    for (int i = 0; i < (int)materials_.size(); ++i)
    {
        Json::Value material_item_json;
        material_item_json["id"] = i;

        TRefCountPtr<SMaterial>& material = materials_[i];
        const std::string material_name = material->GetName()+"_mtl";
        material_item_json["material"] = material_name;
        //create material json
        std::string material_json_path = YPath::PathCombine(base_path_name, material_name + json_extension_with_dot);
        Json::Value material_json_value;
        material->SaveToJson(material_json_value);
        YJsonHelper::SaveJsonToFile(material_json_path, material_json_value);
        materials.append(material_item_json);
    }
    root_json["material_slot"] = materials;

    return true;
}


bool SStaticMesh::PostLoadOp()
{
	static_mesh_->AllocGpuResource();
	
	for (TRefCountPtr<SMaterial>& mtl : materials_)
	{
		assert(mtl);
		mtl->PostLoadOp();
	}
	return true;
}

void SStaticMesh::Update(double deta_time)
{
}

YStaticMesh* SStaticMesh::GetStaticMesh() const
{
	return static_mesh_.get();
}

void SStaticMesh::SetSavedPath(const std::string new_path)
{
    name_ = new_path;
}


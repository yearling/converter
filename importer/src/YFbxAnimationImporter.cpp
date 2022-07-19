#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"
#include "fbxsdk/scene/geometry/fbxblendshape.h"
#include "Engine/YLog.h"
#include "Utility/YStringFormat.h"

std::unique_ptr<AnimationData> YFbxImporter::ImportAnimationData(YSkeleton* skeleton, std::unordered_map<int, FbxNode*>& bone_id_to_fbxnode)
{

	std::unique_ptr<AnimationData> animation_data = std::make_unique<AnimationData>();
	int32 ResampleRate = 30.0f;
	int32 AnimStackCount = fbx_scene_->GetSrcObjectCount<FbxAnimStack>();
	if (AnimStackCount == 0)
	{
		return nullptr;
	}

	float time = -1;
	int frame_rate = FbxTime::GetFrameRate(fbx_scene_->GetGlobalSettings().GetTimeMode());
	int AnimStackIndex = 0;
	FbxAnimStack* CurAnimStack = fbx_scene_->GetSrcObject<FbxAnimStack>(AnimStackIndex);
	FbxTimeSpan AnimTimeSpan = CurAnimStack->GetLocalTimeSpan();
	if (AnimTimeSpan.GetDirection() == FBXSDK_TIME_FORWARD)
	{
		time = YMath::Max((float)(AnimTimeSpan.GetDuration().GetMilliSeconds() / 1000.0f * scene_info_->frame_rate), time);
	}
	animation_data->name_ = CurAnimStack->GetName();

	const int32 NumSamplingKeys = YMath::FloorToInt(AnimTimeSpan.GetDuration().GetSecondDouble() * ResampleRate);
	animation_data->time_ = NumSamplingKeys * (1.0 / (float)frame_rate);
	const FbxTime TimeIncrement = AnimTimeSpan.GetDuration() / YMath::Max(NumSamplingKeys, 1);
	if (!skeleton)
	{
		return animation_data;
	}
	for (YBone& bone : skeleton->bones_)
	{
		AnimationSequenceTrack& animation_sequence_track = animation_data->sequence_track[bone.bone_name_];
		animation_sequence_track.pos_keys_.reserve(NumSamplingKeys);
		animation_sequence_track.rot_keys_.reserve(NumSamplingKeys);
		animation_sequence_track.scale_keys_.reserve(NumSamplingKeys);
		for (FbxTime CurTime = AnimTimeSpan.GetStart(); CurTime <= AnimTimeSpan.GetStop(); CurTime += TimeIncrement)
		{
			FbxNode* current_node = bone_id_to_fbxnode[bone.bone_id_];
			FbxAMatrix cur_global_trans = current_node->EvaluateGlobalTransform(CurTime);
			YTransform cur_local_transform;
			if (bone.parent_id_ == -1)
			{
				cur_local_transform = converter_.ConverterFbxTransform(cur_global_trans);
			}
			else
			{
				FbxNode* parent_node = bone_id_to_fbxnode[bone.parent_id_];
				FbxAMatrix parent_cur_global_trans = parent_node->EvaluateGlobalTransform(CurTime);
				FbxAMatrix local_trans = (parent_cur_global_trans.Inverse()) * cur_global_trans;
				cur_local_transform = converter_.ConverterFbxTransform(local_trans);
			}
			YTransform relative_trans = cur_local_transform* bone.bind_local_tranform_.InverseFast();
			animation_sequence_track.pos_keys_.push_back(relative_trans.translation);
			animation_sequence_track.rot_keys_.push_back(relative_trans.rotator);
			animation_sequence_track.scale_keys_.push_back(relative_trans.scale);
		}
	}
	return animation_data;
}


bool YFbxImporter::ImportBlendShapeAnimation(YSkinDataImported* skin_data, AnimationData* anim_data, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs)
{
	if (!(skin_data && anim_data))
	{
		return false;
	}


	std::unique_ptr<BlendShapeSequneceTrack> bs_sequence_track = std::make_unique<BlendShapeSequneceTrack>();

	float time = -1;
	int frame_rate = FbxTime::GetFrameRate(fbx_scene_->GetGlobalSettings().GetTimeMode());
	int AnimStackIndex = 0;
	int32 ResampleRate = 30.0f;
	//for (int32 AnimStackIndex = 0; AnimStackIndex < AnimStackCount; AnimStackIndex++)
	FbxAnimStack* CurAnimStack = fbx_scene_->GetSrcObject<FbxAnimStack>(AnimStackIndex);
	FbxTimeSpan AnimTimeSpan = CurAnimStack->GetLocalTimeSpan();
	FbxAnimLayer* current_animation_layer = CurAnimStack->GetMember<FbxAnimLayer>();
	if (AnimTimeSpan.GetDirection() == FBXSDK_TIME_FORWARD)
	{
		time = YMath::Max((float)(AnimTimeSpan.GetDuration().GetMilliSeconds() / 1000.0f * scene_info_->frame_rate), time);
	}

	const int32 NumSamplingKeys = YMath::FloorToInt(AnimTimeSpan.GetDuration().GetSecondDouble() * ResampleRate);
	const FbxTime TimeIncrement = AnimTimeSpan.GetDuration() / YMath::Max(NumSamplingKeys, 1);

	int mesh_index = 0;
	for (SkinMesh& skin_mesh : skin_data->meshes_)
	{
		FbxNode* geo_node = mesh_contain_skeleton_and_bs[mesh_index];
		FbxMesh* mesh = geo_node->GetMesh();
		if (!skin_mesh.bs_.target_shapes_.empty())
		{
			BlendShapeSequneceTrack& bs_sequence_track = anim_data->bs_sequence_track[mesh_index];
			for (FbxTime CurTime = AnimTimeSpan.GetStart(); CurTime <= AnimTimeSpan.GetStop(); CurTime += TimeIncrement)
			{
				int lBlendShapeDeformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
				for (int blend_shape_index = 0; blend_shape_index < lBlendShapeDeformerCount; ++blend_shape_index)
				{
					FbxBlendShape* lBlendShape = (FbxBlendShape*)mesh->GetDeformer(blend_shape_index, FbxDeformer::eBlendShape);

					int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
					for (int channel_index = 0; channel_index < lBlendShapeChannelCount; ++channel_index)
					{
						FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(channel_index);
						if (lChannel)
						{
							// Get the percentage of influence on this channel.
							FbxAnimCurve* curve = mesh->GetShapeChannel(blend_shape_index, channel_index, current_animation_layer);
							std::string channel_name = lChannel->GetName();
							if (!curve)
							{
								bs_sequence_track.value_curve_[channel_name].push_back(0.0);
							}
							else
							{
								double lWeight = curve->Evaluate(CurTime);
								bs_sequence_track.value_curve_[channel_name].push_back(lWeight);
							}
						}
					}
				}
			}
		}
		mesh_index++;
	}
	return true;
}
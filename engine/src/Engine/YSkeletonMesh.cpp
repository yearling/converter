#include "Engine/YSkeletonMesh.h"
#include "Engine/YCanvasUtility.h"
#include "Engine/YCanvas.h"
YBone::YBone()
{
	bind_local_tranform_ = YTransform::identity;
	bind_local_matrix_ = bind_local_tranform_.ToMatrix();
	bone_id_ = -1;
	parent_id_ = -1;
	fist_init = false;
}

YSkeleton::YSkeleton()
{
	root_bone_id_ = -1;
}

void YSkeleton::Update(double delta_time)
{
	if (root_bone_id_ != -1)
	{
		UpdateRecursive(delta_time,root_bone_id_);
	}
}

void YSkeleton::UpdateRecursive(double delta_time,int bone_id)
{
	YBone& cur_bone = bones_[bone_id];

	if (cur_bone.parent_id_ != -1)
	{
		int parent_bone_id = cur_bone.parent_id_;
		YBone& parent_bone = bones_[parent_bone_id];
		cur_bone.global_transform_ = cur_bone.local_transform_ * parent_bone.global_transform_;
		cur_bone.global_matrix_ = cur_bone.local_matrix_ * parent_bone.global_matrix_;
	}
	else
	{
		cur_bone.global_transform_ = cur_bone.local_transform_;
		cur_bone.global_matrix_ = cur_bone.local_matrix_;
	}

	for (int child_bone_id : cur_bone.children_)
	{
		UpdateRecursive(delta_time, child_bone_id);
	}
}

YSkeletonMesh::YSkeletonMesh()
{
	play_time = 0.0;
}

void YSkeletonMesh::Update(double delta_time)
{
	play_time += delta_time;
	if (animation_data_)
	{
		if (play_time > animation_data_->time_) 
		{
			play_time = 0.0;
		}
		int current_frame = int(play_time * 30.0f);

		for (YBone& bone : skeleton_->bones_)
		{
			AnimationSequenceTrack& track = animation_data_->sequence_track[bone.bone_name_];
			int key_size = track.pos_keys_.size();
			current_frame = current_frame % key_size;
			YVector pos = track.pos_keys_[current_frame];
			YQuat rot = track.rot_keys_[current_frame];
			YVector scale = track.scale_keys_[current_frame];
			YTransform local_trans = YTransform(pos, rot, scale);
			bone.local_transform_ = local_trans;
			bone.local_matrix_ = local_trans.ToMatrix();
		}
	}
	skeleton_->Update(delta_time);
	for (int i = 0; i < skeleton_->bones_.size(); ++i) {
	    YBone& bone = skeleton_->bones_[i];
		YVector joint_center = bone.global_transform_.TransformPosition(YVector(0, 0, 0));
		g_Canvas->DrawCube(joint_center, YVector4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f);
		if (bone.parent_id_ != -1) {
			YVector parent_joint_center = skeleton_->bones_[bone.parent_id_].global_transform_.TransformPosition(YVector(0, 0, 0));
			g_Canvas->DrawLine(parent_joint_center, joint_center, YVector4(0.0f, 1.0f, 0.0f, 1.0f));
		}
	}
}

void YSkeletonMesh::Render()
{

}

AnimationData::AnimationData()
{

}

#include "Math/YBox.h"

YBox::YBox(const YVector* pointer, int count)
	:min_(YVector::zero_vector)
	,max_(YVector::zero_vector)
	, is_valid_(0)
{
	for (int i = 0; i < count; ++i)
	{
		*this += pointer[i];
	}
}

YBox::YBox(const std::vector<YVector>& points)
	:min_(YVector::zero_vector)
	, max_(YVector::zero_vector)
	, is_valid_(0)
{
	for (int i = 0; i < (int)points.size(); ++i)
	{
		*this += points[i];
	}
}

YBox& YBox::operator+=(const YVector& other)
{
	if (is_valid_)
	{
		min_.x = YMath::Min(min_.x, other.x);
		min_.y = YMath::Min(min_.y, other.y);
		min_.z = YMath::Min(min_.z, other.z);

		max_.x = YMath::Max(max_.x, other.x);
		max_.y = YMath::Max(max_.y, other.y);
		max_.z = YMath::Max(max_.z, other.z);
	}
	else
	{
		min_ = max_ = other;
		is_valid_ = 1;
	}
	return *this;
}

YBox& YBox::operator+=(const YBox& other)
{
	if (is_valid_ && other.is_valid_)
	{
		min_.x = YMath::Min(min_.x, other.min_.x);
		min_.y = YMath::Min(min_.y, other.min_.y);
		min_.z = YMath::Min(min_.z, other.min_.z);

		max_.x = YMath::Max(max_.x, other.max_.x);
		max_.y = YMath::Max(max_.y, other.max_.y);
		max_.z = YMath::Max(max_.z, other.max_.z);
	}
	else if (other.is_valid_)
	{
		*this = other;
	}
	return *this;
}

YBox YBox::TransformBy(const YMatrix& m) const
{
	if (!is_valid_)
	{
		return YBox(EForceInit::ForceInitToZero);
	}
	YVector ori_center = GetCenter();
	YVector ori_extent = GetExtent();
	YVector new_center = m.TransformPosition(ori_center);
	YVector new_extent = m.TransformVector(ori_extent).GetAbs();
	YBox new_box(new_center-new_extent,new_center+ new_extent);
	return new_box;
}

YBox YBox::TransformBy(const YTransform& m) const
{
	return	TransformBy(m.ToMatrix());
}

bool YBox::Intersect(const YBox& other) const
{
	if ((min_.x > other.max_.x) || (max_.x < other.min_.x))
	{
		return false;
	}
	if ((min_.y > other.max_.y) || (max_.y < other.min_.y))
	{
		return false;
	}
	if ((min_.z > other.max_.z) || (max_.z < other.min_.z))
	{
		return false;
	}

	return true;
	
}

YBox YBox::Overlap(const YBox& other) const
{
	if (!Intersect(other))
	{
		static YBox empty_box(ForceInitToZero);
		return empty_box;
	}

	YVector min_vector, max_vector;
	min_vector.x = YMath::Max(min_.x, other.min_.x);
	max_vector.x = YMath::Min(max_.x, other.max_.x);

	min_vector.y = YMath::Max(min_.y, other.min_.y);
	max_vector.y = YMath::Min(max_.y, other.max_.y);

	min_vector.z = YMath::Max(min_.z, other.min_.z);
	max_vector.z = YMath::Min(max_.z, other.max_.z);

	return YBox(min_vector, max_vector);
}

YArchive& operator<<(YArchive& mem_file, YBox& box)
{
    mem_file << box.min_;
    mem_file << box.max_;
    mem_file << box.is_valid_;
    return mem_file;
}

#pragma once
#include <stdint.h>
#include <vector>
#include "Math/YVector.h"
#include "Engine/YLog.h"
#include "Math/YTransform.h"
#include "Math/YMatrix.h"
struct YBox
{
public:
	YVector min_;
	YVector max_;
	uint8_t is_valid_;
public:
	YBox():is_valid_(0){}
	explicit YBox(EForceInit)
	{
		Init();
	}

	YBox(const YVector& in_min, const YVector& in_max)
	:min_(in_min),max_(in_max),is_valid_(1){

	}

	inline void Init()
	{
		min_ = max_ = YVector::zero_vector;
		is_valid_ = 0;
	}

	YBox(const YVector* pointer, int count);
	YBox(const std::vector<YVector>& points);
	inline static YBox BuildAABB(const YVector& origin, const YVector& extent)
	{
		YBox new_box(origin - extent, origin + extent);
		return new_box;
	}
public:
	inline bool operator==(const YBox& other) const
	{
		return (min_ == other.min_) && (max_ == other.max_);
	}

	YBox& operator+=(const YVector& other);

	inline YBox operator+(const YVector& other)
	{
		return YBox(*this) += other;
	}

	YBox& operator+=(const YBox& other);

	inline YBox operator+(const YBox& other)
	{
		return YBox(*this) += other;
	}
	inline YVector& operator[](int index)
	{
		assert(0 <= index && index < 2);
		if (index == 0)
		{
			return min_;
		}
		return max_;
	}

	inline YVector GetCenter() const
	{
		return YVector((min_ + max_) * 0.5f);
	}

	inline YVector GetExtent() const
	{
		return 0.5f * (max_ - min_);
	}
	inline bool IsInside(const YVector& in) const
	{
		return ((min_.x < in.x) && (in.x < max_.x) && (min_.y < in.y) && (in.y < max_.y) && (min_.z < in.z) && (in.z < min_.z));
	}

	inline bool IsInside(const YBox& other) const
	{
		return (IsInside(other.min_) && IsInside(other.max_));
	}
	
public:
	YBox TransformBy(const YMatrix& m) const;
	YBox TransformBy(const YTransform& m) const;
	bool Intersect(const YBox& other) const;
	// return the overlap YBox of two box
	YBox Overlap(const YBox& other)const;
};

namespace std
{
	template<>
	struct is_pod<YBox>
	{
		static constexpr bool  value = true;
	};
}

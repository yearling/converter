#pragma once
#include <deque>
#include <numeric>

template<typename T>
struct AverageSmooth
{
public:
	AverageSmooth(int in_sample_num) :average_num_(in_sample_num) {};
	AverageSmooth() {};
	float Average() const
	{
		T acc = std::accumulate(sequence.begin(), sequence.end(), (T)0);
		if (sequence.empty())
		{
			return 0.f;
		}
		return acc / (float)sequence.size();
	}
	void SmoothAcc(T v)
	{
		while (sequence.size() > average_num_)
		{
			sequence.pop_front();
		}
		sequence.push_back(v);
	}
	void SetAverageNum(int n) { average_num_ = n; }
protected:
	std::deque<T> sequence;
	int average_num_ = 20;
};
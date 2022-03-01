#pragma once
class YNoncopyable
{
protected:
	// ensure the class cannot be constructed directly
	YNoncopyable() {}
	// the class should not be used polymorphically
	~YNoncopyable() {}
private:
	YNoncopyable(const YNoncopyable&) =default;
	YNoncopyable& operator=(const YNoncopyable&) =default;
};


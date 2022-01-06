#pragma once
struct YViewPort
{
public:
	YViewPort();
	YViewPort(int width, int height) :width_(width), height_(height),left_top_x(0),left_top_y(0) {}
	int width_;
	int height_;
	int left_top_x;
	int left_top_y;
	int GetWidth() { return width_; }
	int GetHeight() { return height_; }

};
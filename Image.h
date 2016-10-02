#pragma once

#include <stdint.h>
#include <algorithm>

struct Point
{
	int x, y;

	void Set(int x, int y) {
		this->x = x;
		this->y = y;
	}
};

struct Rect
{
	int x, y, w, h;
	
	int Left() const {
		return x;
	}
	
	int Right() const {
		return x + w;
	}
	
	int Top() const {
		return y;
	}
	
	int Bottom() const {
		return y + h;
	}
	
	void Clear() {
		x = y = w = h = 0;
	}

	void Set(int x, int y, int w, int h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
	
	void Union(const Rect& rect) {
		if (rect.w < 0 || rect.h < 0) {
			return;
		}
		if (w == 0 || h == 0) {
			*this = rect;
			return;
		}
		int left = (std::min)(Left(), rect.Left());
		int top = (std::min)(Top(), rect.Top());
		int right = (std::max)(Right(), rect.Right());
		int bottom = (std::max)(Bottom(), rect.Bottom());
		Set(left, top, right - left, bottom - top);
	}

	RECT GetRECT() const {
		RECT r;
		r.left = Left();
		r.right = Right();
		r.top = Top();
		r.bottom = Bottom();
		return r;
	}

	bool IsHit(int x, int y) const {
		return x >= Left() && x < Right() && y >= Top() && y < Bottom();
	}
};

static inline
bool operator == (const Rect& a, const Rect& b) {
	return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

struct Image
{
	Image();
	~Image();
	
	int width;
	int height;
	int lineStride;
	
	void* pixels;
	enum {
		PixelType_Unknown,
		PixelType_Gray8,
		PixelType_GrayAlpha16,
		PixelType_BGR24,
		PixelType_BGRA32,
	} pixelType;
	
	bool loaded;
	bool LoadFromFile(const char* filename);
	bool Unload();	
	
};

void Draw(
	Image& dst, int dx, int dy,
	const Image& src, int sx, int sy, int sw, int sh,
	Rect& updatedRect
);

void FillRect(
	Image& dst, int x, int y, int w, int h, uint32_t color,
	Rect& updatedRect
);

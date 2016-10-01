#pragma once

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
	const Image& src, int sx, int sy, int sw = -1, int sh = -1
);


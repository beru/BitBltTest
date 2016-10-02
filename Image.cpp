
#include <stdafx.h>
#include "Image.h"
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Image::Image()
	:
	width(0),
	height(0),
	lineStride(0),
	pixels(nullptr),
	pixelType(PixelType_Unknown),
	loaded(false)
{
	
	
}

Image::~Image()
{
	if (loaded) {
		Unload();
	}
}

bool Image::LoadFromFile(const char* filename)
{
	if (loaded) {
		return false;
	}
	int numComponents;
	pixels = stbi_load(filename, &width, &height, &numComponents, 4 /* 0 */);
	if (!pixels) {
		return false;
	}
	switch (numComponents) {
	case 1:
		pixelType = PixelType_Gray8;
		break;
	case 2:
		pixelType = PixelType_GrayAlpha16;
		break;
	case 3:
		pixelType = PixelType_BGR24;
		break;
	case 4:
		pixelType = PixelType_BGRA32;
		break;
	default:
		return false;
	}
	loaded = true;
	lineStride = width * numComponents;
	return true;
}

bool Image::Unload()
{
	if (!loaded) {
		return false;
	}
	stbi_image_free(pixels);
	return true;
}

void Draw(
	Image& dst, int dx, int dy,
	const Image& src, int sx, int sy, int sw, int sh,
	Rect& updatedRect
	)
{
	updatedRect.Clear();

	// out of bounds
	if (0
		|| dx > dst.width
		|| dy > dst.height
		|| sx > src.width
		|| sy > src.height
		|| sw == 0
		|| sh == 0
	) {
		return;
	}
	
	// auto width
	if (sw == -1) {
		sw = src.width;
	}
	// auto height
	if (sh == -1) {
		sh = src.height;
	}
	
	// coordinates normalization
	if (dx < 0) {
		sx -= dx;
		sw += dx;
		dx = 0;
	}
	if (dy < 0) {
		sy -= dy;
		sh += dy;
		dy = 0;
	}
	if (sx < 0) {
		sw += sx;
		sx = 0;
	}
	if (sy < 0) {
		sh += sy;
		sy = 0;
	}

	// coordinates clipping
	if (sx + sw > src.width) {
		sw = src.width - sx;
	}
	if (sy + sh > src.height) {
		sh = src.height - sy;
	}
	if (dx + sw > dst.width) {
		sw = dst.width - dx;
	}
	if (dy + sh > dst.height) {
		sh = dst.height - dy;
	}
	
	if (dst.pixelType != Image::PixelType_BGRA32
		|| src.pixelType != Image::PixelType_BGRA32
	) {
		// TODO: support various pixel types
		return;
	}
	
	uint32_t* pdst = (uint32_t*) dst.pixels;
	const uint32_t* psrc = (const uint32_t*) src.pixels;
	
	pdst = (uint32_t*)((uint8_t*)pdst + dy * dst.lineStride) + dx;
	psrc = (const uint32_t*)((const uint8_t*)psrc + sy * src.lineStride) + sx;
	for (int y=0; y<sh; ++y) {
		for (int x=0; x<sw; ++x) {
			pdst[x] = psrc[x];
		}
		pdst = (uint32_t*)((uint8_t*)pdst + dst.lineStride);
		psrc = (const uint32_t*)((const uint8_t*)psrc + src.lineStride);
	}

	updatedRect.Set(dx, dy, sw, sh);
}


void FillRect(
	Image& dst, int x, int y, int w, int h, uint32_t color,
	Rect& updatedRect
	)
{
	updatedRect.Clear();

	// coordinates normalization
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	// coordinates clipping
	if (x + w > dst.width) {
		w = dst.width - x;
	}
	if (y + h > dst.height) {
		h = dst.height - y;
	}

	if (w <= 0 || h <= 0) {
		return;
	}
	
	uint32_t* pdst = (uint32_t*) dst.pixels;
	pdst = (uint32_t*)((uint8_t*)pdst + y * dst.lineStride) + x;
	for (int y=0; y<h; ++y) {
		for (int x=0; x<w; ++x) {
			pdst[x] = color;
		}
		pdst = (uint32_t*)((uint8_t*)pdst + dst.lineStride);
	}

	updatedRect.Set(x, y, w, h);
}


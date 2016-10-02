
#include "stdafx.h"

#include <Windows.h>
#include <mmsystem.h>
#pragma comment( lib, "winmm.lib")

#include <vector>
#include <assert.h>
#include "Image.h"

namespace {

HWND hWnd;
HBITMAP hBMP;
std::vector<uint8_t> bmiBuff(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
BITMAPINFO* pBMI;
void* pBits;
HDC hMemDC;
HFONT hFont;

enum {
	TimerID_Draw,
};

const int FPS = 60;
int frameCount = 0;

enum ImgID {
	Dest,
	Star,
	Button,

	MaxID,
};
std::vector<Image*> images;

struct Button
{
	int width;
	int height;

	struct ImageRef {
		Image* img;
		Point pos;
	};

	enum State {
		Normal,
		MouseOver,
		MouseDown,

		StateMax
	} state;

	ImageRef images[StateMax];
};

struct RenderBlock
{
	Rect rect;
	int state;
};

enum RenderBlockId {
	RBID_Background,
	RBID_Star,
	RBID_Button0,
	RBID_Button1,

	RBID__Max,
};

RenderBlock renderBlocks[RBID__Max][2];

void UpdateRenderBlock(RenderBlockId id, const Rect& rect, int state = 0) {
	RenderBlock& rb = renderBlocks[id][frameCount & 1];
	rb.rect = rect;
	rb.state = state;
}

Rect GetBitBltRect() {
	Rect rect;
	rect.Clear();
	for (size_t i = 0; i < RBID__Max; ++i) {
		auto& a = renderBlocks[i][0];
		auto& b = renderBlocks[i][1];
		if (a.rect == b.rect) {
			if (a.state != b.state) {
				rect.Union(a.rect);
			}
		}
		else {
			rect.Union(a.rect);
			rect.Union(b.rect);
		}
	}
	return rect;
}

} // anonymous namespace

void OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	pBMI = (BITMAPINFO*) &bmiBuff[0];
	BITMAPINFO& bmi = *pBMI;
	
	int width = GetSystemMetrics(SM_CXFULLSCREEN);
	int height = GetSystemMetrics(SM_CYFULLSCREEN);
	int bitsPerPixel = 32;
	int bytesPerPixel = bitsPerPixel / 8;
	width = (width + 3) & (~3);
	BITMAPINFOHEADER& header = bmi.bmiHeader;
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = -height;	// TopDown
	header.biPlanes = 1;
	header.biBitCount = bitsPerPixel;
	header.biCompression = BI_RGB;
	header.biSizeImage = width * abs(height) * bytesPerPixel;
	header.biXPelsPerMeter = 0;
	header.biYPelsPerMeter = 0;
	header.biClrUsed = 0;
	header.biClrImportant = 0;

	hBMP = ::CreateDIBSection(
		(HDC)0,
		&bmi,
		DIB_RGB_COLORS,
		&pBits,
		NULL,
		0
	);

	HDC hWndDC = ::GetDC(hWnd);
	hMemDC = ::CreateCompatibleDC(hWndDC);
	::SetMapMode(hMemDC, ::GetMapMode(hWndDC));
	::ReleaseDC(hWnd, hWndDC);
	::SelectObject(hMemDC, hBMP);

	LOGFONT lf = {0};
	lf.lfHeight = -MulDiv(12, GetDeviceCaps(hMemDC, LOGPIXELSY), 72);
	lf.lfQuality = ANTIALIASED_QUALITY;
	hFont = ::CreateFontIndirect(&lf);
	::SelectObject(hMemDC, hFont);

	struct ImgDef {
		ImgID id;
		const char* path;
	};
	
	ImgDef defs[] = {
		ImgID::Star,	"star.png",
		ImgID::Button,	"btn.png",

	};

	images.resize(MaxID);
	for (auto def : defs) {
		auto img = new Image();
		if (!img->LoadFromFile(def.path)) {
			delete img;
			assert(false);
		}
		images[def.id] = img;
	}

	auto pDst = new Image();
	images[Dest] = pDst;
	auto& dst = *pDst;
	dst.width = pBMI->bmiHeader.biWidth;
	dst.height = abs(pBMI->bmiHeader.biHeight);
	dst.lineStride = dst.width * 4;
	dst.pixelType = Image::PixelType_BGRA32;
	dst.pixels = pBits;

	memset(&renderBlocks, 0xFF, sizeof(renderBlocks));

	::SetTimer(hWnd, TimerID_Draw, 1000 / FPS, 0);
}

void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	::KillTimer(hWnd, TimerID_Draw);
	::DeleteDC(hMemDC);
	::DeleteObject(hBMP);
	::DeleteObject(hFont);

	for (auto img : images) {
		delete img;
	}
}

void OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	hdc = BeginPaint(hWnd, &ps);
	
	const RECT& rec = ps.rcPaint;
	::BitBlt(
		hdc,
		rec.left,
		rec.top,
		rec.right - rec.left,
		rec.bottom - rec.top,
		hMemDC,
		rec.left,
		rec.top,
		SRCCOPY
	);
	
	EndPaint(hWnd, &ps);
}

void OnMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	
}

void OnMouseUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	
}

void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	
}

inline DWORD getTime()
{
	::timeBeginPeriod(1);
	DWORD ret = ::timeGetTime();
	::timeEndPeriod(1);
	return ret;
}

inline void draw(int dx, int dy, ImgID id, int sx, int sy, int sw, int sh, Rect& updated)
{
	Image& dst = *images[Dest];
	const Image& src = *images[id];
	::Draw(dst, dx, dy, src, sx, sy, sw, sh, updated);
}


void OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!IsWindow(hWnd)) {
		return;
	}

	DWORD now = getTime();

#if 0
	if (frameCount) {
		return;
	}
#endif

	Rect updated;

	Image& dst = *images[Dest];
	FillRect(dst, 0, 0, dst.width, dst.height, RGB(150, 50, 50), updated);
	UpdateRenderBlock(RBID_Background, updated);

	int offset = +(now % 10000) / 10;
	int xc = 200 +offset / 2;
	int yc = 200 + offset;
	draw(xc, yc, ImgID::Star, 0, 0, 200, 200, updated);
	UpdateRenderBlock(RBID_Star, updated);

	++frameCount;

	updated = GetBitBltRect();
	RECT r = updated.GetRECT();
#if 0
	InvalidateRect(hWnd, NULL, FALSE);
#else
	InvalidateRect(hWnd, &r, FALSE);
#endif
}


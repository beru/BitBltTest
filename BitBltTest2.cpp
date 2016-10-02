
#include "stdafx.h"

#include <Windows.h>
#include <mmsystem.h>
#include <Windowsx.h>

#pragma comment( lib, "winmm.lib")
#pragma comment( lib, "Imm32.lib")

#include <vector>
#include <assert.h>
#include "Image.h"
#include "trace.h"

namespace {

enum TimerId {
	TimerId_Draw,
};

enum ImgId {
	ImgId_Dest,
	ImgId_Star,
	ImgId_Button,

	ImgId_MaxId,
};

struct Button
{
	Rect rect;

	struct ImageRef {
		const Image* img;
		Point pos;

		void Set(const Image* img, int x, int y) {
			this->img = img;
			pos.x = x;
			pos.y = y;
		}
	};

	enum State {
		State_Normal,
		State_MouseOver,
		State_MouseDown,

		State_Max
	} state;

	ImageRef images[State_Max];
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

int frameCount = 0;
RenderBlock renderBlocks[RBID__Max][2];

void UpdateRenderBlock(int id, const Rect& rect, int state = 0) {
	RenderBlock& rb = renderBlocks[id][frameCount & 1];
	rb.rect = rect;
	rb.state = state;
}

Rect GetBitBltRect() {
	Rect rect;
	rect.Clear();
	// Combine dirty rectangles
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


HWND hWnd;
HBITMAP hBMP;
std::vector<uint8_t> bmiBuff(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
BITMAPINFO* pBMI;
void* pBits;
HDC hMemDC;
HFONT hFont;
const int FPS = 60;
std::vector<Image*> images;
Button btn0;
Button btn1;

Button* btns[] = {
	&btn0,
	&btn1
};

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
		ImgId id;
		const char* path;
	};
	
	ImgDef defs[] = {
		ImgId_Star,	"star.png",
		ImgId_Button,	"btn.png",

	};

	images.resize(ImgId_MaxId);
	for (auto def : defs) {
		auto img = new Image();
		if (!img->LoadFromFile(def.path)) {
			delete img;
			assert(false);
		}
		images[def.id] = img;
	}

	auto pDst = new Image();
	images[ImgId_Dest] = pDst;
	auto& dst = *pDst;
	dst.width = pBMI->bmiHeader.biWidth;
	dst.height = abs(pBMI->bmiHeader.biHeight);
	dst.lineStride = dst.width * 4;
	dst.pixelType = Image::PixelType_BGRA32;
	dst.pixels = pBits;

	auto btnImg = images[ImgId_Button];

	btn0.rect.Set(500, 100, 300, 100);
	btn0.images[Button::State_Normal].Set(btnImg, 0, 0);
	btn0.images[Button::State_MouseOver].Set(btnImg, 0, 100);
	btn0.images[Button::State_MouseDown].Set(btnImg, 0, 200);

	btn1 = btn0;
	btn1.rect.x = 1100;

	memset(&renderBlocks, 0xFF, sizeof(renderBlocks));

	ImmDisableIME(-1);

	::SetTimer(hWnd, TimerId_Draw, 1000 / FPS, 0);
}

void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	::KillTimer(hWnd, TimerId_Draw);
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

	TRACE(_T("%d %d %d %d\n"), rec.left, rec.top, rec.right - rec.left, rec.bottom - rec.top);

}

void OnMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (wParam & MK_LBUTTON) {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		for (size_t i = 0; i<_countof(btns); ++i) {
			auto btn = btns[i];
			if (btn->rect.IsHit(x, y)) {
				btn->state = Button::State_MouseDown;
				::SetCapture(hWnd);
				return;
			}
		}
	}
}

void OnMouseUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
		if (btn->state == Button::State_MouseDown) {
			if (btn->rect.IsHit(x, y)) {
				btn->state = Button::State_MouseOver;
			}else {
				btn->state = Button::State_Normal;
			}
			break;
		}
	}
	::ReleaseCapture();
}

void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
		if (btn->state == Button::State_MouseDown) {
			return;
		}
	}
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
		if (btn->rect.IsHit(x, y)) {
			btn->state = Button::State_MouseOver;
		}else {
			btn->state = Button::State_Normal;
		}
	}
}

inline DWORD getTime()
{
	::timeBeginPeriod(1);
	DWORD ret = ::timeGetTime();
	::timeEndPeriod(1);
	return ret;
}

inline bool IsKeyDown(int vk)
{
	return GetAsyncKeyState(vk) < 0;
}

inline bool IsMouseDown() {
	return IsKeyDown(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON);
}

void OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!IsWindow(hWnd)) {
		return;
	}

	POINT cursorPos;
	::GetCursorPos(&cursorPos);
	::ScreenToClient(hWnd, &cursorPos);

	bool windowHasFocus = (GetFocus() == hWnd);
	bool isMouseDown = IsMouseDown();

	DWORD now = getTime();

#if 0
	if (frameCount) {
		return;
	}
#endif

	Rect updated;

	Image& dst = *images[ImgId_Dest];
	FillRect(dst, 0, 0, dst.width, dst.height, RGB(150, 50, 50), updated);
	UpdateRenderBlock(RBID_Background, updated);

	int offset = +(now % 10000) / 10;
	int xc = 200 +offset / 2;
	int yc = 200 + offset;
	Image& star = *images[ImgId_Star];
	Draw(dst, xc, yc, star, 0, 0, 200, 200, updated);
	UpdateRenderBlock(RBID_Star, updated);

	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
		Button::ImageRef& imgRef = btn->images[btn->state];
		Draw(dst, btn->rect.x, btn->rect.y, *imgRef.img, imgRef.pos.x, imgRef.pos.y, btn->rect.x, btn->rect.y, updated);
		UpdateRenderBlock(RBID_Button0 + i, updated, btn->state);
	}

	++frameCount;

	updated = GetBitBltRect();
	RECT r = updated.GetRECT();
#if 0
	InvalidateRect(hWnd, NULL, FALSE);
#else
	InvalidateRect(hWnd, &r, FALSE);
#endif
}



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

#define TIMER_METHOD_SetTimer 1
#define TIMER_METHOD_timeSetEvent 2
#define TIMER_METHOD_CreateTimerQueueTimer 3

//#define TIMER_METHOD TIMER_METHOD_SetTimer
#define TIMER_METHOD TIMER_METHOD_timeSetEvent
//#define TIMER_METHOD TIMER_METHOD_CreateTimerQueueTimer

namespace {

enum TimerId {
	TimerId_UpdateFrame,
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

		State_Num
	} state;

    bool stateChanged;

	ImageRef images[State_Num];
};

int frameCount = 0;

// Of course we shouldn't use many global variables in production code...(°ロ°)☝
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

#if TIMER_METHOD == TIMER_METHOD_timeSetEvent

MMRESULT timerId;

void timeSetEvent_Callback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    HWND hWnd = (HWND)dwUser;
    ::PostMessage(hWnd, WM_TIMER, TimerId_UpdateFrame, 0);
}

#elif TIMER_METHOD == TIMER_METHOD_CreateTimerQueueTimer

HANDLE hTimerQueue;

void CreateTimerQueueTimer_Callback(void* pParameter, BOOLEAN TimerOrWaitFired)
{
    HWND hWnd = (HWND)pParameter;
    ::PostMessage(hWnd, WM_TIMER, TimerId_UpdateFrame, 0);
}

#endif

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
    btn0.state = Button::State_Normal;
    btn0.stateChanged = true;

	btn1 = btn0;
	btn1.rect.x = 1100;
    btn1.state = Button::State_Normal;
    btn1.stateChanged = true;

	ImmDisableIME(-1);

    // http://omeg.pl/blog/2011/11/on-winapi-timers-and-their-resolution/
#if TIMER_METHOD == TIMER_METHOD_SetTimer
    ::SetTimer(hWnd, TimerId_UpdateFrame, 1000 / FPS, 0);
	::SendMessage(hWnd, WM_TIMER, TimerId_UpdateFrame, 0);
#elif TIMER_METHOD == TIMER_METHOD_timeSetEvent
    timerId = ::timeSetEvent(1000/FPS, 0, timeSetEvent_Callback, (DWORD_PTR)hWnd, TIME_PERIODIC|TIME_KILL_SYNCHRONOUS);
	::SendMessage(hWnd, WM_TIMER, TimerId_UpdateFrame, 0);
#elif TIMER_METHOD == TIMER_METHOD_CreateTimerQueueTimer
    if (::CreateTimerQueueTimer(&hTimerQueue, NULL, CreateTimerQueueTimer_Callback, hWnd, 0, 1000/FPS, WT_EXECUTEDEFAULT) == 0) {
        TRACE("CreateTimerQueueTimer failed %d\n", ::GetLastError());
    }
#else
#endif
}

void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#if TIMER_METHOD == TIMER_METHOD_SetTimer
	::KillTimer(hWnd, TimerId_UpdateFrame);
#elif TIMER_METHOD == TIMER_METHOD_timeSetEvent
    ::timeKillEvent(timerId);
#elif TIMER_METHOD == TIMER_METHOD_CreateTimerQueueTimer
    ::DeleteTimerQueueTimer(NULL, hTimerQueue, NULL);
#endif
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

	//TRACE(_T("%d %d %d %d\n"), rec.left, rec.top, rec.right - rec.left, rec.bottom - rec.top);

}

void OnMouseDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (wParam & MK_LBUTTON) {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		for (size_t i = 0; i<_countof(btns); ++i) {
			auto btn = btns[i];
			if (btn->rect.IsHit(x, y)) {
				::SetCapture(hWnd);
                btn->stateChanged = true;
				btn->state = Button::State_MouseDown;
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
            Button::State nextState = btn->rect.IsHit(x, y) ? Button::State_MouseOver : Button::State_Normal;
            btn->stateChanged = true;
			btn->state = nextState;
			break;
		}
	}
	::ReleaseCapture();
}

void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
}

inline DWORD getTime()
{
	::timeBeginPeriod(1);
	DWORD ret = ::timeGetTime();
	::timeEndPeriod(1);
	return ret;
}

static inline
void updateFrame(HWND hWnd, POINT cursorPt, bool isMouseOver)
{
	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
		if (btn->state == Button::State_MouseDown) {
			return;
		}
	}
	for (size_t i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
        Button::State nextState = (isMouseOver && btn->rect.IsHit(cursorPt.x, cursorPt.y)) ? Button::State_MouseOver : Button::State_Normal;
        if (btn->state != nextState) {
            btn->stateChanged = true;
        }
    	btn->state = nextState;
	}
}

static inline
void draw(HWND hWnd)
{
	POINT cursorPos;
	::GetCursorPos(&cursorPos);
	::ScreenToClient(hWnd, &cursorPos);

	DWORD now = getTime();

    TRACE("%d\n", now);

#if 0
	if (frameCount) {
		return;
	}
#endif

	bool updated = false;
    Rect updatedRect;
    updatedRect.Clear();

	Image& dst = *images[ImgId_Dest];
    if (frameCount <= 1) {
	    FillRect(dst, 0, 0, dst.width, dst.height, RGB(150, 50, 50), updatedRect);
	    updated = true;
    }
#if 0
	int offset = +(now % 10000) / 10;
	int xc = 200 +offset / 2;
	int yc = 200 + offset;
	Image& star = *images[ImgId_Star];
	Draw(dst, xc, yc, star, 0, 0, 200, 200, updated);
	UpdateRenderBlock(RBID_Star, updated);
#endif

	for (int i = 0; i<_countof(btns); ++i) {
		auto btn = btns[i];
        if (frameCount <= 1 || btn->stateChanged) {
		    Button::ImageRef& imgRef = btn->images[btn->state];
            Rect updatedRect2;
		    Draw(dst, btn->rect.x, btn->rect.y, *imgRef.img, imgRef.pos.x, imgRef.pos.y, btn->rect.x, btn->rect.y, updatedRect2);
            updatedRect.Union(updatedRect2);
	        updated = true;
        }
        btn->stateChanged = false;
	}

	++frameCount;

#if 0
    InvalidateRect(hWnd, NULL, FALSE);
#else
    if (updated) {
        RECT r;
        r.left = updatedRect.x;
        r.top = updatedRect.y;
        r.right = updatedRect.x + updatedRect.w;
        r.bottom = updatedRect.y + updatedRect.h;
    	InvalidateRect(hWnd, &r, FALSE);
    }
#endif

}

void OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!IsWindow(hWnd)) {
		return;
	}

	if (wParam != TimerId_UpdateFrame) {
		return;
	}

    POINT pt;
    ::GetCursorPos(&pt);

    bool isMouseOver = (WindowFromPoint(pt) == hWnd);

    ::ScreenToClient(hWnd, &pt);

    updateFrame(hWnd, pt, isMouseOver);

	draw(hWnd);
}


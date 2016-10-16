#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#if defined(_DEBUG) || defined(DEBUG)

static void TRACE(const char* pszFormat, ...)
{
    va_list	argp;
    static char pszBuf[10240];	// so sorry-ass
    va_start(argp, pszFormat);
    vsprintf_s(pszBuf, _countof(pszBuf) - 1, pszFormat, argp);
    va_end(argp);
    OutputDebugStringA(pszBuf);
}

static void TRACE(const wchar_t* pszFormat, ...)
{
	va_list	argp;
	static wchar_t pszBuf[10240];	// so sorry-ass
	va_start(argp, pszFormat);
    vswprintf( pszBuf, _countof(pszBuf)-1, pszFormat, argp);
	va_end(argp);
	OutputDebugStringW( pszBuf);
}

#else
#define TRACE(...)

#endif

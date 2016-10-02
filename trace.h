#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#if defined(_DEBUG) || defined(DEBUG)

static void TRACE( LPCTSTR pszFormat, ...)
{
	va_list	argp;
	static TCHAR pszBuf[10240];	// so sorry-ass
	va_start(argp, pszFormat);
	_vstprintf( pszBuf, _countof(pszBuf)-1, pszFormat, argp);
	va_end(argp);
	OutputDebugString( pszBuf);
}

#else
#define TRACE(...)

#endif

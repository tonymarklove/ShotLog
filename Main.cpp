// Copyright © Tony Marklove.

#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0600
#include <windows.h>
#include <shellapi.h>
#include <exception>
#include <string>
#include <ctime>
#include "IL/il.h"

using std::wstring;

const int UM_SYSTRAY = WM_USER + 1;
const int IDM_EXIT = 0;
const int TIMER_ID = 1;


wstring tooltip;
wstring iconFile;
wstring savePath;
int timeInterval = 15 * 60 * 1000;;
bool trayVisible = true;


ILuint GenerateSingleImage(void)
{
    ILuint ImageName; // The image name to return.
    ilGenImages(1, &ImageName); // Grab a new image name.
    return ImageName; // Go wild with the return value.
}

void DeleteSingleImage(ILuint ImageName)
{
    ilDeleteImages(1, &ImageName); // Delete the image name.
    return;
}

BOOL SaveBitmapFile(HDC p_hDC)
{

	HBITMAP hBmp = (HBITMAP)GetCurrentObject( p_hDC, OBJ_BITMAP );

	BITMAPINFO stBmpInfo;
	stBmpInfo.bmiHeader.biSize = sizeof( stBmpInfo.bmiHeader );
	stBmpInfo.bmiHeader.biBitCount = 0;
	GetDIBits( p_hDC, hBmp, 0, 0, NULL, &stBmpInfo, DIB_RGB_COLORS );

	ULONG iBmpInfoSize;
	switch( stBmpInfo.bmiHeader.biBitCount )
	{
	case 24:
	iBmpInfoSize = sizeof(BITMAPINFOHEADER);
	break;
	case 16:
	case 32:
	iBmpInfoSize = sizeof(BITMAPINFOHEADER)+sizeof(DWORD)*3;
	break;
	default:
	iBmpInfoSize = sizeof(BITMAPINFOHEADER)
	+ sizeof(RGBQUAD)
	* ( 1 << stBmpInfo.bmiHeader.biBitCount );
	break;
	}
	
	PBITMAPINFO pstBmpInfo;
	if( iBmpInfoSize != sizeof(BITMAPINFOHEADER) )
	{
	pstBmpInfo = (PBITMAPINFO)GlobalAlloc
	( GMEM_FIXED | GMEM_ZEROINIT, iBmpInfoSize );
	PBYTE pbtBmpInfoDest
	= (PBYTE)pstBmpInfo;
	PBYTE pbtBmpInfoSrc
	= (PBYTE)&stBmpInfo;
	ULONG iSizeTmp
	= sizeof( BITMAPINFOHEADER );

	while( iSizeTmp-- )
	{
	*( ( pbtBmpInfoDest )++ ) = *( ( pbtBmpInfoSrc )++ );
	}
	}

	BITMAPFILEHEADER stBmpFileHder;
	stBmpFileHder.bfType = 0x4D42; // 'BM'
	stBmpFileHder.bfSize
	= sizeof(BITMAPFILEHEADER)
	+ sizeof(BITMAPINFOHEADER)
	+ iBmpInfoSize
	+ pstBmpInfo->bmiHeader.biSizeImage;
	stBmpFileHder.bfReserved1 = 0;
	stBmpFileHder.bfReserved2 = 0;
	stBmpFileHder.bfOffBits = sizeof(BITMAPFILEHEADER) + iBmpInfoSize;

	PBYTE pBits
	= (PBYTE)GlobalAlloc
	( GMEM_FIXED | GMEM_ZEROINIT
	, stBmpInfo.bmiHeader.biSizeImage );
	
	memset(pBits, 0xff, stBmpInfo.bmiHeader.biSizeImage);

	HBITMAP hBmpOld;
	HBITMAP hTmpBmp
	= CreateCompatibleBitmap
	( p_hDC
	, pstBmpInfo->bmiHeader.biWidth
	, pstBmpInfo->bmiHeader.biHeight );
	hBmpOld = (HBITMAP)SelectObject( p_hDC, hTmpBmp );
	GetDIBits
	( p_hDC, hBmp, 0, pstBmpInfo->bmiHeader.biHeight
	, (LPSTR)pBits, pstBmpInfo, DIB_RGB_COLORS );

	// Save image using DevIL.
	ILuint img = GenerateSingleImage();
	ilBindImage(img);
	
	PBYTE bitPtr = pBits;
	for (int y=0; y<pstBmpInfo->bmiHeader.biWidth; y++) {
		for (int x=0; x<pstBmpInfo->bmiHeader.biHeight; x++) {
			*(bitPtr+3) = 0xff;
			bitPtr += 4;
		}
	}
	
	ilTexImage(pstBmpInfo->bmiHeader.biWidth, pstBmpInfo->bmiHeader.biHeight, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, pBits);
	ilEnable(IL_FILE_OVERWRITE);
	ILenum Error;
	Error = ilGetError(); 
	ILubyte *Data = ilGetData();
	
	time_t epoch = time(0);
	tm* epochTime = localtime(&epoch);
	TCHAR timeStr[300];

	swprintf(timeStr, TEXT("%s%d-%02d-%02d@%02d-%02d-%02d.png"), savePath.c_str(),
		epochTime->tm_year+1900, epochTime->tm_mon+1, epochTime->tm_mday,
		epochTime->tm_hour, epochTime->tm_min, epochTime->tm_sec
	);
	
	ilSaveImage(timeStr);
	ilBindImage(0);
	DeleteSingleImage(img);

	SelectObject( p_hDC, hBmpOld );
	DeleteObject( hTmpBmp );
	GlobalFree( pstBmpInfo );
	GlobalFree( pBits );
	return TRUE;
}

class Menu {
	public:
		Menu() {
			if (!(menu = CreatePopupMenu())) {
				throw std::exception("Error creting menu.");
			}
			
			AppendMenu(menu, MF_STRING, IDM_EXIT, TEXT("E&xit"));
		}
		
		~Menu() {
			DestroyMenu(menu);
		}
		
		HMENU menu;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == UM_SYSTRAY) {
		if (lParam == WM_RBUTTONUP) {
			Menu menu;
			POINT point;
			GetCursorPos(&point);
			SetForegroundWindow(hwnd);
			TrackPopupMenuEx(menu.menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, point.x,
				point.y, hwnd, 0);
			PostMessage(hwnd, 0, 0, 0);
			return 0;
		}
	}
	else if (msg == WM_COMMAND) {
		if (LOWORD(wParam) == IDM_EXIT) {
			PostQuitMessage(0);
			return 0;
		}
	}
	else if (msg == WM_TIMER) {
		HWND desktopHWND = GetDesktopWindow();
		HDC desktopDC = GetDC(desktopHWND);
		SaveBitmapFile(desktopDC);
		ReleaseDC(desktopHWND, desktopDC);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

class WindowClass {
	public:
		WindowClass(HINSTANCE instance) : instance(instance) {
			WNDCLASSEX wc = {sizeof(WNDCLASSEX), 0, WindowProc, 0, 0, instance,
				LoadIcon(0,IDI_APPLICATION), LoadCursor(0,IDC_ARROW),
				(HBRUSH)COLOR_BACKGROUND, 0, className, 0};
			if (!RegisterClassEx(&wc)) throw std::exception("Error creating window class.");
		}
		
		~WindowClass() {
			UnregisterClass(className, instance);
		}
		
		const TCHAR* ID() const { return className; }
		
	private:
		static const TCHAR* className;
		HINSTANCE instance;
};

const TCHAR* WindowClass::className = TEXT("HiddenWindowClass");


class Window {
	public:
		Window(HINSTANCE instance, const WindowClass& wc) {
			if (!(handle = CreateWindow(wc.ID(), 0, 0, 0,0, 0,0,
				HWND_MESSAGE, 0, instance, 0)))
			{
				throw std::exception("Error creating window.");
			}
		}
		
		~Window() {
			DestroyWindow(handle);
		}
		
		const HWND Handle() const { return handle; }
		
	private:
		HWND handle;
};

class Icon {
	public:
		Icon(LPCTSTR iconFile, HINSTANCE instance) {
			if (!(icon = (HICON)LoadImage(instance, iconFile, IMAGE_ICON, 0, 0,
				LR_LOADFROMFILE)))
			{
				throw std::exception("Error loading icon.");
			}
		}
		
		~Icon() {
			DestroyIcon(icon);
		}
		
		HICON icon;
};

class IconPtr {
	public:
		IconPtr() : icon(0) {}
		void store(Icon* icon) {
			this->icon = icon;
		}
		~IconPtr() { if (icon) delete icon; }
		
		Icon* icon;
};

class NotifyIcon {
	public:
		NotifyIcon(UINT id, LPCTSTR tooltip, const LPCTSTR iconFile,
			HINSTANCE instance, const Window& win)
		:
			id(id),
			hwnd(win.Handle())
		{
			NOTIFYICONDATA nid = {sizeof(NOTIFYICONDATA), hwnd, id,
				NIF_MESSAGE | NIF_ICON, UM_SYSTRAY};

			try {
				icon.store(new Icon(iconFile, instance));
				nid.hIcon = icon.icon->icon;
			}
			catch(...) {
				nid.hIcon = LoadIcon(0,IDI_APPLICATION);
			}
			
			if (tooltip) {
				nid.uFlags |= NIF_TIP;
				wcscpy_s(nid.szTip, 64, tooltip);
			}
			
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
		
		~NotifyIcon() {
			NOTIFYICONDATA nid = {sizeof(NOTIFYICONDATA), hwnd, id};
			Shell_NotifyIcon(NIM_DELETE, &nid);
		}
		
	private:
		const UINT id;
		HWND hwnd;
		IconPtr icon;
};

class NotifyIconPtr {
	public:
		NotifyIconPtr() : icon(0) {}
		void store(NotifyIcon* icon) {
			this->icon = icon;
		}
		~NotifyIconPtr() { if (icon) delete icon; }
		
		NotifyIcon* icon;
};


bool ProcessCommandLine() {
	LPTSTR cmdLine = GetCommandLine();
	
	// Find the beginning of the text.
	while (iswspace(*cmdLine)) ++cmdLine;
	
	// First non white-space character is either a quote or part of the program
	// path.
	if (*cmdLine == '"') {
		// Find the closing double quote.
		do ++cmdLine; while (*cmdLine && *cmdLine != '"');
	}
	else if (*cmdLine == '\'') {
		// Find the closing single quote.
		do ++cmdLine; while (*cmdLine && *cmdLine != '\'');
	}
	else {
		// Just skip the program path.
		while (*cmdLine && !iswspace(*cmdLine)) ++cmdLine;
	}
	
	++cmdLine;
	
	// Next look for options. If the next character is not "-" there are no
	// options.
	while (*cmdLine) {
		while (iswspace(*cmdLine)) ++cmdLine;
		
		if (*cmdLine == '-') {
			++cmdLine;
			
			if (*cmdLine == 'i') {
				// Icon file name must be within double quotes.
				while (*cmdLine && *cmdLine != '"') ++cmdLine;
				++cmdLine;
				while (*cmdLine && *cmdLine != '"') iconFile += *cmdLine++;
			}
			else if (*cmdLine == 't') {
				// Tooltip must be within double quotes.
				while (*cmdLine && *cmdLine != '"') ++cmdLine;
				++cmdLine;
				while (*cmdLine && *cmdLine != '"') tooltip += *cmdLine++;
			}
			else if (*cmdLine == 's') {
				// Save path must be within double quotes.
				while (*cmdLine && *cmdLine != '"') ++cmdLine;
				++cmdLine;
				while (*cmdLine && *cmdLine != '"') savePath += *cmdLine++;
				
				if (savePath[savePath.length()] != '/' && savePath[savePath.length()] != '\\') {
					savePath += '/';
				}
			}
			else if (*cmdLine == 'd') {
				wstring delay;
				// Delay a number in seconds.
				while (*cmdLine && !iswspace(*cmdLine)) ++cmdLine;
				++cmdLine;
				while (*cmdLine && iswdigit(*cmdLine)) delay += *cmdLine++;
				timeInterval = _wtoi(delay.c_str());
				if (timeInterval < 10) {
					timeInterval = 10;
				}
				timeInterval *= 1000;
			}
			else if (*cmdLine == 'h') {
				// Hide tray icon.
				++cmdLine;
				trayVisible = false;
			}
			else {
				// Unsupported option.
				return false;
			}
		}
		else {
			// The rest is the save path.
			break;
		}
	
		++cmdLine;
	}
	
	return true;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmdLine, int)
try {
	ProcessCommandLine();
	WindowClass wc(instance);
	Window window(instance, wc);
	NotifyIconPtr ni;

	if (trayVisible) {
		ni.store(new NotifyIcon(0, tooltip.c_str(), iconFile.c_str(), instance, window));
	}

	ilInit();
	
	SetTimer(window.Handle(), TIMER_ID, timeInterval, 0);

	MSG msg;
	
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return static_cast<int>(msg.wParam);
}
catch(const std::exception& e) {
	MessageBoxA(0, e.what(), "Error", MB_OK);
	return -1;
}
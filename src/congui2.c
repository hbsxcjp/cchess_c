// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
 
#include <windows.h>
#include <stdio.h>
 
HWND hStatic1 = NULL;
const int IDT_TIMER1 = 0;
 
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
 
    switch (Message) {
 
        /* Upon destruction, tell the main thread to stop */
        case WM_DESTROY: {
            printf(".");
            PostQuitMessage(0);
            break;
        }            
 
        /* All other messages (a lot of them) are processed using default procedures */
        default:
            return DefWindowProc(hWnd, Message, wParam, lParam);
    }
    return 0;
}
 
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
 
    WCHAR buf[20];
    int Cur = 0x100;
 
    wsprintf(buf, L"%ld", Cur);
    SetWindowText(hStatic1, buf);
}
 
void SetFont(HWND hwnd, int iFont, WCHAR* FontName)//iFont height and Font name
{
    HDC hdc;
    HFONT hFont;
    LOGFONT lf;
 
    lf.lfHeight = iFont;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 400;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = GB2312_CHARSET;
    lf.lfOutPrecision = 3;
    lf.lfClipPrecision = 2;
    lf.lfQuality = 1;
    lf.lfPitchAndFamily = 2;
    lstrcpyW(lf.lfFaceName, FontName);
 
    hFont = CreateFontIndirect(&lf);
    if (hFont == NULL)
        MessageBox(hwnd, L"CreateFont failure", L"error", MB_ICONERROR);
    hdc = GetDC(hwnd);
    SelectObject(hdc, hFont);
    GetObject(hFont, sizeof(LOGFONT), &lf);
    SendMessage(hwnd, WM_SETFONT, WPARAM(hFont), 0);
    ReleaseDC(hwnd, hdc);
}
 
 
int main()
{
    WNDCLASS        wc;          // windows class
    HWND hWnd; /* A 'HANDLE', hence the H, or a pointer to our window */
    MSG msg; /* A temporary location for all messages */
 
    HINSTANCE hInstance= GetModuleHandle(NULL);
 
    wc.style = CS_HREDRAW | CS_VREDRAW;                         //窗口样式
    wc.lpfnWndProc = WndProc;                                         //窗口的回调函数
    wc.hInstance = hInstance;                                       //窗口实例句柄
    wc.cbClsExtra = 0;                                               //窗口结构体扩展：无
    wc.cbWndExtra = 0;                                               //窗口实例扩展：无
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);       //窗口背景颜色：白色
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);              //窗口图标
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);                  //窗口中的鼠标样式
    wc.lpszClassName = L"WindowClass";                                       //窗口结构体名称
    wc.lpszMenuName = NULL;                                            //主菜单名称：无
    /* register windows class */
    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, L"Register Window Failed.！", L"Error", MB_ICONSTOP);
    }
 
    hWnd = CreateWindowExW(WS_TILED,
        L"WindowClass",
        L"Console Window Example",
        WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
        GetSystemMetrics(SM_CXSCREEN) / 2 - 250 / 2, /* x */
        GetSystemMetrics(SM_CYSCREEN) / 2 - 330 / 2, /* y */
        250, /* width */
        330, /* height */
        NULL, NULL, hInstance, NULL);
 
    if (hWnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
 
    hStatic1 = CreateWindowExW(0, L"STATIC", L"Loading", WS_CHILD | WS_VISIBLE, 20, 20, 150, 125, hWnd, 0, 0, 0);
    WCHAR fontname[] = L"Arial";
    SetFont(hStatic1, 80, fontname);
 
    SetTimer(hWnd, IDT_TIMER1, 1000, TimerProc);
 
    // Show window  
    ShowWindow(hWnd, SW_SHOW);
 
    // Update window   
    UpdateWindow(hWnd);
 
    while (GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
        TranslateMessage(&msg); /* Translate key codes to chars if present */
        DispatchMessage(&msg); /* Send it to WndProc */
    }
    return msg.wParam;
 
}
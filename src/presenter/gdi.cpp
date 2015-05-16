/*
  Copyright (c) 2015 StarBrilliant <m13253@hotmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms are permitted
  provided that the above copyright notice and this paragraph are
  duplicated in all such forms and that any documentation,
  advertising materials, and other materials related to such
  distribution and use acknowledge that the software was developed by
  StarBrilliant.
  The name of StarBrilliant may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "gdi.h"
#include "../app.h"
#include "../config.h"
#include <cassert>
#include <cstdlib>
#include <map>
#include <vector>
#include <windows.h>

namespace dmhm {

struct GDIPresenterPrivate {
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static std::map<HWND, GDIPresenter *> hWndMap; // tree_map is enogh, not necessarily need a unordered_map
    Application *app = nullptr;
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
    HDC window_dc = nullptr;
    HDC buffer_dc = nullptr;
    HBITMAP buffer_bmp = nullptr;
    int32_t top; int32_t left; int32_t right; int32_t bottom;
    void get_stage_rect(GDIPresenter *pub);
    void create_buffer(GDIPresenter *pub);
    void do_paint(GDIPresenter *pub, uint32_t *bitmap, uint32_t width, uint32_t height);
};

GDIPresenter::GDIPresenter(Application *app) {
    p->app = app;
    p->hInstance = GetModuleHandleW(nullptr);

    //SetProcessDPIAwareness(Process_System_DPI_Aware);

    /* Register window class */
    WNDCLASSEXW wnd_class;
    wnd_class.cbSize = sizeof wnd_class;
    wnd_class.style = CS_HREDRAW | CS_VREDRAW;
    wnd_class.lpfnWndProc = p->WndProc;
    wnd_class.cbClsExtra = 0;
    wnd_class.cbWndExtra = 0;
    wnd_class.hInstance = p->hInstance;
    wnd_class.hIcon = reinterpret_cast<HICON>(LoadImageW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION), IMAGE_ICON, 0, 0, LR_SHARED));
    wnd_class.hCursor = reinterpret_cast<HCURSOR>(LoadImageW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_SHARED));
    wnd_class.hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH));
    wnd_class.lpszMenuName = nullptr;
    wnd_class.lpszClassName = L"com.starbrilliant.danmakuhime";
    wnd_class.hIconSm = nullptr;
    ATOM wnd_class_atom = RegisterClassExW(&wnd_class);
    if(wnd_class_atom == 0) {
        /* Failed to set window class */
        report_error("\xe8\xae\xbe\xe5\xae\x9a\xe7\xaa\x97\xe5\x8f\xa3\xe7\xb1\xbb\xe5\x9e\x8b\xe5\xa4\xb1\xe8\xb4\xa5");
        abort();
    }

    /* Create window */
    p->hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST,
        reinterpret_cast<LPCWSTR>(wnd_class_atom),
        L"\u5f39\u5e55\u59ec",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        p->hInstance,
        nullptr
    );
    if(!p->hWnd) {
        /* Failed to create window */
        report_error("\xe5\x88\x9b\xe5\xbb\xba\xe7\xaa\x97\xe5\x8f\xa3\xe5\xa4\xb1\xe8\xb4\xa5");
        abort();
    }
    p->hWndMap[p->hWnd] = this;

    p->get_stage_rect(this);
    p->create_buffer(this);

    ShowWindow(p->hWnd, SW_SHOW);
}

GDIPresenter::~GDIPresenter() {
    if(p->hWnd)
        DestroyWindow(p->hWnd);
    if(p->window_dc)
        ReleaseDC(p->hWnd, p->window_dc);
    if(p->buffer_dc)
        DeleteDC(p->buffer_dc);
    if(p->buffer_bmp)
        DeleteObject(p->buffer_bmp);
}

void GDIPresenter::report_error(const std::string error) {
    MessageBoxW(nullptr, utf8_to_wide(error, false).c_str(), nullptr, MB_ICONERROR);
}

void GDIPresenter::get_stage_size(uint32_t &width, uint32_t &height) {
    assert(p->right >= p->left && p->bottom >= p->top);
    width = p->right - p->left;
    height = p->bottom - p->top;
}

void GDIPresenter::paint_frame() {
    uint32_t width, height;
    get_stage_size(width, height);
    std::vector<uint32_t> bitmap_buffer(width * height);

    for(size_t i = 0; i < height; i++)
        for(size_t j = 0; j < width; j++)
            bitmap_buffer[i*width + j] = (i << 24) | (j << 16) | ((i+j) << 8) | 128;

    p->do_paint(this, bitmap_buffer.data(), width, height);
}

int GDIPresenter::run_loop() {
    for(;;) {
        paint_frame();
        MSG message;
        if(PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if(message.message == WM_QUIT)
                return 0;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

void GDIPresenterPrivate::get_stage_rect(GDIPresenter *pub) {
    HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
    if(!monitor) {
        /* Failed to retrieve screen size */
        pub->report_error("\xe8\x8e\xb7\xe5\x8f\x96\xe5\xb1\x8f\xe5\xb9\x95\xe5\xb0\xba\xe5\xaf\xb8\xe5\xa4\xb1\xe8\xb4\xa5");
        abort();
    }
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof monitor_info;
    if(!GetMonitorInfoW(monitor, &monitor_info)) {
        /* Failed to retrieve screen size */
        pub->report_error("\xe8\x8e\xb7\xe5\x8f\x96\xe5\xb1\x8f\xe5\xb9\x95\xe5\xb0\xba\xe5\xaf\xb8\xe5\xa4\xb1\xe8\xb4\xa5");
        abort();
    }
    top = monitor_info.rcWork.top;
    left = monitor_info.rcWork.right - config::stage_width;
    right = monitor_info.rcWork.right;
    bottom = monitor_info.rcWork.bottom;
}

void GDIPresenterPrivate::create_buffer(GDIPresenter *pub) {
    if(!window_dc) {
        window_dc = GetDC(hWnd);
        if(!window_dc) {
            /* Failed to create buffer */
            pub->report_error("\xe5\x88\x9b\xe5\xbb\xba\xe7\xbc\x93\xe5\x86\xb2\xe5\x8c\xba\xe5\xa4\xb1\xe8\xb4\xa5");
            abort();
        }
    }
    if(!buffer_dc) {
        buffer_dc = CreateCompatibleDC(nullptr);
        if(!buffer_dc) {
            /* Failed to create buffer */
            pub->report_error("\xe5\x88\x9b\xe5\xbb\xba\xe7\xbc\x93\xe5\x86\xb2\xe5\x8c\xba\xe5\xa4\xb1\xe8\xb4\xa5");
            abort();
        }
    }

    uint32_t width, height;
    pub->get_stage_size(width, height);
    if(buffer_bmp)
        DeleteObject(buffer_bmp);
    buffer_bmp = CreateCompatibleBitmap(buffer_dc, width, height);
    if(!buffer_bmp) {
        /* Failed to create buffer */
        pub->report_error("\xe5\x88\x9b\xe5\xbb\xba\xe7\xbc\x93\xe5\x86\xb2\xe5\x8c\xba\xe5\xa4\xb1\xe8\xb4\xa5");
        abort();
    }
}

void GDIPresenterPrivate::do_paint(GDIPresenter *, uint32_t *bitmap, uint32_t width, uint32_t height) {
    for(uint32_t i = 0; i < height/2; i++)
        for(uint32_t j = 0; j < width; j++)
            std::swap(bitmap[i*width + j], bitmap[(height-1-i)*width + j]); // TODO: optimize
    for(uint32_t i = 0; i < width * height; i++) {
        uint8_t alpha = uint8_t(bitmap[i] >> 24);
        uint8_t red = uint8_t((bitmap[i] >> 16) * alpha / 255);
        uint8_t green = uint8_t((bitmap[i] >> 8) * alpha / 255);
        uint8_t blue = uint8_t(bitmap[i] * alpha / 255);
        bitmap[i] = (uint32_t(alpha) << 24) | (uint32_t(red) << 16) | (uint32_t(green) << 8) | uint32_t(blue);
    }

    BITMAPINFO bitmap_info;
    bitmap_info.bmiHeader.biSize = sizeof bitmap_info.bmiHeader;
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    bitmap_info.bmiHeader.biSizeImage = 0;
    bitmap_info.bmiHeader.biXPelsPerMeter = 96;
    bitmap_info.bmiHeader.biYPelsPerMeter = 96;
    bitmap_info.bmiHeader.biClrUsed = 0;
    bitmap_info.bmiHeader.biClrImportant = 0;
    SetDIBits(buffer_dc, buffer_bmp, 0, height, bitmap, &bitmap_info, 0);

    SelectObject(buffer_dc, buffer_bmp);

    POINT window_pos;
    window_pos.x = left;
    window_pos.y = top;
    SIZE window_size;
    window_size.cx = right-left;
    window_size.cy = bottom-top;
    POINT buffer_pos;
    buffer_pos.x = 0;
    buffer_pos.y = 0;
    BLENDFUNCTION blend_function;
    blend_function.BlendOp = AC_SRC_OVER;
    blend_function.BlendFlags = 0;
    blend_function.SourceConstantAlpha = 255; // Set the SourceConstantAlpha value to 255 (opaque) when you only want to use per-pixel alpha values.
    blend_function.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hWnd, window_dc, &window_pos, &window_size, buffer_dc, &buffer_pos, 0, &blend_function, ULW_ALPHA);
}

LRESULT CALLBACK GDIPresenterPrivate::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

std::map<HWND, GDIPresenter *> GDIPresenterPrivate::hWndMap;

}

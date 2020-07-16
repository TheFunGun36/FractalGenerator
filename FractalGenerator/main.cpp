#include <windows.h>
#include <d2d1.h>
#include <ctime>
#include <cmath>
#include <vector>

#pragma comment(lib, "d2d1.lib")

#define TIMER_ID 321
#define SafeRelease(p) if (p != NULL) p->Release(); p = NULL;

namespace d2d {

    int wndSizeX, wndSizeY;

    PAINTSTRUCT ps;
    ID2D1HwndRenderTarget* pRenderTarget = NULL;
    ID2D1SolidColorBrush* pVertexBrush = NULL;
    ID2D1SolidColorBrush* pPointBrush = NULL;
    ID2D1SolidColorBrush* pSelectedBrush = NULL;
    ID2D1SolidColorBrush* pLineBrush = NULL;
    ID2D1Factory* pFactory = NULL;

    void CreateResources(HWND hWnd) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        wndSizeX = rc.right - rc.left;
        wndSizeY = rc.bottom - rc.top;

        if (FAILED(pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &pRenderTarget))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.3f, 0.3f), &pVertexBrush))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f), &pPointBrush))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.4f, 0.1f), &pSelectedBrush))) {
            DestroyWindow(hWnd);
            return;
        }

        if (FAILED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.65f, 0.7f, 0.7f), &pLineBrush))) {
            DestroyWindow(hWnd);
            return;
        }
    }

    void DestroyResources() {
        SafeRelease(pRenderTarget);
        SafeRelease(pPointBrush);
        SafeRelease(pVertexBrush);
        SafeRelease(pSelectedBrush);
        SafeRelease(pLineBrush);
    }

    void BeginDraw(HWND hWnd) {
        BeginPaint(hWnd, &ps);
        if (pRenderTarget == NULL) {
            CreateResources(hWnd);
        }

        pRenderTarget->BeginDraw();
    }
    void EndDraw(HWND hWnd) {
        HRESULT hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            DestroyWindow(hWnd);
        }
        EndPaint(hWnd, &ps);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    srand(clock());

    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    wcex.style = HS_VERTICAL | HS_HORIZONTAL;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInst;
    wcex.hCursor = LoadCursor(hInst, IDC_ARROW);
    wcex.lpszClassName = L"CLA";
    wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);

    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow(L"CLA", L"Fractal Generator", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    //SetTimer(hWnd, TIMER_ID, 16, NULL);

    BOOL ret = 1;
    while (ret > 0) {
        ret = GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //KillTimer(hWnd, TIMER_ID);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool isTimerExists = false;
    static int pointNum = 0;
    static int vertexNum;
    static POINT cursor;
    static std::vector<POINT> vertex;
    static POINT selected = { -5, -5 };
    static std::vector<POINT> point;
    switch (msg) {
    case WM_MOUSEMOVE:
        cursor.y = HIWORD(lParam);
        cursor.x = LOWORD(lParam);
        break;

    case WM_LBUTTONDOWN:
        vertex.push_back(cursor);
        InvalidateRect(hWnd, 0, false);
        break;

    case WM_RBUTTONDOWN:
        selected = cursor;
        InvalidateRect(hWnd, 0, false);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_SPACE) {
            if (isTimerExists) {
                KillTimer(hWnd, TIMER_ID);
                isTimerExists = false;
            }
            else if (vertex.size() != 0 && selected.x != -5 && selected.y != -5) {
                SetTimer(hWnd, TIMER_ID, 16, NULL);
                isTimerExists = true;
                vertexNum = vertex.size();
            }
        }
        else if (wParam == VK_ESCAPE) {
            if (isTimerExists) {
                KillTimer(hWnd, TIMER_ID);
                isTimerExists = false;
            }
            selected = { -5, -5 };
            vertexNum = 0;
            vertex.clear();
            pointNum = 0;
            point.clear();
        }
        InvalidateRect(hWnd, 0, false);
        break;

    case WM_SIZE:
        d2d::DestroyResources();
        InvalidateRect(hWnd, 0, false);
        break;

    case WM_TIMER:
        if (pointNum + 1000 >= point.max_size()) {
            KillTimer(hWnd, TIMER_ID);
            isTimerExists = false;
        }
        else {
            for (int i = 0; i < 300; i++) { 
                int n = rand() % vertexNum;
                POINT newPoint = { abs(selected.x + vertex[n].x) / 2, abs(selected.y + vertex[n].y) / 2 };
                point.push_back(newPoint);
                selected = newPoint;
                pointNum++;
            }
        }
        InvalidateRect(hWnd, 0, false);
        break;

    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d::pFactory)))
            DestroyWindow(hWnd);
        d2d::CreateResources(hWnd);
        break;

    case WM_PAINT:
    {
        d2d::BeginDraw(hWnd);

        d2d::pRenderTarget->Clear(D2D1::ColorF(0.7f, 0.8f, 0.9f));

        d2d::pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(selected.x, selected.y), 5.0f, 5.0f), d2d::pSelectedBrush);

        for (POINT el : vertex) {
            d2d::pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(el.x, el.y), 5.0f, 5.0f), d2d::pVertexBrush);
        }

        for (POINT el : point) {
            d2d::pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(el.x, el.y), 2.0f, 2.0f), d2d::pPointBrush);
        }

        d2d::EndDraw(hWnd);
    }
    break;

    case WM_DESTROY:
        d2d::DestroyResources();
        SafeRelease(d2d::pFactory);
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
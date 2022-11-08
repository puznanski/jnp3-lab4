#include "lab4.h"

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <cmath>
#include <windowsx.h>
#include <numbers>


INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE instance,
    _In_opt_ [[maybe_unused]] HINSTANCE prev_instance,
    _In_ [[maybe_unused]] PWSTR cmd_line,
    _In_ [[maybe_unused]] INT cmd_show) {

    App app;

    if (SUCCEEDED(app.Initialize(instance, cmd_show)))
    {
        app.RunMessageLoop();
    }
        
    return 0;
}

Timer::Timer() {
    QueryPerformanceFrequency(&frequency);
}

double Timer::get_time(int seconds) {
    LARGE_INTEGER current_counter;
    QueryPerformanceCounter(&current_counter);

    auto difference = current_counter.QuadPart % (frequency.QuadPart * seconds);

    return (double)difference / (double)frequency.QuadPart;
}

LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        App* app = (App*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(app)
        );

        result = 1;
    }
    else
    {
        App* app = reinterpret_cast<App*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (app)
        {
            switch (msg)
            {
            case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                app->OnResize(width, height);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DISPLAYCHANGE:
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_PAINT:
            {
                app->OnRender();
                ValidateRect(hwnd, NULL);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DESTROY:
            {
                PostQuitMessage(0);
            }
            result = 1;
            wasHandled = true;
            break;
            case WM_MOUSEMOVE:
            {
                app->mouse_x = GET_X_LPARAM(lParam);
                app->mouse_y = GET_Y_LPARAM(lParam);
            }
            result = 0;
            wasHandled = true;
            break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    return result;
}

App::App() :
    hwnd(nullptr),
    direct2d_factory(nullptr),
    direct2d_render_target(nullptr),
    main_brush(nullptr),
    gray_brush(nullptr),
    character_path(nullptr),
    nose_path(nullptr),
    main_gradient_brush(nullptr),
    eye_brush_right(nullptr),
    eye_brush_left(nullptr)
{}

App::~App()
{
    SafeRelease(&direct2d_factory);
    SafeRelease(&direct2d_render_target);
}

void App::RunMessageLoop()
{
    MSG msg;

    do {
        mouse_pressed = (GetAsyncKeyState(VK_LBUTTON) < 0);

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message != WM_QUIT) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            auto time = timer.get_time(2) * std::numbers::pi;
            angle = std::sin(time) * 10.0f;
            OnRender();
        }
    } 
    while (msg.message != WM_QUIT);
}

HRESULT App::Initialize(HINSTANCE instance, INT cmd_show)
{
    HRESULT hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
    {
        // Register the window class.
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = App::WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = instance;
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
        wcex.lpszClassName = L"D2DApp";

        ATOM register_result = RegisterClassEx(&wcex);
        if (register_result == 0) {
            return 1;
        }

        hwnd = CreateWindowEx(
            0,                              // Optional window styles.
            L"D2DApp",                      // Window class
            L"JNP3 - lab4",                 // Window text
            WS_OVERLAPPEDWINDOW,            // Window style

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

            nullptr,       // Parent window    
            nullptr,       // Menu
            instance,  // Instance handle
            this        // Additional application data
        );

        if (hwnd)
        {
            // Because the SetWindowPos function takes its size in pixels, we
            // obtain the window's DPI, and use it to scale the window size.
            float dpi = GetDpiForWindow(hwnd);

            SetWindowPos(
                hwnd,
                NULL,
                NULL,
                NULL,
                static_cast<int>(ceil(640.f * dpi / 96.f) * 2),
                static_cast<int>(ceil(480.f * dpi / 96.f) * 2),
                SWP_NOMOVE);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
        }
        else
        {
            return 1;
        }
    }

    return hr;
}

HRESULT App::CreateDeviceResources()
{
    using D2D1::RadialGradientBrushProperties;
    using D2D1::ColorF;
    using D2D1::Point2F;

    HRESULT hr = S_OK;

    if (!direct2d_render_target)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = direct2d_factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &direct2d_render_target
        );

        if (SUCCEEDED(hr))
        {
            // Create main brush.
            hr = direct2d_render_target->CreateSolidColorBrush(
                brush_color,
                &main_brush
            );
        }
        if (SUCCEEDED(hr))
        {
            // Create gray brush.
            hr = direct2d_render_target->CreateSolidColorBrush(
                gray_brush_color,
                &gray_brush
            );
        }
        if (SUCCEEDED(hr))
        {
            // Create character path.
            hr = direct2d_factory->CreateTransformedGeometry(
                create_character(),
                D2D1::Matrix3x2F::Translation(-126, -128),
                &character_path
            );
        }
        if (SUCCEEDED(hr))
        {
            // Create nose path.
            hr = direct2d_factory->CreateTransformedGeometry(
                create_nose(),
                D2D1::Matrix3x2F::Translation(-32, 15) * D2D1::Matrix3x2F::Scale(0.7f, 0.7f),
                &nose_path
            );
        }

        ID2D1GradientStopCollection* gradient_stops = nullptr;
        UINT const NUM_GRADIENT_STOPS = 3;
        D2D1_GRADIENT_STOP gradient_stops_data[NUM_GRADIENT_STOPS];
        ID2D1GradientStopCollection* eye_stops = nullptr;
        UINT const NUM_EYE_STOPS = 2;
        D2D1_GRADIENT_STOP eye_stops_data[NUM_EYE_STOPS];

        if (SUCCEEDED(hr))
        {
            gradient_stops_data[0] =
                { .position = 0.0f, .color = ColorF(0.73f, 0.36f, 0.89f, 1.0f) };
            gradient_stops_data[1] =
                { .position = 0.6f, .color = ColorF(0.67f, 0.04f, 0.95f, 1.0f) };
            gradient_stops_data[2] =
                { .position = 1.0f, .color = ColorF(0.48f, 0.05f, 0.97f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                gradient_stops_data, NUM_GRADIENT_STOPS, &gradient_stops);
        }
        if (SUCCEEDED(hr))
        {
            hr = direct2d_render_target->CreateRadialGradientBrush(
                RadialGradientBrushProperties(Point2F(0, 0),
                    Point2F(0, 0), 150, 150),
                gradient_stops, &main_gradient_brush);
        }
        if (SUCCEEDED(hr)) {
            eye_stops_data[0] =
            { .position = 0.8f, .color = ColorF(1.0f, 1.0f, 1.0f, 1.0f) };
            eye_stops_data[1] =
            { .position = 1.0f, .color = ColorF(0.5f, 0.5f, 0.5f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                eye_stops_data, NUM_EYE_STOPS, &eye_stops);
            
        }
		if (SUCCEEDED(hr)) 
        {
			hr = direct2d_render_target->CreateRadialGradientBrush(
				RadialGradientBrushProperties(Point2F(-EYE_X_OFFSET, EYE_Y_OFFSET),
					Point2F(0, 0), EYEBALL_RADIUS * 3, EYEBALL_RADIUS * 3),
				eye_stops, &eye_brush_left);
			hr = direct2d_render_target->CreateRadialGradientBrush(
				RadialGradientBrushProperties(Point2F(EYE_X_OFFSET, EYE_Y_OFFSET),
					Point2F(0, 0), EYEBALL_RADIUS * 3, EYEBALL_RADIUS * 3),
				eye_stops, &eye_brush_right);
		}
    }

    return hr;
}

void App::DiscardDeviceResources()
{
    SafeRelease(&direct2d_render_target);
    SafeRelease(&main_brush);
    SafeRelease(&gray_brush);
    SafeRelease(&character_path);
    SafeRelease(&nose_path);
    SafeRelease(&main_gradient_brush);
    SafeRelease(&eye_brush_left);
    SafeRelease(&eye_brush_right);
}

HRESULT App::OnRender()
{
    using D2D1::Point2F;
    using D2D1::BezierSegment;
    using D2D1::RectF;
    using D2D1::Matrix3x2F;
    using D2D1::ColorF;
    using D2D1::Ellipse;

    HRESULT hr = S_OK;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr))
    {
        auto transformation = Matrix3x2F::Scale(2.0f, 2.0f) * Matrix3x2F::Translation(
            direct2d_render_target->GetSize().width / 2.0f, 
            direct2d_render_target->GetSize().height / 2.0f
        );
        auto mouth_transformation = Matrix3x2F::Rotation(angle) * transformation;

        auto left_eyeball = create_eyeball(transformation, -EYE_X_OFFSET);
        auto right_eyeball = create_eyeball(transformation, EYE_X_OFFSET);
        auto mouth_path = create_mouth(mouse_pressed);

        direct2d_render_target->BeginDraw();
        direct2d_render_target->Clear(background_color);

        direct2d_render_target->SetTransform(transformation);
        direct2d_render_target->FillGeometry(character_path, main_gradient_brush);
        direct2d_render_target->DrawGeometry(character_path, main_brush);

        auto left_eye = Point2F(-EYE_X_OFFSET, EYE_Y_OFFSET);
        direct2d_render_target->FillEllipse(Ellipse(left_eye, EYE_RADIUS, EYE_RADIUS), eye_brush_left);
        direct2d_render_target->DrawEllipse(Ellipse(left_eye, EYE_RADIUS, EYE_RADIUS), main_brush);
        auto right_eye = Point2F(EYE_X_OFFSET, EYE_Y_OFFSET);
        direct2d_render_target->FillEllipse(Ellipse(right_eye, EYE_RADIUS, EYE_RADIUS), eye_brush_right);
        direct2d_render_target->DrawEllipse(Ellipse(right_eye, EYE_RADIUS, EYE_RADIUS), main_brush);

        direct2d_render_target->FillEllipse(left_eyeball, main_brush);
        direct2d_render_target->FillEllipse(right_eyeball, main_brush);

        direct2d_render_target->SetTransform(mouth_transformation);
        direct2d_render_target->DrawGeometry(mouth_path, main_brush, 2.0f);
        direct2d_render_target->FillGeometry(nose_path, gray_brush);
        direct2d_render_target->DrawGeometry(nose_path, main_brush);

        hr = direct2d_render_target->EndDraw();
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

void App::OnResize(UINT width, UINT height)
{
    if (direct2d_render_target)
    {
        // Note: This method can fail, but it's okay to ignore the
        // error here, because the error will be returned again
        // the next time EndDraw is called.
        direct2d_render_target->Resize(D2D1::SizeU(width, height));
    }
}

HRESULT App::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2d_factory);

    return hr;
}

ID2D1PathGeometry* App::create_character() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(126.9f, 32.7f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(109.5f, 32.6f), Point2F(96.8f, 41.1f), Point2F(93.8f, 36.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(93.1f, 35.5f), Point2F(93.3f, 34.f), Point2F(93.f, 33.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(94.6f, 23.0f), Point2F(90.2f, 13.0f), Point2F(86.8f, 8.f)));
    path_sink->AddBezier(BezierSegment(Point2F(80.7f, 1.0f), Point2F(66.4f, -2.f), Point2F(55.6f, 3.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(40.6f, 10.5f), Point2F(39.2f, 31.2f), Point2F(43.3f, 41.f)));
    path_sink->AddBezier(BezierSegment(Point2F(43.f, 42.6f), Point2F(44.1f, 42.7f), Point2F(44.6f, 44.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(48.6f, 55.3f), Point2F(40.6f, 67.2f), Point2F(38.f, 70.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(32.4f, 80.3f), Point2F(-15.1f, 156.2f), Point2F(5.8f, 205.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(12.3f, 220.6f), Point2F(23.8f, 229.3f), Point2F(30.4f, 234.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(54.9f, 253.0f), Point2F(84.7f, 254.f), Point2F(114.0f, 255.7f)));
    path_sink->AddBezier(BezierSegment(Point2F(122.2f, 256.1f), Point2F(130.f, 256.1f), Point2F(138.6f, 255.7f)));
    path_sink->AddBezier(BezierSegment(Point2F(167.8f, 254.f), Point2F(197.6f, 253.0f), Point2F(222.2f, 234.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(228.7f, 229.3f), Point2F(240.2f, 220.5f), Point2F(246.8f, 205.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(267.8f, 156.2f), Point2F(220.2f, 80.3f), Point2F(214.3f, 70.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(211.9f, 67.2f), Point2F(204.0f, 55.3f), Point2F(208.0f, 44.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(208.f, 42.7f), Point2F(208.7f, 42.6f), Point2F(209.2f, 41.f)));
    path_sink->AddBezier(BezierSegment(Point2F(213.3f, 31.2f), Point2F(212.0f, 10.5f), Point2F(196.9f, 3.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(186.1f, -2.2f), Point2F(171.8f, 1.0f), Point2F(165.7f, 8.f)));
    path_sink->AddBezier(BezierSegment(Point2F(162.1f, 13.3f), Point2F(158.2f, 23.f), Point2F(159.1f, 33.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(159.2f, 34.4f), Point2F(159.4f, 35.5f), Point2F(158.f, 36.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(155.8f, 41.f), Point2F(143.7f, 32.f), Point2F(126.9f, 32.78f)));
    
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    path_sink->Close();

    return path;
}

ID2D1PathGeometry* App::create_nose() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(36.11039f, 48.4787f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(52.26039f, 49.1387f), Point2F(61.49039f, 42.5987f), Point2F(64.38039f, 38.3487f)));
    path_sink->AddBezier(BezierSegment(Point2F(67.27039f, 34.0987f), Point2F(72.74039f, 21.3487f), Point2F(70.72039f, 10.2487f)));
    path_sink->AddBezier(BezierSegment(Point2F(68.70039f, -0.8512100f), Point2F(36.11039f, 0.4487899f), Point2F(36.11039f, 0.4487899f)));
    path_sink->AddBezier(BezierSegment(Point2F(36.11039f, 0.4487899f), Point2F(3.510398f, -0.8612100f), Point2F(1.490398f, 10.2487f)));
    path_sink->AddBezier(BezierSegment(Point2F(-0.5296014f, 21.3587f), Point2F(4.950398f, 34.1087f), Point2F(7.840398f, 38.3487f)));
    path_sink->AddBezier(BezierSegment(Point2F(10.73039f, 42.5887f), Point2F(19.95039f, 49.1387f), Point2F(36.11039f, 48.47879f)));
   
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    path_sink->Close();

    return path;
}

ID2D1PathGeometry* App::create_mouth(bool is_happy) {
    using D2D1::Point2F;
    using D2D1::SizeF;
    using D2D1::ArcSegment;

    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);
    path_sink->BeginFigure(Point2F(-40.0f, 75.0f), D2D1_FIGURE_BEGIN_HOLLOW);

    if (is_happy)
    {
        path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(50.0f, 30.0f), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }
    else
    {
        path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(50.0f, 30.0f), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }

    path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    path_sink->Close();

    return path;
}

D2D1_ELLIPSE App::create_eyeball(const D2D1::Matrix3x2F& transformation, FLOAT offset_x) {
    using D2D1::Point2F;
    using D2D1::Matrix3x2F;
    using D2D1::Ellipse;

    Matrix3x2F transformationCopy = transformation;
    if (transformationCopy.IsInvertible()) {
        transformationCopy.Invert();
    }

    auto mouse_position_transformed = transformationCopy.TransformPoint(Point2F(mouse_x, mouse_y));

    auto eye_x = offset_x;
    auto eye_y = EYE_Y_OFFSET;
    auto dist_x = mouse_position_transformed.x - eye_x, dist_y = mouse_position_transformed.y - eye_y;
    auto dist_squared = dist_x * dist_x + dist_y * dist_y;
    auto eyeball_range = EYE_RADIUS - EYEBALL_RADIUS;

    D2D1_POINT_2F eyeball_position;
    if (dist_squared <= eyeball_range * eyeball_range) {
        eyeball_position = Point2F(mouse_position_transformed.x, mouse_position_transformed.y);
    }
    else {
        auto fac = eyeball_range / std::sqrt(dist_squared);
        eyeball_position = Point2F(eye_x + dist_x * fac, eye_y + dist_y * fac);
    }

    return Ellipse(eyeball_position, EYEBALL_RADIUS, EYEBALL_RADIUS);
}
#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d2d1_3.h>

template<class Interface>
inline void SafeRelease(
    Interface** ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

INT WINAPI wWinMain(_In_ HINSTANCE instance,
	_In_opt_ HINSTANCE prev_instance,
	_In_ PWSTR cmd_line,
	_In_ INT cmd_show);

class Timer {
public:
    Timer();

    double get_time(int seconds);
private:
    LARGE_INTEGER frequency;
};

class App
{
public:
    App();
    ~App();

    HRESULT Initialize(HINSTANCE instance, INT cmd_show);
    void RunMessageLoop();

private:
    HWND hwnd;
    ID2D1Factory* direct2d_factory;
    ID2D1HwndRenderTarget* direct2d_render_target;
    ID2D1SolidColorBrush* main_brush;
    ID2D1SolidColorBrush* gray_brush;
    ID2D1TransformedGeometry* character_path;
    ID2D1TransformedGeometry* nose_path;
    ID2D1RadialGradientBrush* main_gradient_brush;
    ID2D1RadialGradientBrush* eye_brush_right;
    ID2D1RadialGradientBrush* eye_brush_left;
    Timer timer;

    
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
    HRESULT OnRender();
    
    void OnResize(
        UINT width,
        UINT height
    );

    static LRESULT CALLBACK WindowProc(
        HWND hwnd, 
        UINT msg, 
        WPARAM wParam, 
        LPARAM lParam
    );
    
    FLOAT angle = 0.0f;
    INT mouse_x = 0;
    INT mouse_y = 0;
    bool mouse_pressed = false;

    ID2D1PathGeometry* create_character();
    ID2D1PathGeometry* create_nose();
    ID2D1PathGeometry* create_mouth(bool is_happy);
    D2D1_ELLIPSE create_eyeball(const D2D1::Matrix3x2F& transformation, FLOAT offset_x);


    static constexpr FLOAT EYE_X_OFFSET = 59.0f;
    static constexpr FLOAT EYE_Y_OFFSET = -28.0f;
    static constexpr FLOAT EYE_RADIUS = 45.0f;
    static constexpr FLOAT EYEBALL_RADIUS = 15.0f;

    static constexpr D2D1_COLOR_F background_color =
        { .r = 0.42f, .g = 0.73f, .b = 0.91f, .a = 1.0f };
    static constexpr D2D1_COLOR_F brush_color =
        { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
    static constexpr D2D1_COLOR_F gray_brush_color =
        { .r = 0.3f, .g = 0.3f, .b = 0.3f, .a = 1.0f };
    static constexpr D2D1_COLOR_F clear_color =
        { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
};

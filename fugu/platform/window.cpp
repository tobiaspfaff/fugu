#include "window.hpp"
#include "util/util.hpp"

#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#define APP_NAME_STR_LEN 80
#include <windows.h>

using namespace std;

struct Window::Impl {
	HWND hWnd;
	HINSTANCE hInst;
};

// Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		//run(info);
		return 0;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

Window::Window(const string& name, int w, int h) :
	width(w), height(h), impl(new Impl)
{
	wchar_t buffer[256];
	size_t nlength = 0;
	mbstowcs_s(&nlength, buffer, name.c_str(), name.size() + 1);
	WNDCLASSEX winClass;
	impl->hInst = GetModuleHandle(nullptr);

	// Initialize the window class structure:
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc = WndProc;
	winClass.cbClsExtra = 0;
	winClass.cbWndExtra = 0;
	winClass.hInstance = impl->hInst;
	winClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	winClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winClass.lpszMenuName = nullptr;
	winClass.lpszClassName = buffer;
	winClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

	// Register window class:
	if (!RegisterClassEx(&winClass))
		fatalError("register class");

	// Create window with the registered class:
	RECT wr = { 0, 0, w, h };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	impl->hWnd = CreateWindowEx(0,
		buffer,            // class name
		buffer,            // app name
		WS_OVERLAPPEDWINDOW | // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,           // x/y coords
		wr.right - wr.left, // width
		wr.bottom - wr.top, // height
		nullptr,               // handle to parent
		nullptr,               // handle to menu
		impl->hInst,    // hInstance
		nullptr);              // no extra parameters
	if (!impl->hWnd)
		fatalError("create window");
}

Window::~Window() = default;

void* Window::getInstance()
{
	return impl->hInst;
}

void* Window::getHandle()
{
	return impl->hWnd;
}
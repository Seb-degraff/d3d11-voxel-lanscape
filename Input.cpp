#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>

#include "Input.h"

namespace Input
{
	// internal state
	float mouse_x_;
	float mouse_y_;
	float last_mouse_x_;
	float last_mouse_y_;

	State state;

	/// <returns>true if we used the message</returns>
	bool HandleInput(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg) {
			case WM_MOUSEMOVE: {
				mouse_x_ = GET_X_LPARAM(lparam);
				mouse_y_ = GET_Y_LPARAM(lparam);
				return true;
			}
			case WM_KEYDOWN: {
				if (wparam == 'W') {
					state.w = true;
					return true;
				}
				if (wparam == 'A') {
					state.a = true;
					return true;
				}
				if (wparam == 'S') {
					state.s = true;
					return true;
				}
				if (wparam == 'D') {
					state.d = true;
					return true;
				}
				if (wparam == 'Q') {
					state.q = true;
					return true;
				}
				if (wparam == 'E') {
					state.e = true;
					return true;
				}
				// hardcode the jump key for now
				if (wparam == VK_SPACE) {
					state.jump = true;
					return true;
				}
				break;
			}
			case WM_KEYUP: {
				if (wparam == 'W') {
					state.w = false;
					return true;
				}
				if (wparam == 'A') {
					state.a = false;
					return true;
				}
				if (wparam == 'S') {
					state.s = false;
					return true;
				}
				if (wparam == 'D') {
					state.d = false;
					return true;
				}
				if (wparam == 'Q') {
					state.q = false;
					return true;
				}
				if (wparam == 'E') {
					state.e = false;
					return true;
				}
				break;
			}
		}
		return false;
	}

	void Tick()
	{
		// to avoid computing a delta on the first frame when last mouse is 
		if (last_mouse_x_ != 0 || last_mouse_y_ != 0) {
			state.mouse_delta_x = mouse_x_ - last_mouse_x_;
			state.mouse_delta_y = mouse_y_ - last_mouse_y_;
		}
		last_mouse_x_ = mouse_x_;
		last_mouse_y_ = mouse_y_;
	}
};

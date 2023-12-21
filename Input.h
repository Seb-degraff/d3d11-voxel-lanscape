#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>

namespace Input {
	struct State {
		bool w;
		bool a;
		bool s;
		bool d;
		bool e;
		bool q;
		float mouse_delta_x;
		float mouse_delta_y;
	};

	extern State state;

	/// <returns>true if we used the message</returns>
	bool HandleInput(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void Tick();
};

#pragma once

#include "hex.hpp"
#include "fwd.hpp"

#include <SDL.h>


class h_MainLoop final
{

public:
	h_MainLoop();
	~h_MainLoop();

	bool Loop();

public://main menu interface logic
	void Quit();
	void StartGame();
	void QuitToMainMenu();

private:
	void Paint();
	void UpdateCursor();
	void ProcessEvents();

private:
	const h_SettingsPtr settings_;
	bool quit_requested_= false;

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;

	bool cursor_was_grabbed_;

	int screen_width_, screen_height_;

	h_WorldHeaderPtr world_header_;
	r_WorldRendererPtr world_renderer_;
	h_WorldPtr world_;
	h_PlayerPtr player_;

	bool game_started_;

	std::unique_ptr<ui_Painter> ui_painter_;
	std::unique_ptr<ui_MenuBase> root_menu_;
};

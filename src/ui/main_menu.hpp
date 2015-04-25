#pragma once
#include "ui_base_classes.hpp"

class h_MainLoop;

class ui_SettingsMenu : public ui_MenuBase
{
public:
	ui_SettingsMenu( ui_MenuBase* parent, int x, int y, int sx, int sy );
	virtual ~ui_SettingsMenu() override;

	virtual void Tick() override {};

private:
	void OnBackButton();
	void OnTexturesSizeSlider();

private:
	ui_Button* button_back_;

	ui_Text* text_textures_size_;
	ui_Slider* slider_textures_size_;

	ui_Text* text_textures_filtration_;
	ui_Button* button_textures_fitration_;
};

class ui_MainMenu : public ui_MenuBase
{
public:
	ui_MainMenu( h_MainLoop* main_loop, int sx, int sy );
	virtual ~ui_MainMenu() override;

	virtual void Tick() override;

private:
	void OnPlayButton();
	void OnSettingsButton();
	void OnQuitButton();
private:
	ui_Button* button_play_;
	ui_Button* button_settings_;
	ui_Button* button_quit_;
	ui_Checkbox* checkbox_;
	ui_Text* game_title_;
	ui_Text* game_subtitle_;
	ui_ProgressBar* progress_bar_;

	h_MainLoop * const main_loop_;
};

#pragma once
#include "ui_base_classes.hpp"

class h_MainLoop;

class ui_SettingsMenu : public ui_MenuBase
{
public:
	ui_SettingsMenu( ui_MenuBase* parent, const h_SettingsPtr settings, int x, int y, int sx, int sy );
	virtual ~ui_SettingsMenu() override;

	virtual void Tick() override {}

private:
	void OnBackButton();
	void OnTexturesSizeSlider();

private:
	h_SettingsPtr settings_;

	std::unique_ptr<ui_Button> button_back_;

	std::unique_ptr<ui_Text> text_textures_size_;
	std::unique_ptr<ui_Slider> slider_textures_size_;

	std::unique_ptr<ui_Text> fullscreen_text_;
	std::unique_ptr<ui_Checkbox> fullscreen_checkbox_;
};

class ui_MainMenu : public ui_MenuBase
{
public:
	ui_MainMenu( h_MainLoop* main_loop, const h_SettingsPtr& settings, int sx, int sy );
	virtual ~ui_MainMenu() override;

	virtual void KeyPress( ui_Key key ) override;
	virtual void Tick() override;

private:
	void OnPlayButton();
	void OnSettingsButton();
	void OnQuitButton();

private:
	h_MainLoop * const main_loop_;
	h_SettingsPtr settings_;

	std::unique_ptr<ui_Button> button_play_;
	std::unique_ptr<ui_Button> button_settings_;
	std::unique_ptr<ui_Button> button_quit_;
	std::unique_ptr<ui_Text> game_title_;
	std::unique_ptr<ui_Text> game_subtitle_;
};

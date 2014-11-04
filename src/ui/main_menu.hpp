#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include "ui_base_classes.hpp"


class h_MainLoop;

class ui_SettingsMenu:
	public ui_MenuBase,
	public ui_Button::ui_ButtonCallback
{
	public:
	ui_SettingsMenu( ui_MenuBase* parent, int x, int y, int sx, int sy );
	~ui_SettingsMenu();

	void ButtonCallback( ui_Button* button ) override;

private:
	ui_Button* button_back;
};

class ui_MainMenu :
	public ui_MenuBase,
	public ui_Button::ui_ButtonCallback,
	public ui_Checkbox::ui_CheckboxCallback,
	public ui_Slider::ui_SliderCallback
{
	public:

	ui_MainMenu( h_MainLoop* main_loop_, int sx, int sy );
	~ui_MainMenu();

	//void Draw( ui_Painter* painter );

	void ButtonCallback( ui_Button* button ) override;
	void CheckboxCallback( ui_Checkbox* checkbox ) override;
	void SliderCallback( ui_Slider* slider ) override;


	private:

	ui_Button* button_play;
	ui_Button* button_settings;
	ui_Button* button_quit;
	ui_Checkbox* checkbox;
	ui_Text* game_title;
	ui_ProgressBar* progress_bar;
	ui_Slider* slider;


	h_MainLoop * const main_loop;

};
#endif//MAIN_MENU_HPP

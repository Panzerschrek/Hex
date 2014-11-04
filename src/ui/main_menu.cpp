#include "../hex.hpp"

#include "../main_loop.hpp"
#include "ui_painter.hpp"

#include "main_menu.hpp"

//main menu colors
static const unsigned char normal_color[]= { 128, 128, 128, 255 };
static const unsigned char active_color[]= { 200, 48, 48, 255 };
static const unsigned char text_color[]= { normal_color[0], normal_color[1], normal_color[2], 0 };


/*
------------ui_SettingsMenu---------------
*/
ui_SettingsMenu::ui_SettingsMenu( ui_MenuBase* parent, int x, int y, int sx, int sy ):
	ui_MenuBase( parent, x, y, sx, sy )
{
	int button_shift_y= ui_MenuBase::size_y/(ui_Base::CellSize()) - 2;

	button_back= new ui_Button( "< Back", 1, button_shift_y, 4, 1, normal_color, active_color );
	button_back->SetCallback( this );

	ui_MenuBase::elements.push_back( button_back );

}

ui_SettingsMenu::~ui_SettingsMenu()
{
	delete button_back;
}

void ui_SettingsMenu::ButtonCallback( ui_Button* button )
{
	if( button == button_back )
	{
		parent_menu->SetActive( true );
		printf( "active" );
		//parent_menu->KillChild();
	}
}
/*
---------ui_MainMenu---------------
*/

ui_MainMenu::ui_MainMenu( h_MainLoop* main_loop_, int sx, int sy ):
	ui_MenuBase( nullptr, 0, 0, sx, sy ),
	main_loop(main_loop_)
{

	const int menu_buttons= 3;
	const int button_size= 10;
	int button_shift_x= ui_MenuBase::size_x/(ui_Base::CellSize()*2) - button_size/2;
	int button_shift_y= ui_MenuBase::size_y/(ui_Base::CellSize()*2) - (2*3 + 2)/2;

	button_play= new ui_Button( "Play", 		button_shift_x, button_shift_y+0, button_size, 2, normal_color, active_color );
	button_play->SetCallback( this );
	button_settings= new ui_Button( "Settings", button_shift_x, button_shift_y+3, button_size, 2, normal_color, active_color );
	button_settings->SetCallback( this );
	button_quit= new ui_Button( "Quit", 		button_shift_x, button_shift_y+6, button_size, 2, normal_color, active_color );
	button_quit->SetCallback( this );

	checkbox= new ui_Checkbox( 4, 4, false, normal_color, active_color );
	checkbox->SetCallback( this );
	game_title= new ui_Text( "Hex", ui_Text::ALIGNMENT_CENTER, 5, 4, text_color );
	progress_bar= new ui_ProgressBar( 3, 20, 20, 2, 0.0f, normal_color, active_color );
	slider= new ui_Slider( 1, 14, 8, 0.3f, normal_color, active_color );
	slider->SetInvStep(8);
	slider->SetCallback(this);

	ui_MenuBase::elements.push_back( button_play );
	ui_MenuBase::elements.push_back( button_settings );
	ui_MenuBase::elements.push_back( button_quit );

	ui_MenuBase::elements.push_back( checkbox );
	ui_MenuBase::elements.push_back( game_title );
	ui_MenuBase::elements.push_back( progress_bar );
	ui_MenuBase::elements.push_back( slider );
}

ui_MainMenu::~ui_MainMenu()
{
	delete button_play;
	delete button_settings;
	delete button_quit;

	delete checkbox;
	delete game_title;
	delete progress_bar;
	delete slider;
}

void ui_MainMenu::ButtonCallback( ui_Button* button )
{
	if( button == button_settings )
	{
		child_menu= new ui_SettingsMenu( this, 0,0, size_x, size_y );
		this->SetActive( false );
	}
	else if( button == button_play )
	{
		main_loop->StartGame();
	}
	else if( button == button_quit )
	{
		main_loop->Quit();
	}

	printf( "main menu button!\n" );
}

void ui_MainMenu::CheckboxCallback( ui_Checkbox* checkbox )
{
	/*if( checkbox == this->checkbox )
	{
		static float p= 0.0f;
		progress_bar->SetProgress( p );
		p+= 0.01f;
	}*/
}


void ui_MainMenu::SliderCallback( ui_Slider* slider )
{
	printf( "Slider callback!\n" );
	float pos= slider->SliderPos();

	int i_pos= int(roundf(pos * 8.0f) );
	slider->SetSliderPos( float(i_pos) * 0.125f );
}

#include "../hex.hpp"

#include "../main_loop.hpp"
#include "ui_painter.hpp"

#include "main_menu.hpp"

//main menu colors
static const unsigned char normal_color[]= { 128, 128, 128, 255 };
static const unsigned char active_color[]= { 200, 48, 48, 255 };
static const unsigned char text_color[]= { normal_color[0], normal_color[1], normal_color[2], 255 };

/*
------------ui_SettingsMenu---------------
*/
ui_SettingsMenu::ui_SettingsMenu( ui_MenuBase* parent, int x, int y, int sx, int sy )
	: ui_MenuBase( parent, x, y, sx, sy )
{
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()) - 2;

	button_back= new ui_Button( "< Back", 1, button_shift_y, 4, 1, normal_color, active_color );
	button_back->SetCallback( this );

	int center_cell_x= ui_MenuBase::size_x_ / (2*ui_Base::CellSize());
	int center_cell_y= ui_MenuBase::size_y_ / (2*ui_Base::CellSize());

	int punkt_row= center_cell_y-4;

	text_textures_size= new ui_Text( "Textures size: 1", ui_Text::ALIGNMENT_RIGHT, center_cell_x-10, punkt_row, 10, 1, text_color );
	slider_textures_size= new ui_Slider( center_cell_x, punkt_row, 8, 1.0f, normal_color, active_color );
	slider_textures_size->SetInvStep( 3 );
	slider_textures_size->SetCallback(this);

	punkt_row++;

	text_textures_filtration= new ui_Text( "Textures filter:", ui_Text::ALIGNMENT_RIGHT, center_cell_x-10, punkt_row, 10, 1, text_color );
	button_textures_fitration= new ui_Button( "linear", center_cell_x, punkt_row, 8, 1, normal_color, active_color );

	ui_MenuBase::elements_.push_back( button_back );
	ui_MenuBase::elements_.push_back( text_textures_size );
	ui_MenuBase::elements_.push_back( slider_textures_size );
	ui_MenuBase::elements_.push_back( text_textures_filtration );
	ui_MenuBase::elements_.push_back( button_textures_fitration );

}

ui_SettingsMenu::~ui_SettingsMenu()
{
	delete button_back;
}

void ui_SettingsMenu::ButtonCallback( ui_Button* button )
{
	if( button == button_back )
	{
		parent_menu_->SetActive( true );
		printf( "active" );
		this->Kill();
		//parent_menu->KillChild();
	}
}
void ui_SettingsMenu::SliderCallback( ui_Slider* slider )
{
	int i_pos= int(roundf(slider->SliderPos() * 3));
	slider_textures_size->SetSliderPos( float(i_pos) / 3.0f );

	i_pos= 3-i_pos;
	int size= 1<<i_pos;

	char text[48];
	sprintf( text, "Textures size: 1/%d", size );
	text_textures_size->SetText( text );
}

/*
---------ui_MainMenu---------------
*/

ui_MainMenu::ui_MainMenu( h_MainLoop* main_loop_, int sx, int sy )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, main_loop(main_loop_)
{
	const int menu_buttons= 3;
	const int button_size= 10;
	int button_shift_x= ui_MenuBase::size_x_/(ui_Base::CellSize()*2) - button_size/2;
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()*2) - (2*3 + 2)/2;

	button_play= new ui_Button( "Play", 		button_shift_x, button_shift_y+0, button_size, 2, normal_color, active_color );
	button_play->SetCallback( this );
	button_settings= new ui_Button( "Settings", button_shift_x, button_shift_y+3, button_size, 2, normal_color, active_color );
	button_settings->SetCallback( this );
	button_quit= new ui_Button( "Quit", 		button_shift_x, button_shift_y+6, button_size, 2, normal_color, active_color );
	button_quit->SetCallback( this );

	//checkbox= new ui_Checkbox( 4, 4, false, normal_color, active_color );
	//checkbox->SetCallback( this );

	game_title= new ui_Text( "H E X", ui_Text::ALIGNMENT_CENTER, button_shift_x, button_shift_y-5, 10, 2, text_color );
	game_subtitle = new ui_Text( "a game in world of hexogonal prisms", ui_Text::ALIGNMENT_CENTER, button_shift_x, button_shift_y-3, 10, 1, text_color );

	//progress_bar= new ui_ProgressBar( 3, 20, 20, 2, 0.0f, normal_color, active_color );

	ui_MenuBase::elements_.push_back( button_play );
	ui_MenuBase::elements_.push_back( button_settings );
	ui_MenuBase::elements_.push_back( button_quit );

	//ui_MenuBase::elements_.push_back( checkbox );
	ui_MenuBase::elements_.push_back( game_title );
	ui_MenuBase::elements_.push_back( game_subtitle );
	//ui_MenuBase::elements_.push_back( progress_bar );
}

ui_MainMenu::~ui_MainMenu()
{
	delete button_play;
	delete button_settings;
	delete button_quit;

	//delete checkbox;
	delete game_title;
	delete game_subtitle;
	//delete progress_bar;
}

void ui_MainMenu::ButtonCallback( ui_Button* button )
{
	if( button == button_settings )
	{
		child_menu_= new ui_SettingsMenu( this, 0,0, size_x_, size_y_ );
		this->SetActive( false );
		this->SetVisible( false );
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

void ui_MainMenu::Tick()
{
	if( child_menu_ != nullptr )
		if( child_menu_->IsMarkedForKilling() )
		{
			delete child_menu_;
			child_menu_= nullptr;
			this->SetActive( true );
			this->SetVisible( true );
		}
}

void ui_MainMenu::SliderCallback( ui_Slider* slider )
{
	printf( "Slider callback!\n" );
	float pos= slider->SliderPos();

	int i_pos= int(roundf(pos * 8.0f) );
	slider->SetSliderPos( float(i_pos) * 0.125f );
}

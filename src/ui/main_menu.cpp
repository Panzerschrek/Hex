#include "../hex.hpp"

#include "../main_loop.hpp"
#include "ui_painter.hpp"

#include "main_menu.hpp"

//main menu colors
static const unsigned char normal_color[]= { 128, 128, 128, 255 };
static const unsigned char active_color[]= { 200, 48, 48, 255 };
static const unsigned char text_color[]= { 96, 96, 96, 255 };
static const unsigned char active_text_color[]= { 150, 32, 32, 255 };

static const ui_Style g_main_style(
	normal_color,
	active_color,
	ui_Style::TextAlignment::Center,
	text_color,
	active_text_color );

static const ui_Style g_text_style(
	normal_color,
	active_color,
	ui_Style::TextAlignment::Center,
	text_color,
	text_color );

/*
------------ui_SettingsMenu---------------
*/
ui_SettingsMenu::ui_SettingsMenu( ui_MenuBase* parent, int x, int y, int sx, int sy )
	: ui_MenuBase( parent, x, y, sx, sy )
{
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()) - 2;

	button_back_= new ui_Button( "< Back", 1, button_shift_y, 4, 1, g_main_style );
	button_back_->SetCallback( [this]{ OnBackButton(); } );

	int center_cell_x= ui_MenuBase::size_x_ / (2*ui_Base::CellSize());
	int center_cell_y= ui_MenuBase::size_y_ / (2*ui_Base::CellSize());

	int punkt_row= center_cell_y-4;

	text_textures_size_= new ui_Text( "Textures size: 1", center_cell_x-10, punkt_row, 10, 1, g_text_style );
	slider_textures_size_= new ui_Slider( center_cell_x, punkt_row, 8, 1.0f, g_main_style );
	slider_textures_size_->SetInvStep( 3 );
	slider_textures_size_->SetCallback( [this] { OnTexturesSizeSlider(); } );

	punkt_row++;

	text_textures_filtration_= new ui_Text( "Textures filter:", center_cell_x-10, punkt_row, 10, 1, g_text_style );
	button_textures_fitration_= new ui_Button( "linear", center_cell_x, punkt_row, 8, 1, g_main_style );

	ui_MenuBase::elements_.push_back( button_back_ );
	ui_MenuBase::elements_.push_back( text_textures_size_ );
	ui_MenuBase::elements_.push_back( slider_textures_size_ );
	ui_MenuBase::elements_.push_back( text_textures_filtration_ );
	ui_MenuBase::elements_.push_back( button_textures_fitration_ );

}

ui_SettingsMenu::~ui_SettingsMenu()
{
	delete button_back_;
	delete text_textures_size_;
	delete slider_textures_size_;
	delete text_textures_filtration_;
	delete button_textures_fitration_;
}

void ui_SettingsMenu::OnBackButton()
{
	parent_menu_->SetActive( true );
	printf( "active" );
	this->Kill();
}

void ui_SettingsMenu::OnTexturesSizeSlider()
{
	int i_pos= int(roundf(slider_textures_size_->SliderPos() * 3));
	slider_textures_size_->SetSliderPos( float(i_pos) / 3.0f );

	i_pos= 3-i_pos;
	int size= 1<<i_pos;

	char text[48];
	sprintf( text, "Textures size: 1/%d", size );
	text_textures_size_->SetText( text );
}

/*
---------ui_MainMenu---------------
*/

ui_MainMenu::ui_MainMenu( h_MainLoop* main_loop, int sx, int sy )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, main_loop_(main_loop)
{
	const int menu_buttons= 3;
	const int button_size= 10;
	int button_shift_x= ui_MenuBase::size_x_/(ui_Base::CellSize()*2) - button_size/2;
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()*2) - (2*3 + 2)/2;

	button_play_= new ui_Button( "Play", 		button_shift_x, button_shift_y+0, button_size, 2, g_main_style );
	button_play_->SetCallback( [this]{ OnPlayButton(); } );
	button_settings_= new ui_Button( "Settings", button_shift_x, button_shift_y+3, button_size, 2, g_main_style );
	button_settings_->SetCallback( [this]{ OnSettingsButton(); } );
	button_quit_= new ui_Button( "Quit", 		button_shift_x, button_shift_y+6, button_size, 2, g_main_style );
	button_quit_->SetCallback( [this]{ OnQuitButton(); } );

	game_title_= new ui_Text( "H E X", button_shift_x, button_shift_y-5, 10, 2, g_text_style );
	game_subtitle_ = new ui_Text( "a game in world of hexogonal prisms", button_shift_x, button_shift_y-3, 10, 1, g_text_style );

	ui_MenuBase::elements_.push_back( button_play_ );
	ui_MenuBase::elements_.push_back( button_settings_ );
	ui_MenuBase::elements_.push_back( button_quit_ );

	ui_MenuBase::elements_.push_back( game_title_ );
	ui_MenuBase::elements_.push_back( game_subtitle_ );
}

ui_MainMenu::~ui_MainMenu()
{
	delete button_play_;
	delete button_settings_;
	delete button_quit_;

	delete game_title_;
	delete game_subtitle_;
}


void ui_MainMenu::OnPlayButton()
{
	main_loop_->StartGame();
}

void ui_MainMenu::OnSettingsButton()
{
	child_menu_= new ui_SettingsMenu( this, 0, 0, size_x_, size_y_ );
	this->SetActive( false );
	this->SetVisible( false );
}

void ui_MainMenu::OnQuitButton()
{
	main_loop_->Quit();
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

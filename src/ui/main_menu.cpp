#include "../hex.hpp"

#include "../main_loop.hpp"
#include "../settings.hpp"
#include "../settings_keys.hpp"
#include "../renderer/rendering_constants.hpp"
#include "ui_painter.hpp"
#include "styles.hpp"

#include "main_menu.hpp"

/*
------------ui_SettingsMenu---------------
*/

static const int g_textures_size_step= R_MAX_TEXTURE_RESOLUTION_LOG2 - R_MIN_TEXTURE_RESOLUTION_LOG2;

ui_SettingsMenu::ui_SettingsMenu( ui_MenuBase* parent, const h_SettingsPtr settings, int x, int y, int sx, int sy )
	: ui_MenuBase( parent, x, y, sx, sy )
	, settings_(settings)
{
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()) - 2;

	button_back_.reset( new ui_Button( "< Back", 1, button_shift_y, 4, 1, c_ui_main_style ) );
	button_back_->SetCallback( [this]{ OnBackButton(); } );

	int center_cell_x= ui_MenuBase::size_x_ / (2*ui_Base::CellSize());
	int center_cell_y= ui_MenuBase::size_y_ / (2*ui_Base::CellSize());
	const int c_text_size= 10;

	int punkt_row= center_cell_y - 4;

	ui_Style texts_style= c_ui_texts_style;
	texts_style.text_alignment= ui_Style::TextAlignment::Right;

	text_textures_size_.reset( new ui_Text( "", center_cell_x - c_text_size - 1, punkt_row, c_text_size, 1, texts_style ) );
	slider_textures_size_.reset( new ui_Slider( center_cell_x, punkt_row, 1 + g_textures_size_step, 1.0f, c_ui_main_style ) );
	slider_textures_size_->SetInvStep( g_textures_size_step );
	slider_textures_size_->SetCallback( [this] { OnTexturesSizeSlider(); } );

	slider_textures_size_->SetSliderPos(
		1.0f - float(settings_->GetInt(h_SettingsKeys::textures_detalization)) / float(g_textures_size_step) );
	OnTexturesSizeSlider();

	punkt_row++;

	fullscreen_text_.reset( new ui_Text( "fullscreen" , center_cell_x - c_text_size - 1, punkt_row, c_text_size, 1, texts_style ) );
	fullscreen_checkbox_.reset( new ui_Checkbox( center_cell_x, punkt_row, settings_->GetBool(h_SettingsKeys::fullscreen), c_ui_main_style ) );
	fullscreen_checkbox_->SetCallback(
		[this]
		{
			settings_->SetSetting( h_SettingsKeys::fullscreen, fullscreen_checkbox_->GetState() );
		} );

	ui_MenuBase::elements_.push_back( button_back_.get() );
	ui_MenuBase::elements_.push_back( text_textures_size_.get() );
	ui_MenuBase::elements_.push_back( slider_textures_size_.get() );
	ui_MenuBase::elements_.push_back( fullscreen_text_.get() );
	ui_MenuBase::elements_.push_back( fullscreen_checkbox_.get() );
}

ui_SettingsMenu::~ui_SettingsMenu()
{
}

void ui_SettingsMenu::OnBackButton()
{
	parent_menu_->SetActive( true );
	this->Kill();
}

void ui_SettingsMenu::OnTexturesSizeSlider()
{
	int i_pos= slider_textures_size_->SliderDiscretPos();

	i_pos= g_textures_size_step - i_pos;

	char text[48];
	sprintf( text, "Textures size: 1/%d", 1 << i_pos );
	text_textures_size_->SetText( text );

	settings_->SetSetting( h_SettingsKeys::textures_detalization, i_pos );
}

/*
---------ui_MainMenu---------------
*/

ui_MainMenu::ui_MainMenu( h_MainLoop* main_loop, const h_SettingsPtr& settings, int sx, int sy )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, main_loop_(main_loop)
	, settings_(settings)
{
	const int button_size= 10;
	int button_shift_x= ui_MenuBase::size_x_/(ui_Base::CellSize()*2) - button_size/2;
	int button_shift_y= ui_MenuBase::size_y_/(ui_Base::CellSize()*2) - (2*3 + 2)/2;

	button_play_.reset( new ui_Button( "Play", button_shift_x, button_shift_y+0, button_size, 2, c_ui_main_style ) );
	button_play_->SetCallback( [this]{ OnPlayButton(); } );

	button_settings_.reset( new ui_Button( "Settings", button_shift_x, button_shift_y+3, button_size, 2, c_ui_main_style ) );
	button_settings_->SetCallback( [this]{ OnSettingsButton(); } );

	button_quit_.reset( new ui_Button( "Quit", button_shift_x, button_shift_y+6, button_size, 2, c_ui_main_style ) );
	button_quit_->SetCallback( [this]{ OnQuitButton(); } );

	game_title_.reset( new ui_Text( "H E X", button_shift_x, button_shift_y-5, 10, 2, c_ui_texts_style ) );
	game_subtitle_.reset( new ui_Text( "a game in world of hexogonal prisms", button_shift_x, button_shift_y-3, 10, 1, c_ui_texts_style ) );

	ui_MenuBase::elements_.push_back( button_play_.get() );
	ui_MenuBase::elements_.push_back( button_settings_.get() );
	ui_MenuBase::elements_.push_back( button_quit_.get() );

	ui_MenuBase::elements_.push_back( game_title_.get() );
	ui_MenuBase::elements_.push_back( game_subtitle_.get() );
}

ui_MainMenu::~ui_MainMenu()
{
}

void ui_MainMenu::OnPlayButton()
{
	main_loop_->StartGame();
}

void ui_MainMenu::OnSettingsButton()
{
	child_menu_.reset( new ui_SettingsMenu( this, settings_, 0, 0, size_x_, size_y_ ) );
	this->SetActive( false );
	this->SetVisible( false );
}

void ui_MainMenu::OnQuitButton()
{
	main_loop_->Quit();
}

void ui_MainMenu::KeyPress( ui_Key key )
{
	if( key == ui_Key::Escape ) OnQuitButton();
}

void ui_MainMenu::Tick()
{
	if( child_menu_ != nullptr )
		if( child_menu_->IsMarkedForKilling() )
		{
			child_menu_.reset();
			this->SetActive( true );
			this->SetVisible( true );
		}
}

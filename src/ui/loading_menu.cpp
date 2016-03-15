#include "styles.hpp"

#include "loading_menu.hpp"

ui_LoadingMenu::ui_LoadingMenu( unsigned int size_x, unsigned int size_y )
	: ui_MenuBase( nullptr, 0, 0, size_x, size_y )
{
	const int c_half_width= 8;

	int column= size_x / ui_Base::CellSize() / 2 - c_half_width;
	int row= size_y / ui_Base::CellSize() / 2 - 2;

	loading_text_.reset( new ui_Text( "", column, row, c_half_width * 2, 1, c_ui_texts_style ) );
	row++;

	progress_bar_.reset( new ui_ProgressBar( column, row, c_half_width * 2, 2, 0.0f, c_ui_texts_style ) );

	elements_.push_back( loading_text_.get() );
	elements_.push_back( progress_bar_.get() );

	SetProgress( 0.0f );
}

ui_LoadingMenu::~ui_LoadingMenu()
{
}

void ui_LoadingMenu::SetProgress( float progress )
{
	static const char* const c_texts[]=
	{
		"loading.  ",
		"loading.. ",
		"loading...",
	};

	loading_text_->SetText( c_texts[ int(progress * 9.0f) % 3 ] );
	progress_bar_->SetProgress( progress );
}


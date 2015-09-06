#include "../hex.hpp"

#include "ui_base_classes.hpp"
#include "ui_painter.hpp"

inline void ColorCopy( unsigned char* dst, const unsigned char* src )
{
	dst[0]= src[0];
	dst[1]= src[1];
	dst[2]= src[2];
	dst[3]= src[3];
}

/*
----------ui_CursorHandler-------------
*/

int ui_CursorHandler::ui_elements_count_= 0;
ui_Base* ui_CursorHandler::ui_elements_[ H_UI_MAX_ELEMENTS ];
int ui_CursorHandler::ui_menu_count_= 0;
ui_MenuBase* ui_CursorHandler::ui_menus_[ H_UI_MAX_MENUS ];

void ui_CursorHandler::AddUIElement( ui_Base* element )
{
	ui_elements_[ ui_elements_count_ ]= element;
	ui_elements_count_++;
}

void ui_CursorHandler::RemoveUIElement( ui_Base* element )
{
	for( int i= 0; i< ui_elements_count_; i++ )
	{
		if( ui_elements_[i] == element )
		{
			if( i != ui_elements_count_-1 )
				ui_elements_[i]= ui_elements_[ui_elements_count_-1];
			ui_elements_count_--;
			break;
		}
	}
}

void ui_CursorHandler::CursorPress( int x, int y, bool pressed )
{
	for( int i= 0; i< ui_elements_count_; i++ )
	{
		ui_Base* el= ui_elements_[i];
		if( !el->IsActive() )
			continue;
		//if inside element
		if( el->X() <= x && el->X()+el->SizeX() > x &&
				el->Y() <= y && el->Y()+el->SizeY() > y )
			el->CursorPress( x, y, pressed );
	}
}

void ui_CursorHandler::UpdateCursorPos( int x, int y )
{
	for( int i= 0; i< ui_elements_count_; i++ )
	{
		ui_Base* el= ui_elements_[i];
		if( !el->IsActive() )
			continue;
		//if inside element
		if( el->X() <= x && el->X()+el->SizeX() > x &&
				el->Y() <= y && el->Y()+el->SizeY() > y )
			el->CursorOver( true );
		else
			el->CursorOver( false );
	}
}

/*
-------------ui_Base------------------
*/

int ui_Base::ui_cell_size_= 32;

ui_Base::ui_Base()
	: is_active_(true), is_visible_(true)
	, color_ { 200, 200, 200, 128 }, cursor_over_color_ { 255, 255, 255, 64 }
{
	ui_CursorHandler::AddUIElement(this);
}

ui_Base::ui_Base( int x, int y, int in_size_x, int in_size_y,
				  const unsigned char* normal_color, const unsigned char* in_cursor_over_color )
	: pos_x_(x), pos_y_(y), size_x_(in_size_x), size_y_(in_size_y)
	, color_ { normal_color[0], normal_color[1], normal_color[2], normal_color[3] }
	, cursor_over_color_ { in_cursor_over_color[0], in_cursor_over_color[1], in_cursor_over_color[2], in_cursor_over_color[3] }
	, is_active_(true), is_visible_(true)
{
	ui_CursorHandler::AddUIElement(this);
}

ui_Base::ui_Base( int x, int y, int in_size_x, int in_size_y )
	: pos_x_(x), pos_y_(y), size_x_(in_size_x), size_y_(in_size_y)
	, color_ { 200, 200, 200, 128 }, cursor_over_color_ { 255, 255, 255, 64 }
	, is_active_(true),  is_visible_(true)
{
	ui_CursorHandler::AddUIElement(this);
}

ui_Base::~ui_Base()
{
	ui_CursorHandler::RemoveUIElement(this);
}

void ui_Base::CursorOver( bool is_over )
{
	if( !is_over )
		ColorCopy( current_color_, color_ );
	else
		ColorCopy( current_color_, cursor_over_color_ );
}

void ui_Base::CursorPress( int x, int y, bool pressed )
{
}

void ui_Base::Draw( ui_Painter* painter )const
{
}

int ui_Base::GenElementFraming( ui_Vertex* triangles, int trim_size ) const
{
	int co= trim_size;

	//left
	triangles[0].coord[0]= X();
	triangles[0].coord[1]= Y();
	triangles[1].coord[0]= X() + co;
	triangles[1].coord[1]= Y();
	triangles[2].coord[0]= X();
	triangles[2].coord[1]= Y() + SizeY() - co;
	triangles[3].coord[0]= X() + co;
	triangles[3].coord[1]= Y() + SizeY() - co;
	triangles[4].coord[0]= X() + co;
	triangles[4].coord[1]= Y();
	triangles[5].coord[0]= X();
	triangles[5].coord[1]= Y() + SizeY() - co;
	//top
	triangles[6].coord[0]= X() + co;
	triangles[6].coord[1]= Y();
	triangles[7].coord[0]= X() + SizeX();
	triangles[7].coord[1]= Y();
	triangles[8].coord[0]= X() + SizeX();
	triangles[8].coord[1]= Y() + co;
	triangles[9].coord[0]= X() + co;
	triangles[9].coord[1]= Y();
	triangles[10].coord[0]= X() + co;
	triangles[10].coord[1]= Y() + co;
	triangles[11].coord[0]= X() + SizeX();
	triangles[11].coord[1]= Y() + co;
	//right
	triangles[12].coord[0]= X() + SizeX() - co;
	triangles[12].coord[1]= Y() + co;
	triangles[13].coord[0]= X() + SizeX();
	triangles[13].coord[1]= Y() + co;
	triangles[14].coord[0]= X() + SizeX() - co;
	triangles[14].coord[1]= Y() + SizeY();
	triangles[15].coord[0]= X() + SizeX();
	triangles[15].coord[1]= Y() + SizeY();
	triangles[16].coord[0]= X() + SizeX();
	triangles[16].coord[1]= Y() + co;
	triangles[17].coord[0]= X() + SizeX() - co;
	triangles[17].coord[1]= Y() + SizeY();
	//bottom
	triangles[18].coord[0]= X();
	triangles[18].coord[1]= Y() + SizeY() - co;
	triangles[19].coord[0]= X() + SizeX() - co;
	triangles[19].coord[1]= Y() + SizeY() - co;
	triangles[20].coord[0]= X() + SizeX() - co;
	triangles[20].coord[1]= Y() + SizeY();
	triangles[21].coord[0]= X();
	triangles[21].coord[1]= Y() + SizeY() - co;
	triangles[22].coord[0]= X();
	triangles[22].coord[1]= Y() + SizeY();
	triangles[23].coord[0]= X() + SizeX() - co;
	triangles[23].coord[1]= Y() + SizeY();

	return 8*3;
}

int ui_Base::GenRectangle( ui_Vertex* triangles, int x, int y, int sx, int sy ) const
{
	triangles[0].coord[0]= x;
	triangles[0].coord[1]= y;
	triangles[1].coord[0]= x;
	triangles[1].coord[1]= y + sy;
	triangles[2].coord[0]= x + sx;
	triangles[2].coord[1]= y;

	triangles[3].coord[0]= x+sx;
	triangles[3].coord[1]= y + sy;
	triangles[4].coord[0]= x;
	triangles[4].coord[1]= y + sy;
	triangles[5].coord[0]= x + sx;
	triangles[5].coord[1]= y;
	return 6;
}
/*
---------------ui_MenuBase---------
*/

ui_MenuBase::ui_MenuBase( ui_MenuBase* parent, int x, int y, int sx, int sy )
	: pos_x_(x), pos_y_(y), size_x_(sx), size_y_(sy), parent_menu_(parent), child_menu_(nullptr), marked_for_killing_(false)
{
}

void ui_MenuBase::Draw( ui_Painter* painter )
{
for( auto el : elements_ )
	{
		if( el->IsVisible() )
			el->Draw( painter );
	}
	if( child_menu_ != nullptr )
		child_menu_->Draw( painter );

}

void ui_MenuBase::SetActive( bool active )
{
for( ui_Base* el : elements_ )
		el->SetActive( active );
}

void ui_MenuBase::SetVisible( bool visible )
{
for( ui_Base* el : elements_ )
		el->SetVisible( visible );
}

/*
---------------ui_Button-------------
*/

ui_Button::ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2 )
	, callback_(nullptr)
{
	if( text != nullptr )
		strcpy( button_text_, text );
	else
		button_text_[0]= 0;
}

ui_Button::ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y,
					  const unsigned char* normal_color, const unsigned char* in_cursor_over_color )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		normal_color, in_cursor_over_color )
	, callback_(nullptr)
{
	if( text != nullptr )
		strcpy( button_text_, text );
	else
		button_text_[0]= 0;
}

ui_Button::~ui_Button()
{
}

void ui_Button::CursorPress( int x, int y, bool pressed )
{
	if( !pressed )
	{
		if( callback_ ) callback_();
	}
}

void ui_Button::Draw( ui_Painter* painter )const
{
	ui_Vertex triangles[64];

	ui_Base::GenRectangle( triangles, X(), Y(), SizeX(), SizeY() );

	painter->DrawUITriangles( triangles, 6, current_color_ );

	unsigned char font_color[4];
	font_color[0]= current_color_[0]/2;
	font_color[1]= current_color_[1]/2;
	font_color[2]= current_color_[2]/2;
	font_color[3]= current_color_[3];

	painter->DrawUITextPixelCoordsCenter( button_text_, float(X()+ SizeX()/2), float(Y()), float(SizeY()), font_color );
}

void ui_Checkbox::CursorPress( int x, int y, bool pressed )
{
	if( pressed )
	{
		flag_= !flag_;
		if( callback_ ) callback_();
	}
}

/*
---------ui_Checkbox-------
*/

ui_Checkbox::ui_Checkbox( int cell_x, int cell_y, bool state )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		(cell_x+1) * ui_Base::CellSize() + ui_Base::CellOffset(), (cell_y+1) * ui_Base::CellSize() + ui_Base::CellOffset() )
	, flag_(state)
	, callback_()
{
}

ui_Checkbox::ui_Checkbox( int cell_x, int cell_y, bool state, const unsigned char* normal_color, const unsigned char* over_cursor_color )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		ui_Base::CellSize() + ui_Base::CellOffset(), ui_Base::CellSize() + ui_Base::CellOffset(),
		normal_color, over_cursor_color )
	, flag_(state)
	, callback_()
{
}

ui_Checkbox::~ui_Checkbox()
{
}

void ui_Checkbox::Draw( ui_Painter* painter )const
{
	ui_Vertex triangles[ 64 ];

	int co= ui_Base::CellOffset() * 3 / 2;
	int vertex_count= GenElementFraming( triangles, co );
	if( flag_ )
	{
		ui_Base::GenRectangle( triangles + vertex_count, X() + SizeX()/2 - co, Y() + SizeY()/2 - co, co*2, co*2 );
		vertex_count+= 6;
	}

	painter->DrawUITriangles( triangles, vertex_count, current_color_ );
}

/*
-------------ui_Text---------------
*/

ui_Text::ui_Text( const char* text, Alignment alignment, int cell_x, int cell_y , int cell_width, int cell_height)
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		ui_Base::CellSize() * cell_width - 2*ui_Base::CellOffset(), ui_Base::CellSize() * cell_height - 2*ui_Base::CellOffset() )
	, alignment_(alignment)
{
	if( text != nullptr )
		strcpy( text_, text );
}

ui_Text::ui_Text( const char* text, Alignment alignment, int cell_x, int cell_y, int cell_width, int cell_height, const unsigned char* color )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		ui_Base::CellSize() * cell_width - 2*ui_Base::CellOffset(), ui_Base::CellSize() * cell_height - 2*ui_Base::CellOffset(), color, color )
	, alignment_(alignment)
{
	if( text != nullptr )
		strcpy( text_, text );
}

ui_Text::~ui_Text()
{
}

void ui_Text::SetText( const char* text )
{
	if( text != nullptr )
		strcpy( text_, text );
}

void ui_Text::Draw( ui_Painter* painter )const
{
	switch(alignment_)
	{
	case Alignment::Right:
		painter->DrawUITextPixelCoordsRight( text_, float(X()+SizeX()), float(Y()), float(SizeY()), ui_Base::color_ );
		break;
	case Alignment::Center:
		painter->DrawUITextPixelCoordsCenter( text_, float(X()+SizeX()/2), float(Y()), float(SizeY()), ui_Base::color_ );
		break;
	case Alignment::Left:
	default:
		painter->DrawUITextPixelCoordsLeft( text_, float(X()), float(Y()), float(SizeY()), ui_Base::color_ );
		break;
	}
}

/*
-------ui_PorgressBar-----------
*/

ui_ProgressBar::ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2 )
{
	SetProgress(progress);
}

ui_ProgressBar::ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress,
								const unsigned char* normal_color, const unsigned char* cursor_over_color )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		normal_color, cursor_over_color )
{
	SetProgress(progress);
}

ui_ProgressBar::~ui_ProgressBar()
{
}

void ui_ProgressBar::SetProgress( float p )
{
	progress_= p;
	if( progress_< 0.0f ) progress_= 0.0f;
	else if( progress_ > 1.0f ) progress_= 1.0f;
}

void ui_ProgressBar::Draw( ui_Painter* painter ) const
{
	ui_Vertex triangles[64];

	int co= ui_Base::CellOffset() * 2;

	int progress_bar_len= SizeX() - co * 2;
	{
		int fixed12_progress= int(progress_ * 4096.0f );
		progress_bar_len= (progress_bar_len * fixed12_progress) >>12;
	}
	int progress_bar_begin= X() + co;

	ui_Base::GenRectangle( triangles, progress_bar_begin, Y()+co, progress_bar_len, SizeY()-co*2 );
	int vertex_count= GenElementFraming( triangles + 6, ui_Base::CellOffset() );

	painter->DrawUITriangles( triangles, vertex_count + 6, color_ );

	unsigned char font_color[4];
	font_color[0]= current_color_[0]/2;
	font_color[1]= current_color_[1]/2;
	font_color[2]= current_color_[2]/2;
	font_color[3]= current_color_[3];

	char text[64];
	sprintf( text, "%d%%", int(round( 100.0f * progress_) ) );

	painter->DrawUIText( text, float(X() + SizeX()/2), float(Y()+ SizeY()/2), 1.0f, font_color );
}

/*
---------ui_Slider------------
*/

ui_Slider::ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		1 * ui_Base::CellSize() - ui_Base::CellOffset()*2 )
	, slider_pos_(slider_pos), slider_inv_step_(16), callback_()
{
}

ui_Slider::ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos, const unsigned char* normal_color, const unsigned char* cursor_over_color )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		1 * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		normal_color, cursor_over_color )
	, slider_pos_(slider_pos), slider_inv_step_(16), callback_()
{
}
ui_Slider::~ui_Slider()
{
}

void ui_Slider::SetSliderPos( float pos )
{
	if( pos < 0.0f ) slider_pos_= 0.0f;
	else if( pos > 1.0f ) slider_pos_= 1.0f;
	else slider_pos_= pos;
}

void ui_Slider::Draw( ui_Painter* painter )const
{
	ui_Vertex triangles[64];

	//   <  and  >   arrows
	triangles[0].coord[0]= X();
	triangles[0].coord[1]= Y() + SizeY() / 2;
	triangles[1].coord[0]= X() + ArrowSize();
	triangles[1].coord[1]= Y();
	triangles[2].coord[0]= X() + ArrowSize();
	triangles[2].coord[1]= Y() + SizeY();

	triangles[3].coord[0]= X() + SizeX();
	triangles[3].coord[1]= Y() + SizeY() / 2;
	triangles[4].coord[0]= X() + SizeX() - ArrowSize();
	triangles[4].coord[1]= Y();
	triangles[5].coord[0]= X() + SizeX() - ArrowSize();
	triangles[5].coord[1]= Y() + SizeY();

	//slider bar
	int bar_half_size= ui_Base::CellOffset();
	int bar_arrow_offset= ArrowOffset();
	ui_Base::GenRectangle(  triangles + 6, X() + ArrowSize() + bar_arrow_offset, Y() + SizeY()/2 - bar_half_size,
							SizeX() - 2 * ( ArrowSize() + bar_arrow_offset ), bar_half_size * 2 );

	int slider_rel_pos= SizeX()/2;
	{
		int bar_length= SizeX() - 2 * ( ArrowSize() + bar_arrow_offset );
		slider_rel_pos= (int)( float(bar_length) * slider_pos_ );
		slider_rel_pos+= ArrowSize() + bar_arrow_offset;
	}
	//slider
	ui_Base::GenRectangle( triangles + 12, X() + slider_rel_pos - SliderHalfSize(), Y(), SliderHalfSize() * 2 , SizeY() );

	painter->DrawUITriangles( triangles, 18, current_color_ );
}

void ui_Slider::CursorPress( int x, int y, bool pressed )
{
	if( !pressed )
		return;

	float slider_step= 1.0f / float(slider_inv_step_);
	if( x < X()+ArrowSize() )
	{
		slider_pos_-= slider_step;
		if( slider_pos_ < 0.0f ) slider_pos_= 0.0f;
	}
	else if( x > X() + SizeX() - ArrowSize() )
	{
		slider_pos_+= slider_step;
		if( slider_pos_ > 1.0f ) slider_pos_= 1.0f;
	}
	else if( x > X() + ArrowSize() + ArrowOffset() && x < X() + SizeX() - ArrowSize() - ArrowOffset() )
	{
		int rel_pos= x - ( X() + ArrowOffset() + ArrowSize() );
		int real_bar_size= SizeX() - 2 * ( ArrowSize() + ArrowOffset() );

		slider_pos_= float(rel_pos) / float(real_bar_size);
	}

	if( callback_ ) callback_();
}
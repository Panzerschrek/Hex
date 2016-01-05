#include "../hex.hpp"
#include "../math_lib/assert.hpp"

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

unsigned int ui_CursorHandler::ui_elements_count_= 0;
std::array<ui_Base*, H_UI_MAX_ELEMENTS> ui_CursorHandler::ui_elements_;
unsigned int ui_CursorHandler::ui_menu_count_= 0;
std::array<ui_MenuBase*, H_UI_MAX_MENUS> ui_CursorHandler::ui_menus_;
bool ui_CursorHandler::mouse_grabbed_= false;

void ui_CursorHandler::AddUIElement( ui_Base* element )
{
	H_ASSERT( ui_elements_count_ <= ui_elements_.size() );

	ui_elements_[ ui_elements_count_ ]= element;
	ui_elements_count_++;
}

void ui_CursorHandler::RemoveUIElement( ui_Base* element )
{
	for( unsigned int i= 0; i< ui_elements_count_; i++ )
	{
		if( ui_elements_[i] == element )
		{
			if( i != ui_elements_count_ - 1 )
				ui_elements_[i]= ui_elements_[ ui_elements_count_ - 1 ];
			ui_elements_count_--;
			return;
		}
	}

	H_ASSERT( false );
}

void ui_CursorHandler::AddMenu(ui_MenuBase *menu )
{
	H_ASSERT( ui_menu_count_ <= ui_menus_.size() );

	ui_menus_[ ui_menu_count_ ]= menu;
	ui_menu_count_++;
}

void ui_CursorHandler::RemoveMenu( ui_MenuBase* menu )
{
	for( unsigned int i= 0; i< ui_menu_count_; i++ )
	{
		if( ui_menus_[i] == menu )
		{
			if( i != ui_menu_count_ - 1 )
				ui_menus_[i]= ui_menus_[ ui_menu_count_ - 1 ];
			ui_menu_count_--;
			return;
		}
	}

	H_ASSERT( false );
}

void ui_CursorHandler::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	ui_MouseButtonMask button_bit= static_cast<ui_MouseButtonMask>(button);

	for( unsigned int i= 0; i < ui_menu_count_; i++ )
	{
		if( ui_menus_[i]->IsActive() && (ui_menus_[i]->AcceptedMouseButtons() & button_bit) )
			ui_menus_[i]->CursorPress( x, y, button, pressed );
	}

	for( unsigned int i= 0; i< ui_elements_count_; i++ )
	{
		ui_Base* el= ui_elements_[i];
		if( !el->IsActive() )
			continue;

		if( el->AcceptedMouseButtons() & button_bit )
			//if inside element
			if( el->X() <= x && el->X()+el->SizeX() > x &&
				el->Y() <= y && el->Y()+el->SizeY() > y )
					el->CursorPress( x, y, button, pressed );
	}
}

void ui_CursorHandler::UpdateCursorPos( int x, int y )
{
	for( unsigned int i= 0; i< ui_elements_count_; i++ )
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

void ui_CursorHandler::ControllerMove( int dx, int dy )
{
	for( unsigned int i= 0; i < ui_menu_count_; i++ )
	{
		if( ui_menus_[i]->IsActive() )
			ui_menus_[i]->ControllerMove( dx, dy );
	}
}

void ui_CursorHandler::GrabMouse( bool grab )
{
	mouse_grabbed_= grab;
}

bool ui_CursorHandler::IsMouseGrabbed()
{
	return mouse_grabbed_;
}

/*
-------------ui_Style----------------
*/

const unsigned char ui_Style::c_default_color[4]=
	{ 200, 200, 200, 128 };
const unsigned char ui_Style::c_hover_default_color[4]=
	{ 255, 255, 255,  64 };
const unsigned char ui_Style::c_text_default_color[4]=
	{ 100, 100, 100,  64 };
const unsigned char ui_Style::c_text_hover_default_color[4]=
	{ 128, 128, 128,  32 };

ui_Style::ui_Style(
	const unsigned char* in_color,
	const unsigned char* in_hover_color,
	TextAlignment in_text_alignment,
	const unsigned char* in_text_color,
	const unsigned char* in_text_hover_color )
	: color{ in_color[0], in_color[1], in_color[2], in_color[3] }
	, hover_color{ in_hover_color[0], in_hover_color[1], in_hover_color[2], in_hover_color[3] }
	, text_color{ in_text_color[0], in_text_color[1], in_text_color[2], in_text_color[3] }
	, text_hover_color{ in_text_hover_color[0], in_text_hover_color[1], in_text_hover_color[2], in_text_hover_color[3] }
	, text_alignment( in_text_alignment )
{
}

/*
-------------ui_Base------------------
*/

int ui_Base::ui_cell_size_= 32;


ui_Base::ui_Base(
	int x, int y, int in_size_x, int in_size_y,
	const ui_Style& style )
	: pos_x_(x), pos_y_(y), size_x_(in_size_x), size_y_(in_size_y)
	, style_( style )
	, is_active_(true), is_visible_(true)
{
	ColorCopy( current_color_, style_.color );
	ColorCopy( current_text_color_, style_.text_color );

	ui_CursorHandler::AddUIElement(this);
}

ui_Base::~ui_Base()
{
	ui_CursorHandler::RemoveUIElement(this);
}

void ui_Base::CursorOver( bool is_over )
{
	if( !is_over )
	{
		ColorCopy( current_color_, style_.color );
		ColorCopy( current_text_color_, style_.text_color );
	}
	else
	{
		ColorCopy( current_color_, style_.hover_color );
		ColorCopy( current_text_color_, style_.text_hover_color );
	}
}

void ui_Base::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	(void)x;
	(void)y;
	(void)button;
	(void)pressed;
}

ui_MouseButtonMask ui_Base::AcceptedMouseButtons() const
{
	return static_cast<ui_MouseButtonMask>(ui_MouseButton::Left);
}

void ui_Base::Draw( ui_Painter* painter ) const
{
	(void)painter;
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

void ui_Base::TextDraw( ui_Painter* painter, const char* text ) const
{
	switch( style_.text_alignment )
	{
	case ui_Style::TextAlignment::Right:
		painter->DrawUITextPixelCoordsRight( text, float(X()+SizeX()), float(Y()), float(SizeY()), current_text_color_ );
		break;
	case ui_Style::TextAlignment::Center:
		painter->DrawUITextPixelCoordsCenter( text, float(X()+SizeX()/2), float(Y()), float(SizeY()), current_text_color_ );
		break;
	case ui_Style::TextAlignment::Left:
	default:
		painter->DrawUITextPixelCoordsLeft( text, float(X()), float(Y()), float(SizeY()), current_text_color_ );
		break;
	}
}

/*
---------------ui_MenuBase---------
*/

ui_MenuBase::ui_MenuBase( ui_MenuBase* parent, int x, int y, int sx, int sy )
	: parent_menu_(parent), child_menu_(nullptr)
	, active_(true), visible_(true)
	, pos_x_(x), pos_y_(y)
	, size_x_(sx), size_y_(sy)
	, marked_for_killing_(false)
{
	ui_CursorHandler::AddMenu( this );
}

ui_MenuBase::~ui_MenuBase()
{
	ui_CursorHandler::RemoveMenu( this );
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
	active_= active;
	for( ui_Base* el : elements_ )
		el->SetActive( active );
}

void ui_MenuBase::SetVisible( bool visible )
{
	visible_= visible;
	for( ui_Base* el : elements_ )
		el->SetVisible( visible );
}

ui_MouseButtonMask ui_MenuBase::AcceptedMouseButtons() const
{
	return static_cast<ui_MouseButtonMask>(ui_MouseButton::Left);
}

void ui_MenuBase::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	(void)x;
	(void)y;
	(void)button;
	(void)pressed;
}

void ui_MenuBase::ControllerMove( int dx, int dy )
{
	(void)dx;
	(void)dy;
}

/*
---------------ui_Button-------------
*/

ui_Button::ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y, const ui_Style& style )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		style )
	, callback_(nullptr)
	, button_text_(text ? text : "")
{
}

ui_Button::~ui_Button()
{
}

void ui_Button::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	(void)x;
	(void)y;
	(void)button;

	if( !pressed )
	{
		if( callback_ ) callback_();
	}
}

void ui_Button::Draw( ui_Painter* painter ) const
{
	ui_Vertex triangles[64];

	ui_Base::GenRectangle( triangles, X(), Y(), SizeX(), SizeY() );

	painter->DrawUITriangles( triangles, 6, current_color_ );

	ui_Base::TextDraw( painter, button_text_.data() );
}

/*
---------ui_Checkbox-------
*/

ui_Checkbox::ui_Checkbox( int cell_x, int cell_y, bool state, const ui_Style& style )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		(cell_x+1) * ui_Base::CellSize() + ui_Base::CellOffset(), (cell_y+1) * ui_Base::CellSize() + ui_Base::CellOffset(),
		style )
	, flag_(state)
	, callback_()
{
}

ui_Checkbox::~ui_Checkbox()
{
}

void ui_Checkbox::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	(void)x;
	(void)y;
	(void)button;

	if( pressed )
	{
		flag_= !flag_;
		if( callback_ ) callback_();
	}
}

void ui_Checkbox::Draw( ui_Painter* painter ) const
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

ui_Text::ui_Text( const char* text, int cell_x, int cell_y , int cell_width, int cell_height, const ui_Style& style )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(), cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		ui_Base::CellSize() * cell_width - 2*ui_Base::CellOffset(), ui_Base::CellSize() * cell_height - 2*ui_Base::CellOffset(),
		style )
	, text_(text ? text : "")
{
}

ui_Text::~ui_Text()
{
}

void ui_Text::SetText( const char* text )
{
	text_= text ? text : "";
}

void ui_Text::Draw( ui_Painter* painter )const
{
	ui_Base::TextDraw( painter, text_.data() );
}

/*
-------ui_PorgressBar-----------
*/

ui_ProgressBar::ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress, const ui_Style& style )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		cell_size_y * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		style )
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

	painter->DrawUITriangles( triangles, vertex_count + 6, style_.color );

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

ui_Slider::ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos, const ui_Style& style )
	: ui_Base(
		cell_x * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_y * ui_Base::CellSize() + ui_Base::CellOffset(),
		cell_size_x * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		1 * ui_Base::CellSize() - ui_Base::CellOffset()*2,
		style )
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

void ui_Slider::Draw( ui_Painter* painter ) const
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
	ui_Base::GenRectangle(
		triangles + 6, X() + ArrowSize() + bar_arrow_offset, Y() + SizeY()/2 - bar_half_size,
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

void ui_Slider::CursorPress( int x, int y, ui_MouseButton button, bool pressed )
{
	(void)y;
	(void)button;

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
		slider_pos_= float( int(slider_pos_ * float(slider_inv_step_) + 0.5f) ) / float(slider_inv_step_);
	}

	if( callback_ ) callback_();
}

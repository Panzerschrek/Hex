#include <cmath>

#include "vec.hpp"
#include "../math_lib/math.hpp"

#include "../block.hpp"
#include "../main_loop.hpp"
#include "../player.hpp"
#include "styles.hpp"
#include "ui_painter.hpp"

#include "ingame_menu.hpp"

static const ui_Key g_inventory_key= ui_Key::E;
static const ui_Key g_fly_mode_key= ui_Key::Z;

static const ui_Key g_jump_key= ui_Key::Space;
static const ui_Key g_down_key= ui_Key::C;

static const ui_Key g_forward_key= ui_Key::W;
static const ui_Key g_backward_key= ui_Key::S;
static const ui_Key g_left_key= ui_Key::A;
static const ui_Key g_right_key= ui_Key::D;

class ui_BlockSelectMenu final : public ui_MenuBase
{
public:
	typedef std::function<void(h_BlockType)> BlockSelectCallback;

	ui_BlockSelectMenu(
		ui_MenuBase* parent,
		int x, int y,
		int sx, int sy,
		const BlockSelectCallback& block_select_callback )
		: ui_MenuBase( parent, x, y, sx, sy )
		, block_select_callback_(block_select_callback)
	{
		unsigned int column= 1;
		unsigned int row= 2;

		ui_Style title_style= c_ui_texts_style;
		title_style.text_alignment= ui_Style::TextAlignment::Left;
		title_.reset( new ui_Text("Select block:", column, row, 12, 1, title_style ) );
		ui_MenuBase::elements_.push_back(title_.get());

		for( unsigned int i= ((unsigned int)h_BlockType::Air) + 1; i < (unsigned int)h_BlockType::NumBlockTypes; i++ )
		{
			std::unique_ptr<ui_Button> button(
				new ui_Button(
					h_Block::GetBlockName(static_cast<h_BlockType>(i)),
					column, row + 1 + ( i - (((unsigned int)h_BlockType::Air) + 1 ) ),
					8, 1,
					c_ui_main_style ) );

			button->SetCallback(
				[this, i]
				{
					block_select_callback_(static_cast<h_BlockType>(i));
				});

			ui_MenuBase::elements_.push_back(button.get());
			buttons_.push_back( std::move(button) );
		}
	}

	virtual ~ui_BlockSelectMenu() override {}

	virtual void KeyPress( ui_Key key ) override
	{
		if( key == ui_Key::Escape || key == g_inventory_key )
			this->Kill();
	}

	virtual void Tick() override {}

private:
	const BlockSelectCallback block_select_callback_;

	std::unique_ptr< ui_Text > title_;
	std::vector< std::unique_ptr<ui_Button> > buttons_;
};

class ui_EscMenu final : public ui_MenuBase
{
public:
	ui_EscMenu(
		ui_MenuBase* parent,
		int x, int y,
		int sx, int sy,
		h_MainLoop& main_loop )
		: ui_MenuBase( parent, x, y, sx, sy )
		, main_loop_(main_loop)
	{
		unsigned int row, column;
		const unsigned int c_button_half_width= 5;
		row= SizeY() / ui_Base::CellSize() / 2 - 1;
		column= SizeX() / ui_Base::CellSize() / 2 - c_button_half_width;

		resume_button_.reset( new ui_Button( "resume", column, row, c_button_half_width*2, 1, c_ui_texts_style ) );
		resume_button_->SetCallback( [this]{ Kill(); } );
		row++;

		quit_to_main_menu_button_.reset( new ui_Button( "quit to main menu", column, row++, c_button_half_width*2, 1, c_ui_texts_style ) );
		quit_to_main_menu_button_->SetCallback(
		[this]
		{
			main_loop_.QuitToMainMenu();
		} );

		ui_MenuBase::elements_.push_back(resume_button_.get());
		ui_MenuBase::elements_.push_back(quit_to_main_menu_button_.get());
	}

	virtual void Draw( ui_Painter* painter ) override
	{
		// Draw dark background.
		static const unsigned char c_color[4]= { 0, 0, 0, 0x90 };

		ui_Vertex v[6];
		v[0].coord[0]= 0;
		v[0].coord[1]= 0;
		v[1].coord[0]= size_x_;
		v[1].coord[1]= 0;
		v[2].coord[0]= size_x_;
		v[2].coord[1]= size_y_;
		v[3].coord[0]= 0;
		v[3].coord[1]= size_y_;
		v[4]= v[0];
		v[5]= v[2];

		painter->DrawUITriangles( v, 6, c_color );
		ui_MenuBase::Draw( painter );
	}

	virtual void KeyPress( ui_Key key ) override
	{
		if( key == ui_Key::Escape ) this->Kill();
	}

	virtual void Tick() override
	{}

private:
	h_MainLoop& main_loop_;

	std::unique_ptr<ui_Button> resume_button_;
	std::unique_ptr<ui_Button> quit_to_main_menu_button_;
};

ui_IngameMenu::ui_IngameMenu(
	int sx, int sy,
	const h_PlayerPtr& player,
	h_MainLoop& main_loop )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, main_loop_(main_loop)
	, player_(player)
	, game_paused_(false)
{
	int row= sy / ui_Base::CellSize() - 2;

	ui_Style title_style= c_ui_texts_style;
	title_style.text_alignment= ui_Style::TextAlignment::Left;
	block_type_text_.reset( new ui_Text("", 1, row, 20, 1, title_style ) );
	OnBlockSelected( h_BlockType::Unknown );

	ui_MenuBase::elements_.push_back( block_type_text_.get() );
}

ui_IngameMenu::~ui_IngameMenu()
{
	ui_CursorHandler::GrabMouse( false );
}

ui_MouseButtonMask ui_IngameMenu::AcceptedMouseButtons() const
{
	return
		static_cast<ui_MouseButtonMask>(ui_MouseButton::Left) |
		static_cast<ui_MouseButtonMask>(ui_MouseButton::Right) |
		static_cast<ui_MouseButtonMask>(ui_MouseButton::Middle);
}

void ui_IngameMenu::CursorPress(int x, int y, ui_MouseButton button, bool pressed )
{
	(void)x;
	(void)y;

	if( this->IsActive() && pressed )
	{
		if( button == ui_MouseButton::Right )
			player_->Build();
		else if( button == ui_MouseButton::Left )
			player_->Dig();
		//else if( button == ui_MouseButton::Middle )
		//	player_->TestMobSetPosition();
	}
}

void ui_IngameMenu::KeyPress( ui_Key key )
{
	keys_[ key ]= true;

	if( !this->IsActive() && child_menu_ )
	{
		child_menu_->KeyPress( key );
		return;
	}

	if( key == g_inventory_key )
	{
		child_menu_.reset(
			new ui_BlockSelectMenu(
				this,
				0, 0,
				size_x_, size_y_,
				std::bind(&ui_IngameMenu::OnBlockSelected, this, std::placeholders::_1) ) );

		this->SetActive( false );
		for( auto& key : keys_ ) key.second= false;
	}
	else if( key == ui_Key::Escape )
	{
		child_menu_.reset(
			new ui_EscMenu(
				this,
				0, 0,
				size_x_, size_y_,
				main_loop_ ) );

		this->SetActive( false );
		for( auto& key : keys_ ) key.second= false;

		player_->PauseWorldUpdates();
		game_paused_= true;
	}
	else if( key == g_jump_key )
		player_->Jump();
	else if( key == g_fly_mode_key )
		player_->ToggleFly();
}

void ui_IngameMenu::KeyRelease( ui_Key key )
{
	keys_[ key ]= false;
}

void ui_IngameMenu::ControllerMove( int dx, int dy )
{
	if( this->IsActive() )
	{
		const float c_angle_delta_per_screen_x= 2.0f;
		const float c_angle_delta_per_screen_y= 2.0f;

		m_Vec3 ang_delta(
			c_angle_delta_per_screen_y * float(dy) / float( SizeX() ),
			0.0f,
			c_angle_delta_per_screen_x * float(dx) / float( SizeY() ));

		player_->Rotate( ang_delta );
	}
}

void ui_IngameMenu::Tick()
{
	ui_CursorHandler::GrabMouse( this->IsActive() );

	if( child_menu_ && child_menu_->IsMarkedForKilling() )
	{
		child_menu_.reset();
		this->SetActive( true );

		if( game_paused_ )
		{
			game_paused_= false;
			player_->UnpauseWorldUpdates();
		}
	}

	if( this->IsActive() )
	{
		m_Vec3 move_direction( 0.0f, 0.0f, 0.0f );

		if( keys_[ g_forward_key  ] ) move_direction.y+= 1.0f;
		if( keys_[ g_backward_key ] ) move_direction.y-= 1.0f;

		if( keys_[ g_left_key  ] ) move_direction.x-= 1.0f;
		if( keys_[ g_right_key ] ) move_direction.x+= 1.0f;

		if( keys_[ g_jump_key ] ) move_direction.z+= 1.0f;
		if( keys_[ g_down_key ] ) move_direction.z-= 1.0f;

		player_->SetMovingVector( move_direction );
	}
	else
		player_->SetMovingVector( m_Vec3( 0.0f, 0.0f, 0.0f ) );

	if( !game_paused_ )
		player_->Tick();
}

void ui_IngameMenu::OnBlockSelected( h_BlockType block_type )
{
	player_->SetBuildBlock( block_type );

	if( block_type == h_BlockType::Unknown )
		block_type_text_->SetText("");
	else
	{
		std::string str= "Block: ";
		str+= h_Block::GetBlockName( block_type );
		block_type_text_->SetText( str.data() );
	}
}

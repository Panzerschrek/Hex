#include <cmath>

#include "vec.hpp"
#include "../math_lib/m_math.h"

#include "../block.hpp"
#include "../player.hpp"
#include "styles.hpp"

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
		ui_Text* title= new ui_Text("Select block:", column, row, 12, 1, title_style );
		ui_MenuBase::elements_.push_back(title);

		for( unsigned short i= AIR+1; i < NUM_BLOCK_TYPES; i++ )
		{
			ui_Button* button=
				new ui_Button(
				h_Block::GetBlockName(h_BlockType(i)),
				column, row + 1 + (i-(AIR+1)),
				8, 1,
				c_ui_main_style );

			button->SetCallback(
				[this, i]
				{
					block_select_callback_(h_BlockType(i));
				});

			ui_MenuBase::elements_.push_back(button);
		}
	}

	virtual ~ui_BlockSelectMenu() override
	{
		for( ui_Base* el : ui_MenuBase::elements_ )
			delete el;
	}

	virtual void KeyPress( ui_Key key ) override
	{
		if( key == ui_Key::Escape || key == g_inventory_key )
			this->Kill();
	}

	virtual void Tick() override {}

private:
	const BlockSelectCallback block_select_callback_;
};

ui_IngameMenu::ui_IngameMenu(
	int sx, int sy,
	const h_PlayerPtr& player )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, player_(player)
{
	int row= sy / ui_Base::CellSize() - 2;

	ui_Style title_style= c_ui_texts_style;
	title_style.text_alignment= ui_Style::TextAlignment::Left;
	block_type_text_= new ui_Text("", 1, row, 20, 1, title_style );
	OnBlockSelected( BLOCK_UNKNOWN );

	ui_MenuBase::elements_.push_back(block_type_text_);
}

ui_IngameMenu::~ui_IngameMenu()
{
	delete block_type_text_;

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
		else if( button == ui_MouseButton::Middle )
			player_->TestMobSetPosition();
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
		child_menu_=
			new ui_BlockSelectMenu(
				this,
				0, 0,
				size_x_, size_y_,
				std::bind(&ui_IngameMenu::OnBlockSelected, this, std::placeholders::_1) );

		this->SetActive( false );

		for( auto& key : keys_ ) key.second= false;
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
		delete child_menu_;
		child_menu_= nullptr;
		this->SetActive( true );
	}

	m_Vec3 move_direction( 0.0f, 0.0f, 0.0f );

	if( this->IsActive() )
	{
		float cam_ang_z= player_->Angle().z;

		if( keys_[ g_forward_key ] )
		{
			move_direction.y+= std::cos( cam_ang_z );
			move_direction.x-= std::sin( cam_ang_z );
		}
		if( keys_[ g_backward_key ] )
		{
			move_direction.y-= std::cos( cam_ang_z );
			move_direction.x+= std::sin( cam_ang_z );
		}
		if( keys_[ g_left_key ] )
		{
			move_direction.y-= std::cos( cam_ang_z - m_Math::FM_PI2 );
			move_direction.x+= std::sin( cam_ang_z - m_Math::FM_PI2 );
		}
		if( keys_[ g_right_key ] )
		{
			move_direction.y-= std::cos( cam_ang_z + m_Math::FM_PI2 );
			move_direction.x+= std::sin( cam_ang_z + m_Math::FM_PI2 );
		}

		if( keys_[ g_jump_key ] ) move_direction.z+= 1.0f;
		if( keys_[ g_down_key ] ) move_direction.z-= 1.0f;
	}

	player_->Move( move_direction );
}

void ui_IngameMenu::OnBlockSelected( h_BlockType block_type )
{
	player_->SetBuildBlock( block_type );

	if( block_type == BLOCK_UNKNOWN )
		block_type_text_->SetText("");
	else
	{
		std::string str= "Block: ";
		str+= h_Block::GetBlockName( block_type );
		block_type_text_->SetText( str.data() );
	}
}

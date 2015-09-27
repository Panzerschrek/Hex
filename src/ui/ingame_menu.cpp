#include "../block.hpp"
#include  "styles.hpp"

#include "ingame_menu.hpp"

static const ui_Key g_inventory_key= ui_Key::E;

class ui_BlockSelectMenu final : public ui_MenuBase
{
public:
	ui_BlockSelectMenu( ui_MenuBase* parent, int x, int y, int sx, int sy )
		: ui_MenuBase( parent, x, y, sx, sy )
	{
		for( unsigned short i= AIR+1; i < NUM_BLOCK_TYPES; i++ )
		{
			ui_Button* button=
				new ui_Button(
				h_Block::GetBlockName(h_BlockType(i)),
				5, 3 + i,
				8, 1,
				c_ui_main_style );

			// TODO - add real selection
			button->SetCallback(
				[this, i]
				{
					printf("Block %s selected\n", h_Block::GetBlockName(h_BlockType(i)));
				});

			ui_MenuBase::elements_.push_back(button);
		}
	}

	virtual ~ui_BlockSelectMenu() override
	{
		for( ui_Base* el : ui_MenuBase::elements_ )
		{
			delete el;
		}
	}

	virtual void KeyPress( ui_Key key ) override
	{
		if( key == ui_Key::Escape || key == g_inventory_key )
		{
			this->Kill();
		}
	}

	virtual void Tick() override
	{
	}

private:
};

ui_IngameMenu::ui_IngameMenu(int sx, int sy)
	: ui_MenuBase( nullptr, 0, 0, sx, sy)
{
}

ui_IngameMenu::~ui_IngameMenu()
{
}

void ui_IngameMenu::KeyPress( ui_Key key )
{
	if (!this->IsActive() && child_menu_)
	{
		child_menu_->KeyPress( key );
		return;
	}

	if( key == g_inventory_key )
	{
		child_menu_= new ui_BlockSelectMenu( this, 0, 0, size_x_, size_y_ );
		this->SetActive( false );
		this->SetVisible( false );
	}
}

void ui_IngameMenu::Tick()
{
	if( child_menu_ && child_menu_->IsMarkedForKilling() )
	{
		delete child_menu_;
		child_menu_= nullptr;
		this->SetActive( true );
		this->SetVisible( true );
	}
}

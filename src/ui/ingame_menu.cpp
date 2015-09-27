#include "../block.hpp"
#include  "styles.hpp"

#include "ingame_menu.hpp"

static const ui_Key g_inventory_key= ui_Key::E;

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
	std::function<void(h_BlockType)> hack_on_block_type_changed_callback  )
	: ui_MenuBase( nullptr, 0, 0, sx, sy )
	, active_block_type_(BLOCK_UNKNOWN)
	, hack_on_block_type_changed_callback_(hack_on_block_type_changed_callback)
{
	int row= sy / ui_Base::CellSize() - 2;

	ui_Style title_style= c_ui_texts_style;
	title_style.text_alignment= ui_Style::TextAlignment::Left;
	block_type_text_= new ui_Text("", 1, row, 20, 1, title_style );
	OnBlockSelected(BLOCK_UNKNOWN);

	ui_MenuBase::elements_.push_back(block_type_text_);
}

ui_IngameMenu::~ui_IngameMenu()
{
	delete block_type_text_;
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
		child_menu_=
			new ui_BlockSelectMenu(
				this,
				0, 0,
				size_x_, size_y_,
				std::bind(&ui_IngameMenu::OnBlockSelected, this, std::placeholders::_1) );
		this->SetActive( false );
	}
}

void ui_IngameMenu::Tick()
{
	if( child_menu_ && child_menu_->IsMarkedForKilling() )
	{
		delete child_menu_;
		child_menu_= nullptr;
		this->SetActive( true );
	}
}

void ui_IngameMenu::OnBlockSelected( h_BlockType block_type )
{
	active_block_type_= block_type;

	if( block_type == BLOCK_UNKNOWN )
		block_type_text_->SetText("");
	else
	{
		std::string str= "Block: ";
		str+= h_Block::GetBlockName( block_type );
		block_type_text_->SetText( str.data() );
	}

	hack_on_block_type_changed_callback_(block_type);
}

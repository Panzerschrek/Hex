#pragma once
#include <functional>

#include "../hex.hpp"
#include "ui_base_classes.hpp"

class ui_IngameMenu final : public ui_MenuBase
{
public:
	ui_IngameMenu(
		int sx, int sy,
		// TODO - this paremater is hack. Invent, how to do build and other actions.
		std::function<void(h_BlockType)> hack_on_block_type_changed_callback );

	virtual ~ui_IngameMenu() override;

	virtual void KeyPress( ui_Key key ) override;
	virtual void Tick() override;

private:
	void OnBlockSelected(h_BlockType block_type);

private:
	h_BlockType active_block_type_;
	ui_Text* block_type_text_;

	std::function<void(h_BlockType)> hack_on_block_type_changed_callback_;
};

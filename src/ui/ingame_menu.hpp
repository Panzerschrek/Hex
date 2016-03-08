#pragma once
#include <ctime>
#include <functional>
#include <map>

#include "../hex.hpp"
#include "../fwd.hpp"
#include "ui_base_classes.hpp"

class ui_IngameMenu final : public ui_MenuBase
{
public:
	ui_IngameMenu(
		int sx, int sy,
		const h_PlayerPtr& player,
		h_MainLoop& main_loop );

	virtual ~ui_IngameMenu() override;

	virtual ui_MouseButtonMask AcceptedMouseButtons() const override;

	virtual void CursorPress( int x, int y, ui_MouseButton button, bool pressed ) override;
	virtual void KeyPress( ui_Key key ) override;
	virtual void KeyRelease( ui_Key key ) override;
	virtual void ControllerMove( int dx, int dy ) override;

	virtual void Tick() override;

private:
	void OnBlockSelected( h_BlockType block_type );

private:
	h_MainLoop& main_loop_;
	const h_PlayerPtr player_;
	bool game_paused_;

	std::map<ui_Key, bool> keys_;

	std::unique_ptr<ui_Text> block_type_text_;

};

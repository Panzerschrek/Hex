#pragma once

#include "ui_base_classes.hpp"

class ui_IngameMenu final : public ui_MenuBase
{
public:
	ui_IngameMenu( int sx, int sy );
	virtual ~ui_IngameMenu() override;

	virtual void KeyPress( ui_Key key ) override;
	virtual void Tick() override;

private:
};

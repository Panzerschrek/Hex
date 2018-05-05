#pragma once
#include "ui_base_classes.hpp"

class ui_ConsoleMenu final : public ui_MenuBase
{
public:
	ui_ConsoleMenu( ui_MenuBase* parent, unsigned int size_x, unsigned int size_y );
	virtual ~ui_ConsoleMenu() override;

	virtual void KeyPress( ui_Key key ) override;
	virtual void KeyRelease( ui_Key key ) override;
	virtual void Tick() override;

	virtual void Draw( ui_Painter* painter ) override;

public:
	static constexpr ui_Key c_activation_key= ui_Key::GraveAccent;

private:
	float position_= 0.0f;
	bool retracting_= true;

	uint64_t prev_tick_time_;
};

#pragma once
#include "ui_base_classes.hpp"

class ui_LoadingMenu final : public ui_MenuBase
{
public:
	ui_LoadingMenu( unsigned int size_x, unsigned int size_y );
	~ui_LoadingMenu();

	virtual void Tick() override
	{}

public:
	void SetProgress( float progress );

private:
	std::unique_ptr<ui_Text> loading_text_;
	std::unique_ptr<ui_ProgressBar> progress_bar_;
};

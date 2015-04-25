#pragma once
#include <vector>
#include <functional>

#define H_UI_MAX_ELEMENTS 128
#define H_UI_MAX_INSCRIPTION_LEN 64
#define H_UI_MAX_MENUS 32

class ui_Painter;
class ui_Base;
struct ui_Vertex;
class ui_MenuBase;

typedef std::function<void()> ui_Callback;

class ui_CursorHandler
{
public:
	static void CursorPress( int x, int y, bool pressed );
	static void UpdateCursorPos( int x, int y );

private:
	friend class ui_Base;

	static void AddUIElement( ui_Base* element );
	static void RemoveUIElement( ui_Base* element );

	static int ui_elements_count_;
	static ui_Base* ui_elements_[ H_UI_MAX_ELEMENTS ];
	static int ui_menu_count_;
	static ui_MenuBase* ui_menus_[ H_UI_MAX_MENUS ];
};

class ui_Base
{
public:
	ui_Base();
	ui_Base( int x, int y, int in_size_x, int in_size_y );
	ui_Base( int x, int y, int in_size_x, int in_size_y,
			 const unsigned char* normal_color, const unsigned char* in_cursor_over_color );
	virtual ~ui_Base();

	int X()const
	{
		return pos_x_;
	};
	int Y()const
	{
		return pos_y_;
	};
	int SizeX()const
	{
		return size_x_;
	};
	int SizeY()const
	{
		return size_y_;
	};
	int SetPos( int x, int y )
	{
		pos_x_= x;
		pos_y_= y;
	};

	//cursor reaction methods. Coirdinates - absolute

	//set state of ui element
	virtual void CursorOver( bool is_over );
	virtual void CursorPress( int x, int y, bool pressed/*true - presed, false - unpressed*/ );

	virtual void Draw( ui_Painter* painter )const;

	void SetActive( bool active )
	{
		is_active_= active;
	}
	bool IsActive()const
	{
		return is_active_;
	}
	void SetVisible( bool visible )
	{
		is_visible_= visible;
	}
	bool IsVisible() const
	{
		return is_visible_;
	}

	static void SetCellSize( int size )
	{
		ui_cell_size_= size;
	};
	static int CellSize()
	{
		return ui_cell_size_;
	};
	static int CellOffset()
	{
		return ui_cell_size_/8;
	};

protected:
	//draw helpers. returns number of vertices
	int GenElementFraming( ui_Vertex* vertices, int trim_size ) const;
	int GenRectangle( ui_Vertex* vertices, int x, int y, int sx, int sy ) const;

protected:
	int pos_x_, pos_y_, size_x_, size_y_;

	unsigned char color_[4];
	unsigned char cursor_over_color_[4];
	unsigned char current_color_[4];

	bool is_active_, is_visible_;

	static int ui_cell_size_;
};

class ui_Button : public ui_Base
{
public:
	ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y );
	ui_Button(const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y,
			const unsigned char* normal_color, const unsigned char* in_cursor_over_color );
	virtual ~ui_Button() override;

	void SetCallback( const ui_Callback& callback )
	{
		callback_= callback;
	}
	virtual void CursorPress( int x, int y, bool pressed) override;
	virtual void Draw( ui_Painter* painter )const override;

private:
	ui_Callback callback_;
	char button_text_[ H_UI_MAX_INSCRIPTION_LEN ];
};

class ui_Checkbox : public ui_Base
{
public:
	ui_Checkbox( int cell_x, int cell_y, bool state= false );
	ui_Checkbox( int cell_x, int cell_y, bool state, const unsigned char* normal_color, const unsigned char* over_cursor_color );
	virtual ~ui_Checkbox() override;

	void SetCallback( const ui_Callback& callback )
	{
		callback_= callback;
	}
	bool GetState() const
	{
		return flag_;
	};
	void SetState( bool state )
	{
		flag_= state;
	}
	virtual void Draw( ui_Painter* painter )const override;
	virtual void CursorPress( int x, int y, bool pressed ) override;

private:
	bool flag_;

	ui_Callback callback_;
};

class ui_Text : public ui_Base
{
public:
	enum Alignment
	{
		Left,
		Center,
		Right
	};

	ui_Text( const char* text, Alignment alignent_, int cell_x, int cell_y, int cell_width, int cell_height );
	ui_Text( const char* text, Alignment alignent_, int cell_x, int cell_y, int cell_width, int cell_height, const unsigned char* color );
	virtual ~ui_Text() override;

	void SetText( const char* text );
	virtual void Draw( ui_Painter* painter )const override;

private:
	char text_[ H_UI_MAX_INSCRIPTION_LEN ];
	Alignment alignment_;
};

class ui_ProgressBar : public ui_Base
{
public:
	ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress= 0.0f );
	ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress, const unsigned char* normal_color, const unsigned char* cursor_over_color_ );
	virtual ~ui_ProgressBar() override;

	void SetProgress( float p );
	virtual void Draw( ui_Painter* painter )const override;

private:
	float progress_;//in range 0.0f-1.0f
};

class ui_TextBox : public ui_Base
{
};

class ui_ListBox : public ui_Base
{
};

/*
 Horizontal slider. Length int cells - 2 or more. Height in cells - 1
				 _
   /|           | |              |\
  / |  +________+_+___________+  | \
 /  |  |        | |           |  |  \
 \  |  +________+_+___________+  |  /
  \ |           | |              | /
   \|           |_|              |/
*/
class ui_Slider : public ui_Base
{
public:
	ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos_= 0.0f );
	ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos_, const unsigned char* normal_color, const unsigned char* cursor_over_color_ );
	virtual ~ui_Slider() override;

	void SetCallback( const ui_Callback callback )
	{
		callback_= callback;
	}

	void SetSliderPos( float pos );
	float SliderPos() const
	{
		return slider_pos_;
	};

	virtual void Draw( ui_Painter* painter )const override;
	virtual void CursorPress( int x, int y, bool pressed ) override;

	//step must be positive
	void SetInvStep( int inv_step )
	{
		slider_inv_step_= inv_step;
	};

private:
	inline static int ArrowSize()
	{
		return ui_Base::CellSize()/2 - ui_Base::CellOffset();
	};
	inline static int SliderHalfSize()
	{
		return ui_Base::CellOffset();
	};
	inline static int ArrowOffset()
	{
		return ui_Base::CellOffset()*3/2;
	};

	float slider_pos_;// in range 0.0f-1.0f;
	int slider_inv_step_;
	ui_Callback callback_;
};

class ui_MenuBase
{
public:
	ui_MenuBase( ui_MenuBase* parent, int x, int y, int sx , int sy );
	virtual ~ui_MenuBase() {};
	void Draw( ui_Painter* painter );
	void SetActive( bool active );
	void SetVisible( bool visible );

	void Kill() {
		marked_for_killing_= true;
	};
	bool IsMarkedForKilling() const {
		return marked_for_killing_;
	};

	virtual void Tick()= 0;

protected:
	std::vector<ui_Base*> elements_;//container for all elements. All elements of menu must be here
	ui_MenuBase * const parent_menu_;
	ui_MenuBase* child_menu_;

	int pos_x_, pos_y_, size_x_, size_y_;//size of menu viewport

	bool marked_for_killing_;// true, if menu need kill in
};

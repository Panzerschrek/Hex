#ifndef UI_BASE_CLASSES_HPP
#define UI_BASE_CLASSES_HPP

#include <vector>

#define H_UI_MAX_ELEMENTS 128
#define H_UI_MAX_INSCRIPTION_LEN 64
#define H_UI_MAX_MENUS 32

class ui_Painter;
class ui_Base;
struct ui_Vertex;
class ui_MenuBase;

class ui_CursorHandler
{
public:
    static void CursorPress( int x, int y, bool pressed );
    static void UpdateCursorPos( int x, int y );


private:
    friend class ui_Base;

    static void AddUIElement( ui_Base* element );
    static void RemoveUIElement( ui_Base* element );

    static int ui_elements_count;
    static ui_Base* ui_elements[ H_UI_MAX_ELEMENTS ];
    static int ui_menu_count;
    static ui_MenuBase* ui_menus[ H_UI_MAX_MENUS ];
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
        return pos_x;
    };
    int Y()const
    {
        return pos_y;
    };
    int SizeX()const
    {
        return size_x;
    };
    int SizeY()const
    {
        return size_y;
    };
    int SetPos( int x, int y )
    {
        pos_x= x;
        pos_y= y;
    };

    //cursor reaction methods. Coirdinates - absolute

    //set state of ui element
    virtual void CursorOver( bool is_over );
    virtual void CursorPress( int x, int y, bool pressed/*true - presed, false - unpressed*/ );

    virtual void Draw( ui_Painter* painter )const;

    void SetActive( bool active )
    {
        is_active= active;
    }
    bool IsActive()const
    {
        return is_active;
    }
    void SetVisible( bool visible )
    {
        is_visible= visible;
    }
    bool IsVisible() const
    {
        return is_visible;
    }

    static void SetCellSize( int size )
    {
        ui_cell_size= size;
    };
    static int CellSize()
    {
        return ui_cell_size;
    };
    static int CellOffset()
    {
        return ui_cell_size/8;
    };

protected:
    //draw helpers. returns number of vertices
    int GenElementFraming( ui_Vertex* vertices, int trim_size ) const;
    int GenRectangle( ui_Vertex* vertices, int x, int y, int sx, int sy ) const;

protected:

    int pos_x, pos_y, size_x, size_y;

    unsigned char color[4];
    unsigned char cursor_over_color[4];
    unsigned char current_color[4];

    bool is_active, is_visible;


    static int ui_cell_size;
};



class ui_Button : public ui_Base
{
public:

    class ui_ButtonCallback
    {
    public:
        virtual void ButtonCallback( ui_Button* button )= 0;
    };

    ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y );
    ui_Button( const char* text, int cell_x, int cell_y, int cell_size_x, int cell_size_y,
               const unsigned char* normal_color, const unsigned char* in_cursor_over_color );
    ~ui_Button();

    void SetCallback( ui_ButtonCallback* call )
    {
        callback= call;
    }
    void CursorPress( int x, int y, bool pressed) override;
    void Draw( ui_Painter* painter )const override;



private:
    ui_ButtonCallback* callback;
    char button_text[ H_UI_MAX_INSCRIPTION_LEN ];
};

class ui_Checkbox : public ui_Base
{
public:

    class ui_CheckboxCallback
    {
    public:
        virtual void CheckboxCallback( ui_Checkbox* checkbox )= 0;

    };
    ui_Checkbox( int cell_x, int cell_y, bool state= false );
    ui_Checkbox( int cell_x, int cell_y, bool state, const unsigned char* normal_color, const unsigned char* over_cursor_color );
    ~ui_Checkbox();

    void SetCallback( ui_CheckboxCallback* call )
    {
        callback= call;
    }
    bool GetState() const
    {
        return flag;
    };
    void SetState( bool state )
    {
        flag= state;
    }
    void Draw( ui_Painter* painter )const override;
    void CursorPress( int x, int y, bool pressed ) override;

private:
    bool flag;

    ui_CheckboxCallback* callback;
};

class ui_Text : public ui_Base
{
public:

    enum ui_TextAlignment
    {
        ALIGNMENT_LEFT,
        ALIGNMENT_CENTER,
        ALIGNMENT_RIGHT
    };

    ui_Text( const char* text, ui_TextAlignment alignent_, int cell_x, int cell_y );
    ui_Text( const char* text, ui_TextAlignment alignent_, int cell_x, int cell_y, const unsigned char* color );
    ~ui_Text();

    void SetText( const char* text );
    void Draw( ui_Painter* painter )const override;

private:
    char text[ H_UI_MAX_INSCRIPTION_LEN ];
    ui_TextAlignment alignment;
};

class ui_ProgressBar : public ui_Base
{
public:
    ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress= 0.0f );
    ui_ProgressBar( int cell_x, int cell_y, int cell_size_x, int cell_size_y, float progress, const unsigned char* normal_color, const unsigned char* cursor_over_color_ );
    ~ui_ProgressBar();

    void SetProgress( float p );
    void Draw( ui_Painter* painter )const override;

private:
    float progress;//in range 0.0f-1.0f
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

    class ui_SliderCallback
    {
    public:
        virtual void SliderCallback( ui_Slider* slider )= 0;

    };

    ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos_= 0.0f );
    ui_Slider( int cell_x, int cell_y, int cell_size_x, float slider_pos_, const unsigned char* normal_color, const unsigned char* cursor_over_color_ );
    ~ui_Slider();

    void SetCallback( ui_SliderCallback* call )
    {
        callback= call;
    }

    void SetSliderPos( float pos );
    float SliderPos() const
    {
        return slider_pos;
    };

    void Draw( ui_Painter* painter )const override;
    void CursorPress( int x, int y, bool pressed ) override;

    //step must be positive
    void SetInvStep( int inv_step )
    {
        slider_inv_step= inv_step;
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

    float slider_pos;// in range 0.0f-1.0f;
    int slider_inv_step;
    ui_SliderCallback* callback;
};


class ui_MenuBase
{
public:

    ui_MenuBase( ui_MenuBase* parent, int x, int y, int sx , int sy );
    virtual ~ui_MenuBase(){};
    void Draw( ui_Painter* painter );
    void SetActive( bool active );
	void SetVisible( bool visible );

	void Kill(){ marked_for_killing= true; };
	bool IsMarkedForKilling() const { return marked_for_killing; };

	virtual void Tick()= 0;

protected:

    std::vector<ui_Base*> elements;//container for all elements. All elements of menu must be here
    ui_MenuBase * const parent_menu;
    ui_MenuBase* child_menu;

    int pos_x, pos_y, size_x, size_y;//size of menu viewport

    bool marked_for_killing;// true, if menu need kill in

};

#endif//UI_BASE_CLASSES_HPP

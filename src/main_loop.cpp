#include "main_loop.hpp"
#include "renderer/world_renderer.hpp"
#include "world.hpp"
#include "player.hpp"

#include "settings.hpp"
#include "settings_keys.hpp"

#include "ui/ui_base_classes.hpp"
#include "ui/ui_painter.hpp"
#include "ogl_state_manager.hpp"

#include "ui/main_menu.hpp"
#include "ui/ingame_menu.hpp"


static ui_MouseButton MouseKey( const QMouseEvent* e )
{
	switch( e->button() )
	{
	case Qt::LeftButton:
	default:
		return ui_MouseButton::Left;

	case Qt::RightButton:
		return ui_MouseButton::Right;

	case Qt::MiddleButton:
		return ui_MouseButton::Middle;
	};
}

void h_MainLoop::Start()
{
	QGLFormat format;

	h_SettingsPtr settings= std::make_shared<h_Settings>( "config.json" );

	int antialiasing= std::min( std::max( settings->GetInt( h_SettingsKeys::antialiasing, 4 ), 0 ), 16 );
	if( antialiasing )
		format.setSamples( antialiasing );

	format.setVersion( 3, 3 );
	format.setProfile( QGLFormat::CoreProfile );

	bool vsync= settings->GetBool( h_SettingsKeys::vsync, true );
	format.setSwapInterval( vsync ? 1 : 0 );

	new h_MainLoop( settings, format );
}

QSize h_MainLoop::minimumSizeHint() const
{
	return QSize( screen_width_, screen_height_ );
}

QSize h_MainLoop::sizeHint() const
{
	return QSize( screen_width_, screen_height_ );
}

h_MainLoop::h_MainLoop(
	const h_SettingsPtr& settings,
	const QGLFormat& format )
	: QGLWidget( format, nullptr )
	, settings_(settings)
	, in_focus_(false)
	, game_started_(false)
{
	window_= new QMainWindow( nullptr );

	window_->move( 0, 0 );
	window_->setWindowTitle( "Hex" );
	window_->setWindowIcon( QIcon( "src/hex-logo.ico" ) );

	window_->setCentralWidget( this );
	window_->setContentsMargins( 0, 0, 0, 0 );

	if (!settings_->IsValue(h_SettingsKeys::screen_width ) ) settings_->SetSetting( h_SettingsKeys::screen_width , 640 );
	if (!settings_->IsValue(h_SettingsKeys::screen_height) ) settings_->SetSetting( h_SettingsKeys::screen_height, 480 );

	screen_width_=  std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_width ), H_MIN_SCREEN_WIDTH ), H_MAX_SCREEN_WIDTH  );
	screen_height_= std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_height), H_MIN_SCREEN_HEIGHT), H_MAX_SCREEN_HEIGHT );

	QWidget::setFixedSize( screen_width_, screen_height_ );
	window_->setFocusPolicy( Qt::ClickFocus );
	QWidget::setFocusPolicy( Qt::ClickFocus );
	QWidget::setFocus();

	window_->show();
	window_->setFixedSize( window_->size() );

	QWidget::setAutoFillBackground( false );

	//window->setWindowState( Qt::WindowFullScreen );
}

h_MainLoop::~h_MainLoop()
{
}

void h_MainLoop::initializeGL()
{
	GetGLFunctions(
		[](const char* name)
		{
			return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
		});

	r_OGLState state;
	state.InitialState();
	r_OGLStateManager::SetState( state );

	r_Framebuffer::SetScreenFramebufferSize( screen_width_, screen_height_ );
}

void h_MainLoop::resizeGL(int w , int h)
{
	glViewport(0,0,w,h);
}

//Main rendering loop here
void h_MainLoop::paintGL()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if( game_started_ )
	{
		player_->Lock();
		UpdateCursor();
		player_->Tick();
		player_->Unlock();
		world_renderer_->Draw();

	}
	else
		UpdateCursor();

	if( !ui_painter_ )
		ui_painter_.reset( new ui_Painter() );
	if( !root_menu_ )
		root_menu_.reset( new ui_MainMenu( this, screen_width_, screen_height_ ) );

	if( root_menu_ ) root_menu_->Tick();

	m_Mat4 mat;
	mat.Identity();

	mat[0]= 2.0f / float( screen_width_ );
	mat[12]= -1.0f;
	mat[5]= -2.0f / float( screen_height_ );
	mat[13]= 1.0f;
	ui_painter_->SetMatrix( mat );

	static const GLenum blend_func[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, false, false,
		blend_func );
	r_OGLStateManager::UpdateState( state );

	if( root_menu_ ) root_menu_->Draw( ui_painter_.get() );

	glFlush();
	update();
}

void h_MainLoop::UpdateCursor()
{
	QPoint cur_local_pos= this->mapFromGlobal( cursor_.pos() );
	ui_CursorHandler::UpdateCursorPos( cur_local_pos.x(), cur_local_pos.y() );

	if( !game_started_ )
		return;

	if( in_focus_ && ui_CursorHandler::IsMouseGrabbed() )
	{
		if( cursor_.shape() != Qt::BlankCursor )
		{
			cursor_.setShape( Qt::BlankCursor );
			this->setCursor( cursor_ );
		}

		QPoint cursor_lock_pos( screen_width_ >> 1, screen_height_ >> 1 );
		ui_CursorHandler::ControllerMove( cursor_lock_pos.x() - cur_local_pos.x(), cursor_lock_pos.y() - cur_local_pos.y() );
		cursor_.setPos( this->mapToGlobal(cursor_lock_pos) );
	}
	else
	{
		if( cursor_.shape() != Qt::ArrowCursor )
		{
			cursor_.setShape( Qt::ArrowCursor );
			this->setCursor( cursor_ );
		}
	}
}

void h_MainLoop::ProcessMenuKey( QKeyEvent* e, bool pressed )
{
	if( !root_menu_ ) return;

	int key= e->key();
	ui_Key ui_key;

	if( key >= Qt::Key_A && key <= Qt::Key_Z )
		ui_key= ui_Key( int(ui_Key::A) + (key - Qt::Key_A) );
	else if( key >= Qt::Key_0 && key <= Qt::Key_9 )
		ui_key= ui_Key( int(ui_Key::Zero) + (key - Qt::Key_0) );
	else
	{
		switch( key )
		{
		case Qt::Key_Escape: ui_key= ui_Key::Escape; break;
		case Qt::Key_Tab: ui_key= ui_Key::Tab; break;
		case Qt::Key_Backspace : ui_key= ui_Key::Back; break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
			ui_key= ui_Key::Enter; break;

		case Qt::Key_Up   : ui_key= ui_Key::Up   ; break;
		case Qt::Key_Down : ui_key= ui_Key::Down ; break;
		case Qt::Key_Left : ui_key= ui_Key::Left ; break;
		case Qt::Key_Right: ui_key= ui_Key::Right; break;

		case Qt::Key_Shift: ui_key= ui_Key::Shift; break;
		case Qt::Key_Control: ui_key= ui_Key::Control; break;
		case Qt::Key_Alt: ui_key= ui_Key::Alt; break;

		case Qt::Key_Space: ui_key= ui_Key::Space; break;

		case Qt::Key_Minus: ui_key= ui_Key::Minus; break;
		case Qt::Key_Equal: ui_key= ui_Key::Equal; break;
		case Qt::Key_Backslash: ui_key= ui_Key::Backslash; break;

		case Qt::Key_BracketLeft : ui_key= ui_Key::BracketLeft ; break;
		case Qt::Key_BracketRight: ui_key= ui_Key::BracketRight; break;

		case Qt::Key_Semicolon: ui_key= ui_Key::Semicolon; break;
		case Qt::Key_Apostrophe: ui_key= ui_Key::Apostrophe; break;

		case Qt::Key_Comma: ui_key= ui_Key::Comma; break;
		case Qt::Key_Period: ui_key= ui_Key::Dot; break;
		case Qt::Key_Slash: ui_key= ui_Key::Slash; break;

		default: ui_key= ui_Key::Unknown; break;
		};
	}

	if( pressed ) root_menu_->KeyPress( ui_key );
	else root_menu_->KeyRelease( ui_key );
}

void h_MainLoop::mouseReleaseEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), MouseKey(e), true );
}

void h_MainLoop::mousePressEvent( QMouseEvent* e )
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), MouseKey(e), false );
}

void h_MainLoop::mouseMoveEvent(QMouseEvent*){}

void h_MainLoop::keyPressEvent(QKeyEvent* e)
{
	ProcessMenuKey( e, true );
}

void h_MainLoop::keyReleaseEvent(QKeyEvent* e)
{
	ProcessMenuKey( e, false );
}

void h_MainLoop::focusInEvent( QFocusEvent * )
{
	in_focus_= true;
}

void h_MainLoop::focusOutEvent( QFocusEvent * )
{
	in_focus_= false;
}

void h_MainLoop::closeEvent(QCloseEvent* )
{
	Quit();
}

void h_MainLoop::initializeOverlayGL() {}

void h_MainLoop::resizeOverlayGL(int, int){}

void h_MainLoop::paintOverlayGL(){}

void h_MainLoop::Quit()
{
	game_started_= false;
	world_renderer_.reset();
	player_.reset();
	world_.reset();

	window_->close();
}

void h_MainLoop::StartGame()
{
	if( !game_started_ )
	{
		world_= std::make_shared<h_World>( settings_ );
		player_= std::make_shared<h_Player>( world_ );
		world_renderer_= std::make_shared<r_WorldRenderer>( settings_, world_, player_ );

		world_->SetPlayer( player_ );
		world_->SetRenderer( world_renderer_ );

		world_renderer_->SetViewportSize( screen_width_, screen_height_ );
		world_renderer_->InitGL();

		world_->StartUpdates();

		game_started_= true;

		root_menu_.reset(
			new ui_IngameMenu(
				screen_width_, screen_height_,
				player_ ));
	}
}

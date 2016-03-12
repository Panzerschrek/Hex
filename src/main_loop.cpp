#include <cstring>

#include <QApplication>
#include <QDesktopWidget>

#include "main_loop.hpp"
#include "renderer/world_renderer.hpp"
#include "world.hpp"
#include "world_header.hpp"
#include "player.hpp"

#include "settings.hpp"
#include "settings_keys.hpp"
#include "time.hpp"
#include "console.hpp"

#include "ui/ingame_menu.hpp"
#include "ui/loading_menu.hpp"
#include "ui/main_menu.hpp"
#include "ui/ui_painter.hpp"

#include "ogl_state_manager.hpp"


static const constexpr int g_min_screen_width = 640;
static const constexpr int g_min_screen_height= 480;
static const constexpr int g_max_screen_width = 4096;
static const constexpr int g_max_screen_height= 4096;

static const char g_world_directory[]= "world";

static int GetSamples( const h_Settings& settings )
{
	const char* antialiasing= settings.GetString( h_SettingsKeys::antialiasing );

	if( std::strcmp( antialiasing, "msaa 2x" ) == 0 )
		return 2;
	else if( std::strcmp( antialiasing, "msaa 4x" ) == 0 )
		return 4;
	else if( std::strcmp( antialiasing, "msaa 8x" ) == 0 )
		return 8;
	else if( std::strcmp( antialiasing, "msaa 16x" ) == 0 )
		return 16;

	return 0;
}

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

	int samples= GetSamples( *settings );
	if( samples )
		format.setSamples( samples );

	format.setVersion( 3, 3 );
	format.setProfile( QGLFormat::CoreProfile );

	bool vsync= settings->GetBool( h_SettingsKeys::vsync, true );
	settings->SetSetting( h_SettingsKeys::vsync, vsync );

	format.setSwapInterval( vsync ? 1 : 0 );

	new h_MainLoop( settings, format );
}

h_MainLoop::h_MainLoop(
	const h_SettingsPtr& settings,
	const QGLFormat& format )
	: QGLWidget( format )
	, settings_(settings)
	, game_started_(false)
{
	QDesktopWidget* desktop= QApplication::desktop();

	bool fullscreen= settings_->GetBool( h_SettingsKeys::fullscreen );
	int screen_number= settings_->GetInt( h_SettingsKeys::screen_number, desktop->primaryScreen() );

	screen_width_=  std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_width  ), g_min_screen_width ), g_max_screen_width  );
	screen_height_= std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_height ), g_min_screen_height), g_max_screen_height );

	settings_->SetSetting( h_SettingsKeys::screen_width , screen_width_  );
	settings_->SetSetting( h_SettingsKeys::screen_height, screen_height_ );

	settings_->SetSetting( h_SettingsKeys::fullscreen, fullscreen );
	settings_->SetSetting( h_SettingsKeys::screen_number, screen_number );

	QRect screen_geometry= desktop->screenGeometry( screen_number );

	if( fullscreen )
	{
		screen_width_ = screen_geometry.width ();
		screen_height_= screen_geometry.height();
	}

	setFixedSize( screen_width_, screen_height_ );

	if( fullscreen )
		move( screen_geometry.topLeft() );
	else
		move( 0, 0 );

	setAttribute( Qt::WA_DeleteOnClose, true );
	setAttribute( Qt::WA_QuitOnClose, true );

	setWindowTitle( "Hex" );
	setWindowIcon( QIcon( "src/hex-logo.ico" ) );

	setFocusPolicy( Qt::ClickFocus );
	setAutoFillBackground( false );

	if( fullscreen )
		showFullScreen();

	setFocus();
	show();

	QuitToMainMenu();

	h_Console::Info( "MainLoop started" );
}

h_MainLoop::~h_MainLoop()
{
	h_Console::Info( "MainLoop destoryed" );
}

void h_MainLoop::initializeGL()
{
	h_Console::Info( "Initialize OpenGL" );

	GetGLFunctions(
		[](const char* name)
		{
			return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
		},
		[](const char* name)
		{
			h_Console::Warning( "Function ", name, " not found" );
		});

	r_GLSLProgram::SetProgramBuildLogOutCallback(
		[]( const char* out )
		{
			h_Console::Warning( out );
		} );

	r_Framebuffer::SetScreenFramebufferSize( screen_width_, screen_height_ );

	ui_painter_.reset( new ui_Painter() );
}

void h_MainLoop::resizeGL(int w , int h)
{
	glViewport(0,0,w,h);
}

//Main rendering loop here
void h_MainLoop::paintGL()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	UpdateCursor();

	root_menu_->Tick();

	if( game_started_ )
		world_renderer_->Draw();

	m_Mat4 mat;
	mat.Identity();

	mat[ 0]=  2.0f / float( screen_width_ );
	mat[12]= -1.0f;
	mat[ 5]= -2.0f / float( screen_height_ );
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

	if( hasFocus() && ui_CursorHandler::IsMouseGrabbed() )
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
		case Qt::Key_QuoteLeft: ui_key= ui_Key::GraveAccent; break;

		case Qt::Key_Comma: ui_key= ui_Key::Comma; break;
		case Qt::Key_Period: ui_key= ui_Key::Dot; break;
		case Qt::Key_Slash: ui_key= ui_Key::Slash; break;

		default: ui_key= ui_Key::Unknown; break;
		};
	}

	// HACK. Remove from here.
	if( pressed && ui_key == ui_Key::GraveAccent )
	{
		h_Console::Toggle();
		return;
	}

	if( pressed ) root_menu_->KeyPress( ui_key );
	else root_menu_->KeyRelease( ui_key );
}

void h_MainLoop::mouseReleaseEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), MouseKey(e), false );
}

void h_MainLoop::mousePressEvent( QMouseEvent* e )
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), MouseKey(e), true );
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

void h_MainLoop::closeEvent( QCloseEvent* e )
{
	e->accept();

	QuitToMainMenu();
}

void h_MainLoop::initializeOverlayGL() {}

void h_MainLoop::resizeOverlayGL(int, int){}

void h_MainLoop::paintOverlayGL(){}

void h_MainLoop::Quit()
{
	close();
}

void h_MainLoop::StartGame()
{
	if( !game_started_ )
	{
		// Create "loading menu"
		ui_LoadingMenu* loading_menu= new ui_LoadingMenu( screen_width_, screen_height_ );
		root_menu_.reset( loading_menu );

		// Setup callback for loading.
		uint64_t prev_progress_time_ms= 0;
		const h_LongLoadingCallback long_loading_callback=
		[this, loading_menu, &prev_progress_time_ms]( float progress )
		{
			uint64_t time_ms= hGetTimeMS();
			// Draw frames with low frequency, bacause each frame makes loading slower.
			if( time_ms - prev_progress_time_ms >= 125 )
			{
				prev_progress_time_ms= time_ms;
				loading_menu->SetProgress( progress );

				// Schedule repaint.
				update();
				// Process events, include "paint" event.
				QCoreApplication::processEvents();
			}
		};
		const float c_world_loading_relative_progress= 0.75f;

		// Create world, player, renderer.
		world_header_= std::make_shared<h_WorldHeader>();
		world_header_->Load( g_world_directory );

		world_= std::make_shared<h_World>(
			[ &long_loading_callback, c_world_loading_relative_progress ]( float progress )
			{
				long_loading_callback( progress * c_world_loading_relative_progress );
			},
			settings_,
			world_header_,
			g_world_directory );

		player_= std::make_shared<h_Player>( world_, world_header_ );

		world_renderer_= std::make_shared<r_WorldRenderer>( settings_, world_, player_ );
		world_renderer_->SetViewportSize( screen_width_, screen_height_ );
		world_renderer_->InitGL(
			[ &long_loading_callback, c_world_loading_relative_progress ]( float progress )
			{
				long_loading_callback(
					progress * ( 1.0f - c_world_loading_relative_progress )+
					c_world_loading_relative_progress );
			} );

		// Start world updates after world renderer creation.
		world_->StartUpdates( player_.get(), world_renderer_.get() );

		// Set ingame menu.
		root_menu_.reset(
			new ui_IngameMenu(
				screen_width_, screen_height_,
				player_,
				*this ));

		// Finally, say "we in game".
		game_started_= true;
	}
}

void h_MainLoop::QuitToMainMenu()
{
	if( game_started_ )
	{
		world_->StopUpdates();

		game_started_= false;
		root_menu_.reset();
		world_renderer_.reset();
		player_.reset();
		world_.reset();

		world_header_->Save( g_world_directory );
		world_header_.reset();

		root_menu_.reset();
	}

	if( !root_menu_ )
		root_menu_.reset( new ui_MainMenu( this, settings_, screen_width_, screen_height_ ) );
}

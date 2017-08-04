#include <cstring>

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
#include "shaders_loading.hpp"


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

static ui_Key TranslateKey( const SDL_Scancode scan_code )
{
	if( scan_code >= SDL_SCANCODE_A && scan_code <= SDL_SCANCODE_Z )
		return ui_Key( int(ui_Key::A) + (scan_code - SDL_SCANCODE_A) );
	else if( scan_code == SDL_SCANCODE_0 )
		return ui_Key::Zero;
	if( scan_code >= SDL_SCANCODE_1 && scan_code <= SDL_SCANCODE_9 )
		return ui_Key( int(ui_Key::One) + (scan_code - SDL_SCANCODE_1) );

	switch( scan_code )
	{
	case SDL_SCANCODE_ESCAPE: return ui_Key::Escape;
	case SDL_SCANCODE_TAB: return  ui_Key::Tab;
	case SDL_SCANCODE_BACKSPACE: return ui_Key::Back;
	case SDL_SCANCODE_RETURN: return ui_Key::Enter;

	case SDL_SCANCODE_UP   : return ui_Key::Up   ;
	case SDL_SCANCODE_DOWN : return ui_Key::Down ;
	case SDL_SCANCODE_LEFT : return ui_Key::Left ;
	case SDL_SCANCODE_RIGHT: return ui_Key::Right;

	case SDL_SCANCODE_LSHIFT: return ui_Key::Shift;
	case SDL_SCANCODE_RSHIFT: return ui_Key::Shift;
	case SDL_SCANCODE_LCTRL: return ui_Key::Control;
	case SDL_SCANCODE_RCTRL: return ui_Key::Control;
	case SDL_SCANCODE_LALT: return ui_Key::Alt;
	case SDL_SCANCODE_RALT: return ui_Key::Alt;

	case SDL_SCANCODE_SPACE: return ui_Key::Space;

	case SDL_SCANCODE_MINUS: return ui_Key::Minus;
	case SDL_SCANCODE_EQUALS: return ui_Key::Equal;

	case SDL_SCANCODE_BACKSLASH: return ui_Key::Backslash;

	case SDL_SCANCODE_LEFTBRACKET : return ui_Key::BracketLeft ;
	case SDL_SCANCODE_RIGHTBRACKET: return ui_Key::BracketRight;

	case SDL_SCANCODE_SEMICOLON: return ui_Key::Semicolon;
	case SDL_SCANCODE_APOSTROPHE: return ui_Key::Apostrophe;

	case SDL_SCANCODE_COMMA: return ui_Key::Comma;
	case SDL_SCANCODE_PERIOD: return ui_Key::Dot;
	case SDL_SCANCODE_SLASH: return ui_Key::Slash;

	default:
		break;
	};

	return ui_Key::Unknown;
}

static ui_MouseButton TranslateMouseButton( const Uint8 button )
{
	switch(button)
	{
	case SDL_BUTTON_LEFT: return ui_MouseButton::Left;
	case SDL_BUTTON_RIGHT: return ui_MouseButton::Right;
	case SDL_BUTTON_MIDDLE: return ui_MouseButton::Middle;
	};

	return ui_MouseButton::Left;
}

h_MainLoop::h_MainLoop()
	: settings_(std::make_shared<h_Settings>( "config.json" ))
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		h_Console::Error( "Can not initialize sdl video" );

	bool fullscreen= settings_->GetBool( h_SettingsKeys::fullscreen );
	int display_number= settings_->GetInt( h_SettingsKeys::screen_number, 0 );
	display_number= std::min( display_number, std::max( 0, SDL_GetNumVideoDisplays() - 1 ) );

	screen_width_=  std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_width  ), g_min_screen_width ), g_max_screen_width  );
	screen_height_= std::min( std::max( settings_->GetInt( h_SettingsKeys::screen_height ), g_min_screen_height), g_max_screen_height );

	settings_->SetSetting( h_SettingsKeys::screen_width , screen_width_  );
	settings_->SetSetting( h_SettingsKeys::screen_height, screen_height_ );

	settings_->SetSetting( h_SettingsKeys::fullscreen, fullscreen );
	settings_->SetSetting( h_SettingsKeys::screen_number, display_number );

	if( fullscreen )
	{
		SDL_DisplayMode display_mode;
		SDL_GetCurrentDisplayMode( display_number, &display_mode );
		screen_width_ = display_mode.w;
		screen_height_= display_mode.h;
	}

	// OpenGL attributes
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	const int msaa_samples= GetSamples( *settings_ );
	if( msaa_samples > 0 )
	{
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, msaa_samples );
	}

	// Window creation.

	window_=
		SDL_CreateWindow(
			"Hex",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			screen_width_, screen_height_,
			SDL_WINDOW_OPENGL | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN );

	if( window_ == nullptr )
		h_Console::Error( "Can not create window" );

	// GL Context creation

	gl_context_= SDL_GL_CreateContext( window_ );
	if( gl_context_ == nullptr )
		h_Console::Error( "Can not create OpenGL context" );

	h_Console::Info("");
	h_Console::Info( "OpenGL configuration: " );
	h_Console::Info( "Vendor: ", glGetString( GL_VENDOR ) );
	h_Console::Info( "Renderer: ", glGetString( GL_RENDERER ) );
	h_Console::Info( "Version: ", glGetString( GL_VERSION ) );
	h_Console::Info("");

	if( settings_->GetBool( h_SettingsKeys::vsync, true ) )
		SDL_GL_SetSwapInterval(1);
	else
		SDL_GL_SetSwapInterval(0);

	h_Console::Info( "Initialize OpenGL" );

	// OpenGL setup
	GetGLFunctions(
		SDL_GL_GetProcAddress,
		[](const char* name)
		{
			h_Console::Warning( "Function ", name, " not found" );
		});

	r_GLSLProgram::SetProgramBuildLogOutCallback(
		[]( const char* out )
		{
			h_Console::Warning( out );
		} );

	rSetShaderLoadingLogCallback( []( const char* text ) { h_Console::Error( text ); } );
	rSetShadersDir( "shaders" );

	r_Framebuffer::SetScreenFramebufferSize( screen_width_, screen_height_ );

	// Final preparations

	ui_painter_.reset( new ui_Painter() );

	QuitToMainMenu();
}

h_MainLoop::~h_MainLoop()
{
	h_Console::Info( "MainLoop destoryed" );
}

bool h_MainLoop::Loop()
{
	ProcessEvents();
	UpdateCursor();
	Paint();

	return !quit_requested_;
}

void h_MainLoop::Paint()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

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

	SDL_GL_SwapWindow( window_ );
}

void h_MainLoop::UpdateCursor()
{
	SDL_SetRelativeMouseMode( ui_CursorHandler::IsMouseGrabbed() ? SDL_TRUE : SDL_FALSE );
}

void h_MainLoop::ProcessEvents()
{
	SDL_Event event;
	while( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			if( event.window.event == SDL_WINDOWEVENT_CLOSE )
			{
				Quit();
				return;
			}
			break;

		case SDL_QUIT:
			break;

		case SDL_KEYUP:
		case SDL_KEYDOWN:
			if( root_menu_ != nullptr )
			{
				const ui_Key key= TranslateKey( event.key.keysym.scancode );
				if( key != ui_Key::Unknown )
				{
					if( event.type == SDL_KEYDOWN )
						root_menu_->KeyPress( key );
					else
						root_menu_->KeyRelease( key );
				}
			}
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			ui_CursorHandler::CursorPress(
				event.button.x,
				event.button.y,
				TranslateMouseButton(event.button.button),
				event.type == SDL_MOUSEBUTTONUP );
			break;

		case SDL_MOUSEMOTION:
			ui_CursorHandler::UpdateCursorPos( event.motion.x, event.motion.y );
			ui_CursorHandler::ControllerMove( -event.motion.xrel, -event.motion.yrel );
			break;

		default:
			break;
		};
	} // while has events
}


void h_MainLoop::Quit()
{
	QuitToMainMenu();
	quit_requested_= true;
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

				ProcessEvents();
				UpdateCursor();
				Paint();
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

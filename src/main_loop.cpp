#include "main_loop.hpp"
#include "renderer/world_renderer.hpp"
#include "world.hpp"
#include "player.hpp"

#include "settings.hpp"
#include "settings_keys.hpp"

#include "block_collision.hpp"
#include "console.hpp"
#include "ui/ui_base_classes.hpp"
#include "ui/ui_painter.hpp"
#include "ogl_state_manager.hpp"
#include "framebuffer.hpp"

#include "ui/main_menu.hpp"
#include "ui/ingame_menu.hpp"

void h_MainLoop::Start()
{
	QGLFormat format;

	h_SettingsPtr settings= std::make_shared<h_Settings>("config.json");

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
	, startup_time_(0,0,0,0)
	, prev_move_time_(QTime::currentTime())
	, use_mouse_(false)
	, game_started_(false)
	, cam_pos_( 0.0f, 0.0f, 67.0f )
	, cam_ang_( 0.0f, 0.0f, 0.0f )
	, build_block_(BLOCK_UNKNOWN)
	, root_menu_(nullptr)
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

	frame_count_= 0;
}

h_MainLoop::~h_MainLoop()
{
	//delete renderer;
	//delete window;
	//delete world;
//	delete player;
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
		Input();
		world_renderer_->SetCamAng( cam_ang_ );
		world_renderer_->SetCamPos( cam_pos_ );
		GetBuildPos();
		player_->Unlock();
		world_renderer_->Draw();

	}
	else
		Input();

	if( frame_count_ == 0 )
	{
		ui_painter_= new ui_Painter();
		root_menu_= new ui_MainMenu( this, screen_width_, screen_height_ );
	}

	if(root_menu_) root_menu_->Tick();

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

	if(root_menu_) root_menu_->Draw( ui_painter_ );

	glFlush();
	update();

	frame_count_++;
}

void h_MainLoop::GetBuildPos()
{
	build_dir_= player_->GetBuildPos( &build_pos_x_, &build_pos_y_, &build_pos_z_ );

	m_Vec3 discret_build_pos(
		( float( build_pos_x_ + 0.3333333333f ) ) * H_SPACE_SCALE_VECTOR_X,
		float( build_pos_y_ ) - 0.5f * float(build_pos_x_&1) + 0.5f,
		float( build_pos_z_ ) - 1.0f );

	if( build_dir_ == DIRECTION_UNKNOWN )
		discret_build_pos.z= -1.0f;

	world_renderer_->SetBuildPos( discret_build_pos, build_dir_ );
}

void h_MainLoop::Input()
{
	QPoint cur_local_pos= this->mapFromGlobal( cursor_.pos() );
	ui_CursorHandler::UpdateCursorPos( cur_local_pos.x(), cur_local_pos.y() );

	if( !game_started_ )
		return;

	if( use_mouse_ )
	{
		m_Vec3 ang_delta;
		ang_delta.z= float( screen_width_ /2 - cur_local_pos.x() ) * 0.005f;
		ang_delta.x= float( screen_height_/2 - cur_local_pos.y() ) * 0.005f;
		ang_delta.y= 0.0f;
		player_->Rotate( ang_delta );
		cam_ang_= player_->Angle();

		cur_local_pos= this->mapFromGlobal( QPoint( 0, 0 ) );
		cursor_.setPos( screen_width_/2 - cur_local_pos.x(), screen_height_/2 - cur_local_pos.y() );
	}

	QTime current_time= QTime::currentTime();
	float dt = float( prev_move_time_.msecsTo(current_time) ) / 1000.0f;
	prev_move_time_= current_time;

	const float speed= 5.0f;

	m_Vec3 move_vec( 0.0f, 0.0f, 0.0f );
	if( keys_[ Qt::Key_W ] )
	{
		move_vec.y+= dt * speed * m_Math::Cos( cam_ang_.z );
		move_vec.x-= dt * speed * m_Math::Sin( cam_ang_.z );
	}
	if( keys_[ Qt::Key_S ] )
	{
		move_vec.y-= dt * speed * m_Math::Cos( cam_ang_.z );
		move_vec.x+= dt * speed * m_Math::Sin( cam_ang_.z );
	}
	if( keys_[ Qt::Key_A ] )
	{
		move_vec.y-= dt * speed * m_Math::Cos( cam_ang_.z - m_Math::FM_PI2);
		move_vec.x+= dt * speed * m_Math::Sin( cam_ang_.z - m_Math::FM_PI2);
	}
	if( keys_[ Qt::Key_D ] )
	{
		move_vec.y-= dt * speed * m_Math::Cos( cam_ang_.z + m_Math::FM_PI2);
		move_vec.x+= dt * speed * m_Math::Sin( cam_ang_.z + m_Math::FM_PI2);
	}

	if( keys_[ Qt::Key_Space ] )
		move_vec.z+= dt * speed;
	if( keys_[ Qt::Key_C ] )
		move_vec.z-= dt * speed;

	player_->Move( move_vec );
	cam_pos_= player_->Pos();
	cam_pos_.z+= H_PLAYER_EYE_LEVEL;

}

void h_MainLoop::ProcessMenuKeyPress( QKeyEvent* e )
{
	if( !root_menu_) return;

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

	root_menu_->KeyPress( ui_key );
}

void h_MainLoop::mouseReleaseEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), true );
}
void h_MainLoop::mousePressEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), false );

	if( !game_started_ )
		return;

	if( e->button() == Qt::RightButton && build_block_ != BLOCK_UNKNOWN )
		world_->AddBuildEvent(
			build_pos_x_ - world_->Longitude() * H_CHUNK_WIDTH,
			build_pos_y_ - world_->Latitude () * H_CHUNK_WIDTH,
			build_pos_z_, build_block_ );
	else if( e->button() == Qt::LeftButton )
	{
		short new_build_pos[]= { build_pos_x_, build_pos_y_, build_pos_z_ };
		switch ( build_dir_ )
		{
		case UP:
			new_build_pos[2]--;
			break;
		case DOWN:
			new_build_pos[2]++;
			break;

		case FORWARD:
			new_build_pos[1]--;
			break;
		case BACK:
			new_build_pos[1]++;
			break;

		case FORWARD_RIGHT:
			new_build_pos[1]-= (new_build_pos[0]&1);
			new_build_pos[0]--;
			break;
		case BACK_RIGHT:
			new_build_pos[1]+= ((new_build_pos[0]+1)&1);
			new_build_pos[0]--;
			break;

		case FORWARD_LEFT:
			new_build_pos[1]-= (new_build_pos[0]&1);
			new_build_pos[0]++;
			break;
		case BACK_LEFT:
			new_build_pos[1]+= ((new_build_pos[0]+1)&1);
			new_build_pos[0]++;
			break;

		default:
			new_build_pos[2]= 1024;// make build position not in bounds
		};
		if( keys_[ Qt::Key_X ] )
			world_->Blast(
				new_build_pos[0] - world_->Longitude() * H_CHUNK_WIDTH,
				new_build_pos[1] - world_->Latitude() * H_CHUNK_WIDTH,
				new_build_pos[2], 4 );
		else
			world_->AddDestroyEvent(
				new_build_pos[0] - world_->Longitude() * H_CHUNK_WIDTH,
				new_build_pos[1] - world_->Latitude() * H_CHUNK_WIDTH,
				new_build_pos[2] );
	}
}

void h_MainLoop::mouseMoveEvent(QMouseEvent*){}

void h_MainLoop::keyPressEvent(QKeyEvent* e)
{
	keys_[ e->key() ]= true;

	switch( e->key() )
	{
	case Qt::Key_M:
		use_mouse_= !use_mouse_;
		break;

	case Qt::Key_B:
		printf( "build: %d %d %d\n", build_pos_x_, build_pos_y_, build_pos_z_ );
		break;

	case Qt::Key_QuoteLeft:
		h_Console::Toggle();
		break;

	case Qt::Key_Q:
		if( game_started_ ) world_->Save();
		break;

	default:
		ProcessMenuKeyPress( e );
		break;
	};
}

void h_MainLoop::keyReleaseEvent(QKeyEvent* e)
{
	keys_[ e->key() ]= false;
}

void h_MainLoop::focusInEvent(QFocusEvent *) {}

void h_MainLoop::focusOutEvent( QFocusEvent *)
{
	use_mouse_= false;
}

void h_MainLoop::closeEvent(QCloseEvent* ) {}

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
		world_renderer_= std::make_shared<r_WorldRenderer>( settings_, world_ );
		player_= std::make_shared<h_Player>( world_ );
		world_->SetPlayer( player_ );
		world_->SetRenderer( world_renderer_ );
		world_renderer_->SetViewportSize( screen_width_, screen_height_ );

		world_renderer_->InitGL();

		world_->StartUpdates();

		game_started_= true;

		delete root_menu_;
		root_menu_=
			new ui_IngameMenu(
				screen_width_, screen_height_,
				[this](h_BlockType block_type)
				{
					build_block_= block_type;
				});
	}

}

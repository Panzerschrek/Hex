#include "main_loop.hpp"
#include "renderer/world_renderer.hpp"
#include "world.hpp"
#include "player.hpp"

#include "block_collision.hpp"
#include "console.hpp"
#include "ui/ui_base_classes.hpp"
#include "ui/ui_painter.hpp"
#include "ogl_state_manager.hpp"
#include "framebuffer.hpp"

#include "ui/main_menu.hpp"

void h_MainLoop::Start()
{
	QGLFormat format;

	QSettings settings( "config.ini", QSettings::IniFormat );
	int antialiasing= min( max( settings.value( "antialiasing", 4 ).toInt(), 0 ), 16  );
	if( antialiasing )
		format.setSamples( antialiasing );

	format.setVersion( 3, 3 );
	format.setProfile( QGLFormat::CoreProfile );
	format.setSwapInterval(1);

	new h_MainLoop( format );
}

QSize h_MainLoop::minimumSizeHint() const
{
	return QSize( screen_width_, screen_height_ );
}

QSize h_MainLoop::sizeHint() const
{
	return QSize( screen_width_, screen_height_ );
}

h_MainLoop::h_MainLoop( const QGLFormat& format )
	: QGLWidget(format, NULL )
	, settings_( "config.ini", QSettings::IniFormat )
	, cam_pos_( 0.0f, 0.0f, 67.0f )
	, cam_ang_( 0.0f, 0.0f, 0.0f )
	, startup_time_(0,0,0,0)
	, world_renderer_(nullptr)
	, world_(nullptr)
	, player_(nullptr)
	, game_started_(false)
	, use_mouse_(false)
{
	window_= new QMainWindow( NULL, 0 );

	window_->move( 0, 0 );
	window_->setWindowTitle( "Hex" );
	window_->setWindowIcon( QIcon( QString( "src/hex-logo.ico" ) ) );

	window_->setCentralWidget( this );
	window_->setContentsMargins( 0, 0, 0, 0 );

	screen_height_= min( max( settings_.value( "screen_height", 640 ).toInt(), H_MIN_SCREEN_HEIGHT ), H_MAX_SCREEN_WIDTH  );
	screen_width_=  min( max( settings_.value( "screen_width" , 480 ).toInt(), H_MIN_SCREEN_WIDTH  ), H_MAX_SCREEN_HEIGHT );

	this->setFixedSize( screen_width_, screen_height_ );
	window_->setFocusPolicy( Qt::ClickFocus );
	this->setFocusPolicy( Qt::ClickFocus );
	this->setFocus();

	window_->show();
	window_->setFixedSize( window_->size() );

	this->setAutoFillBackground( false );

	for( int i= 0; i< 512; i++ )
		keys_[i]= false;
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
		main_menu_= new ui_MainMenu( this, screen_width_, screen_height_ );
	}

	main_menu_->Tick();

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

	main_menu_->Draw( ui_painter_ );

	glFlush();
	update();

	frame_count_++;
}

void h_MainLoop::GetBuildPos()
{
	build_dir_= player_->GetBuildPos( &build_pos_x_, &build_pos_y_, &build_pos_z_ );

	m_Vec3 discret_build_pos( ( float( build_pos_x_ + 0.3333333333f ) ) * H_SPACE_SCALE_VECTOR_X,
							  float( build_pos_y_ ) - 0.5f * float(build_pos_x_&1) + 0.5f,
							  float( build_pos_z_ ) - 1.0f );
	if( build_dir_ == DIRECTION_UNKNOWN )
		discret_build_pos.z= -1.0f;

	world_renderer_->SetBuildPos( discret_build_pos );
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
		ang_delta.z= float( screen_width_/2 - cur_local_pos.x() ) * 0.005f;
		ang_delta.x= float( screen_height_/2 - cur_local_pos.y() ) * 0.005f;
		ang_delta.y= 0.0f;
		player_->Rotate( ang_delta );
		cam_ang_= player_->Angle();

		cur_local_pos= this->mapFromGlobal( QPoint( 0, 0 ) );
		cursor_.setPos( screen_width_/2 - cur_local_pos.x(), screen_height_/2 - cur_local_pos.y() );
	}

	static  int prev_time= 0;
	int time;
	float dt;
	time=  -QTime::currentTime().msecsTo( startup_time_ );
	dt= float( time - prev_time ) / 1000.0f;
	prev_time= time;

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
	{
		move_vec.z+= dt * speed;
	}
	if( keys_[ Qt::Key_C ] )
	{
		move_vec.z-= dt * speed;
	}

	//if( dt < 0.25f )
	//    move_vec.z+= -3.0f * dt;

	player_->Move( move_vec );
	cam_pos_= player_->Pos();
	cam_pos_.z+= H_PLAYER_EYE_LEVEL;

}

static void* GetGLProcessAddress(const char* name)
{
	return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
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

	if( e->button() == Qt::RightButton )
		world_->AddBuildEvent( build_pos_x_ - world_->Longitude() * H_CHUNK_WIDTH,
							  build_pos_y_ - world_->Latitude() * H_CHUNK_WIDTH,
							  build_pos_z_, FIRE_STONE );
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
			world_->Blast( new_build_pos[0] - world_->Longitude() * H_CHUNK_WIDTH,
						  new_build_pos[1] - world_->Latitude() * H_CHUNK_WIDTH,
						  new_build_pos[2], 4 );
		else
			world_->AddDestroyEvent( new_build_pos[0] - world_->Longitude() * H_CHUNK_WIDTH,
									new_build_pos[1] - world_->Latitude() * H_CHUNK_WIDTH,
									new_build_pos[2] );
	}
	else if( e->button() == Qt::MiddleButton )
	{
		world_->AddBuildEvent( build_pos_x_ - world_->Longitude() * H_CHUNK_WIDTH,
							  build_pos_y_ - world_->Latitude() * H_CHUNK_WIDTH,
							  build_pos_z_, SPHERICAL_BLOCK );
	}
}
void h_MainLoop::mouseMoveEvent(QMouseEvent* e)
{
}
void h_MainLoop::keyPressEvent(QKeyEvent* e)
{
	if( e->key() < 512 )
		keys_[ e->key() ]= true;

	if( e->key() == Qt::Key_M )
	{
		use_mouse_= !use_mouse_;
		printf( "cam_ang.z= %f\n", cam_ang_.z / M_PI * 180.0f );
		printf( "cam_ang.x= %f\n", cam_ang_.x/ M_PI * 180.0f );
	}
	else if( e->key() == Qt::Key_B )
		printf( "build: %d %d %d\n", build_pos_x_, build_pos_y_, build_pos_z_ );
	else if( e->key() == Qt::Key_QuoteLeft )
		h_Console::OpenClose();
	else if( e->key() == Qt::Key_Q )
	{
		if( game_started_ )
			world_->Save();
	}
	return;
}

void h_MainLoop::keyReleaseEvent(QKeyEvent* e)
{
	if( e->key() < 512 )
		keys_[ e->key() ]= false;
}

void h_MainLoop::focusInEvent(QFocusEvent *)
{
}

void h_MainLoop::focusOutEvent( QFocusEvent *)
{
	use_mouse_= false;
}

void h_MainLoop::closeEvent(QCloseEvent* e)
{
}

void h_MainLoop::Quit()
{
	exit(0);
}

void h_MainLoop::StartGame()
{
	if( !game_started_ )
	{
		world_= new h_World();
		world_renderer_= new r_WorldRenderer( world_ );
		player_= new h_Player( world_ );
		world_->SetPlayer( player_ );
		world_->SetRenderer( world_renderer_ );
		world_renderer_->SetViewportSize( screen_width_, screen_height_ );

		world_renderer_->InitGL();

		world_->StartUpdates();

		game_started_= true;
	}

}

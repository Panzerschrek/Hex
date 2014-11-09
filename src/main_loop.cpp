#ifndef MAIN_LOOP_CPP
#define MAIN_LOOP_CPP

#include "main_loop.hpp"
#include "renderer/world_renderer.hpp"
#include "world.hpp"
#include "player.hpp"

//#include "glcorearb.h"
#include "block_collision.hpp"
#include "console.hpp"
#include "ui/ui_base_classes.hpp"
#include "ui/ui_painter.hpp"
#include "renderer/ogl_state_manager.hpp"

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

h_MainLoop::h_MainLoop(QGLFormat format ):
    QGLWidget(format, NULL ),
    settings( "config.ini", QSettings::IniFormat ),
    cam_pos( 0.0f, 0.0f, 67.0f ),
    cam_ang( 0.0f, 0.0f, 0.0f ),
    startup_time(0,0,0,0),
    world_renderer(nullptr),
    world(nullptr),
    player(nullptr),
    game_started(false),
    use_mouse( false )
{
    window= new QMainWindow( NULL, 0 );

    window->move( 0, 0 );
    window->setWindowTitle( "Hex" );
    window->setWindowIcon( QIcon( QString( "src/hex-logo.ico" ) ) );

    window->setCentralWidget( this );
    window->setContentsMargins( 0, 0, 0, 0 );

    screen_height= min( max( settings.value( "screen_height", 640 ).toInt(), H_MIN_SCREEN_HEIGHT ), H_MAX_SCREEN_WIDTH  );
    screen_width=  min( max( settings.value( "screen_width" , 480 ).toInt(), H_MIN_SCREEN_WIDTH  ), H_MAX_SCREEN_HEIGHT );

    this->setFixedSize( screen_width, screen_height );
    //world_renderer->SetViewportSize( screen_width, screen_height );

    window->setFocusPolicy( Qt::ClickFocus );
    this->setFocusPolicy( Qt::ClickFocus );
    this->setFocus();

    window->show();
    window->setFixedSize( window->size() );

    this->setAutoFillBackground( false );

    for( int i= 0; i< 512; i++ )
        keys[i]= false;
	//window->setWindowState( Qt::WindowFullScreen );

	frame_count= 0;

}

h_MainLoop::~h_MainLoop()
{
	//delete renderer;
	//delete window;
	//delete world;
//	delete player;
}

void h_MainLoop::GetBuildPos()
{
    build_dir= player->GetBuildPos( &build_pos_x, &build_pos_y, &build_pos_z );

    m_Vec3 discret_build_pos( ( float( build_pos_x + 0.3333333333f ) ) * H_SPACE_SCALE_VECTOR_X,
                              float( build_pos_y ) - 0.5f * float(build_pos_x&1) + 0.5f,
                              float( build_pos_z ) - 1.0f );
    if( build_dir == DIRECTION_UNKNOWN )
        discret_build_pos.z= -1.0f;

    world_renderer->SetBuildPos( discret_build_pos );
}

void h_MainLoop::Input()
{
	QPoint cur_local_pos= this->mapFromGlobal( cursor.pos() );
	ui_CursorHandler::UpdateCursorPos( cur_local_pos.x(), cur_local_pos.y() );

	if( !game_started )
		return;

    if( use_mouse )
    {
        m_Vec3 ang_delta;
        ang_delta.z= float( screen_width/2 - cur_local_pos.x() ) * 0.005f;
        ang_delta.x= float( screen_height/2 - cur_local_pos.y() ) * 0.005f;
        ang_delta.y= 0.0f;
        player->Rotate( ang_delta );
        cam_ang= player->Angle();

        cur_local_pos= this->mapFromGlobal( QPoint( 0, 0 ) );
        cursor.setPos( screen_width/2 - cur_local_pos.x(), screen_height/2 - cur_local_pos.y() );
    }

    static  int prev_time= 0;
    int time;
    float dt;
    time=  -QTime::currentTime().msecsTo( startup_time );
    dt= float( time - prev_time ) / 1000.0f;
    prev_time= time;

    const float speed= 5.0f;


    m_Vec3 move_vec( 0.0f, 0.0f, 0.0f );
    if( keys[ Qt::Key_W ] )
    {
        move_vec.y+= dt * speed * m_Math::Cos( cam_ang.z );
        move_vec.x-= dt * speed * m_Math::Sin( cam_ang.z );
    }
    if( keys[ Qt::Key_S ] )
    {
        move_vec.y-= dt * speed * m_Math::Cos( cam_ang.z );
        move_vec.x+= dt * speed * m_Math::Sin( cam_ang.z );
    }
    if( keys[ Qt::Key_A ] )
    {
        move_vec.y-= dt * speed * m_Math::Cos( cam_ang.z - m_Math::FM_PI2);
        move_vec.x+= dt * speed * m_Math::Sin( cam_ang.z - m_Math::FM_PI2);
    }
    if( keys[ Qt::Key_D ] )
    {
        move_vec.y-= dt * speed * m_Math::Cos( cam_ang.z + m_Math::FM_PI2);
        move_vec.x+= dt * speed * m_Math::Sin( cam_ang.z + m_Math::FM_PI2);
    }

    if( keys[ Qt::Key_Space ] )
    {
        move_vec.z+= dt * speed;
    }
    if( keys[ Qt::Key_C ] )
    {
        move_vec.z-= dt * speed;
    }

    //if( dt < 0.25f )
    //    move_vec.z+= -3.0f * dt;

    player->Move( move_vec );
    cam_pos= player->Pos();
    cam_pos.z+= H_PLAYER_EYE_LEVEL;

}

void h_MainLoop::resizeGL(int w , int h)
{
    glViewport(0,0,w,h);
}

QSize h_MainLoop::minimumSizeHint() const
{
    return QSize( screen_width, screen_height );
}

QSize h_MainLoop::sizeHint() const
{
    return QSize( screen_width, screen_height );
}


//Main rendering loop here
void h_MainLoop::paintGL()
{
	 glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if( game_started )
	{
		player->Lock();
		Input();
		world_renderer->SetCamAng( cam_ang );
		world_renderer->SetCamPos( cam_pos );
		GetBuildPos();
		player->Unlock();
		world_renderer->Draw();

	}
	else
		Input();

	if( frame_count == 0 )
	{
		ui_painter= new ui_Painter();
		main_menu= new ui_MainMenu( this, screen_width, screen_height );
	}

	main_menu->Tick();

	m_Mat4 mat;
	mat.Identity();

	mat[0]= 2.0f / float( screen_width );
	mat[12]= -1.0f;
	mat[5]= -2.0f / float( screen_height );
	mat[13]= 1.0f;
	ui_painter->SetMatrix( mat );

	r_OGLStateManager::EnableBlend();
	r_OGLStateManager::DisableFaceCulling();
	r_OGLStateManager::DisableDepthTest();
	r_OGLStateManager::DepthMask(true);
	r_OGLStateManager::BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	main_menu->Draw( ui_painter );

    glFlush();
    update();

    frame_count++;
}
void h_MainLoop::initializeGL()
{
	GetGLFunctions();

    r_OGLState state;
    state.InitialState();
    r_OGLStateManager::SetState( state );
}

void h_MainLoop::mouseReleaseEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), true );
}
void h_MainLoop::mousePressEvent(QMouseEvent* e)
{
	ui_CursorHandler::CursorPress( e->x(), e->y(), false );

	if( !game_started )
		return;

    if( e->button() == Qt::RightButton )
        world->AddBuildEvent( build_pos_x - world->Longitude() * H_CHUNK_WIDTH,
                              build_pos_y - world->Latitude() * H_CHUNK_WIDTH,
                              build_pos_z, FIRE_STONE );
    else if( e->button() == Qt::LeftButton )
    {
        short new_build_pos[]= { build_pos_x, build_pos_y, build_pos_z };
        switch ( build_dir )
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
        if( keys[ Qt::Key_X ] )
            world->Blast( new_build_pos[0] - world->Longitude() * H_CHUNK_WIDTH,
                          new_build_pos[1] - world->Latitude() * H_CHUNK_WIDTH,
                          new_build_pos[2], 4 );
        else
            world->AddDestroyEvent( new_build_pos[0] - world->Longitude() * H_CHUNK_WIDTH,
                                    new_build_pos[1] - world->Latitude() * H_CHUNK_WIDTH,
                                    new_build_pos[2] );
    }
    else if( e->button() == Qt::MiddleButton )
    {
        world->AddBuildEvent( build_pos_x - world->Longitude() * H_CHUNK_WIDTH,
                              build_pos_y - world->Latitude() * H_CHUNK_WIDTH,
                              build_pos_z, SPHERICAL_BLOCK );
    }
}
void h_MainLoop::mouseMoveEvent(QMouseEvent* e)
{
}
void h_MainLoop::keyPressEvent(QKeyEvent* e)
{
    if( e->key() < 512 )
        keys[ e->key() ]= true;

    if( e->key() == Qt::Key_M )
    {
        use_mouse= !use_mouse;
        printf( "cam_ang.z= %f\n", cam_ang.z / M_PI * 180.0f );
        printf( "cam_ang.x= %f\n", cam_ang.x/ M_PI * 180.0f );
    }
    else if( e->key() == Qt::Key_B )
        printf( "build: %d %d %d\n", build_pos_x, build_pos_y, build_pos_z );
	else if( e->key() == Qt::Key_QuoteLeft )
		h_Console::OpenClose();
	else if( e->key() == Qt::Key_Q )
	{
		if( game_started )
			world->Save();
	}
    return;
}
void h_MainLoop::focusOutEvent( QFocusEvent *)
{
    use_mouse= false;
}
void h_MainLoop::focusInEvent(QFocusEvent *)
{
}
void h_MainLoop::keyReleaseEvent(QKeyEvent* e)
{
    if( e->key() < 512 )
        keys[ e->key() ]= false;
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
	if( !game_started )
	{
		world= new h_World();
		world_renderer= new r_WorldRenderer( world );
		player= new h_Player( world );
		world->SetPlayer( player );
		world->SetRenderer( world_renderer );
		world_renderer->SetViewportSize( screen_width, screen_height );

		world_renderer->InitGL();

		world->StartUpdates();

		game_started= true;
	}

}

#endif//MAIN_LOOP_CPP

#ifndef MAIN_LOOP_CPP
#define MAIN_LOOP_CPP

#include "main_loop.hpp"
//#include "glcorearb.h"
#include "block_collision.hpp"

void h_MainLoop::Start()
{
    QGLFormat format;
    format.setSwapInterval(1);
#ifdef OGL21
    format.setVersion( 2, 1 );
    format.setProfile( QGLFormat::CoreProfile );
#else
    format.setVersion( 3, 3 );
    format.setProfile( QGLFormat::CoreProfile );
#endif

    new h_MainLoop( format );
}

h_MainLoop::h_MainLoop(QGLFormat format ):
    QGLWidget(format, NULL),
    cam_pos( 0.0f, 0.0f, 67.0f ),
    cam_ang( 0.0f, 0.0f, 0.0f ),
    startup_time(0,0,0,0),
    use_mouse( false )
{
    world= new h_World();
    renderer= new r_Renderer( world );
    player= new h_Player( world );
    world->SetPlayer( player );

    window= new QWidget( NULL, 0);
    layout= new QVBoxLayout();

    window->setLayout(layout);
    window->move( 0, 0 );
    window->setWindowTitle( "Hex" );
    layout->addWidget( (QWidget*)this, 0 );
    layout->setMargin(0);


    screen_height=768;
    screen_width= 1024;
    this->setFixedSize( screen_width, screen_height );
    renderer->SetViewportSize( screen_width, screen_height );

    window->setFocusPolicy( Qt::ClickFocus );
    this->setFocusPolicy( Qt::ClickFocus );
    this->setFocus();

    window->show();
    window->setFixedSize( window->size() );

    for( int i= 0; i< 512; i++ )
        keys[i]= false;
}

void h_MainLoop::GetBuildPos()
{
    /* build_pos_z= (short) floor( cam_pos.z  + 1.0f );
     GetHexogonCoord( cam_pos.xy(), &build_pos_x, &build_pos_y );

     if( cam_ang.x > M_PI/4.0f )
         build_pos_z++;
     else if( cam_ang.x < -M_PI/4.0f )
         build_pos_z--;
     else
     {
         if( cam_ang.z < M_PI/6.0f )
             build_pos_y++;
         else if( cam_ang.z < M_PI/2.0f )
         {
             build_pos_y+= ( build_pos_x+1)&1;
             build_pos_x--;
         }
         else if ( cam_ang.z < 5.0f * M_PI / 6.0f )
         {
             build_pos_y-= build_pos_x&1;
             build_pos_x--;
         }
         else if( cam_ang.z < 7.0f * M_PI / 6.0f )
         {
             build_pos_y--;
         }
         else if( cam_ang.z < 3.0f * M_PI / 2.0f )
         {
             build_pos_y-= build_pos_x&1;
             build_pos_x++;
         }
         else if ( cam_ang.z < 11.0f * M_PI / 6.0f )
         {
             build_pos_y+= ( build_pos_x+ 1 )&1;
             build_pos_x++;
         }
         else
             build_pos_y++;


     }*/

    build_dir= player->GetBuildPos( &build_pos_x, &build_pos_y, &build_pos_z );

    m_Vec3 discret_build_pos( ( float( build_pos_x + 0.3333333333f ) ) * H_SPACE_SCALE_VECTOR_X,
                              float( build_pos_y ) - 0.5f * float(build_pos_x&1) + 0.5f,
                              float( build_pos_z ) - 1.0f );
	if( build_dir == DIRECTION_UNKNOWN )
		discret_build_pos.z= -1.0f;

    renderer->SetBuildPos( discret_build_pos );
}

void h_MainLoop::Input()
{
    if( use_mouse )
    {
        QPoint cur_local_pos= this->mapFromGlobal( cursor.pos() );

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
    player->Lock();
    Input();
    renderer->SetCamAng( cam_ang );
    renderer->SetCamPos( cam_pos );
    GetBuildPos();
    player->Unlock();
    renderer->Draw();
    glFlush();
    update();
}
void h_MainLoop::initializeGL()
{
    renderer->InitGL();
}

void h_MainLoop::mousePressEvent(QMouseEvent* e)
{
    if( e->button() == Qt::RightButton )
        world->AddBuildEvent( build_pos_x - world->Longitude() * H_CHUNK_WIDTH,
                      build_pos_y - world->Latitude() * H_CHUNK_WIDTH,
                      build_pos_z, FIRE );
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
                      build_pos_z, WOOD );
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
#endif//MAIN_LOOP_CPP

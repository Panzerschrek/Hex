#ifndef MAIN_LOOP_HPP
#define MAIN_LOOP_HPP

#include <QtOpenGL>

#include "renderer/renderer.hpp"
#include "player.hpp"
//enum Qt::Key;

class h_MainLoop : public QGLWidget
{
    Q_OBJECT

public:
   	static void Start();
    //h_MainLoop(){}
    virtual ~h_MainLoop() {}

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;

protected:
	h_MainLoop( QGLFormat format );
    virtual void initializeGL();
    virtual void resizeGL(int w , int h);
    virtual void paintGL();
    void mousePressEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void keyPressEvent(QKeyEvent* e);
    void focusOutEvent( QFocusEvent *);
    void focusInEvent(QFocusEvent *);
    void keyReleaseEvent(QKeyEvent* e);
    void closeEvent(QCloseEvent* e);


    virtual void initializeOverlayGL() {}
    virtual void resizeOverlayGL(int w, int h) {}
    virtual void paintOverlayGL() {}
    //virtual void glInit() {}
   // virtual void glDraw() {}

public slots:
    void setXRotation(int) {}
    void setYRotation(int) {}
    void setZRotation(int) {}

private:

	void Input();
	void GetBuildPos();

	QVBoxLayout *layout;
    QWidget* window;
    QCursor cursor;
    QTime startup_time;
    bool keys[ 512 ];
    bool use_mouse;

    int screen_width, screen_height;
    r_Renderer* renderer;
    h_World* world;
    h_Player* player;

    m_Vec3 cam_ang, cam_pos;
    short build_pos_x, build_pos_y, build_pos_z;
    h_Direction build_dir;


};

#endif//MAIN_LOOP_HPP

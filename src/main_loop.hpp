#pragma once
#include <QtOpenGL>

#include "player.hpp"

class ui_Painter;
class ui_MainMenu;

class r_WorldRenderer;

class h_MainLoop : public QGLWidget
{
	Q_OBJECT

public:
	static void Start();
	//h_MainLoop(){}
	virtual ~h_MainLoop();

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

protected:
	h_MainLoop( QGLFormat format );
	virtual void initializeGL();
	virtual void resizeGL(int w , int h);
	virtual void paintGL();
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
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

public://main menu interface logic
	void Quit();
	void StartGame();

private:

	void Input();
	void GetBuildPos();

	QMainWindow* window;
	QCursor cursor;
	QTime startup_time;
	QSettings settings;
	bool keys[ 512 ];
	bool use_mouse;

	int screen_width, screen_height;


	r_WorldRenderer* world_renderer;
	h_World* world;
	h_Player* player;

	bool game_started;

	m_Vec3 cam_ang, cam_pos;

	short build_pos_x, build_pos_y, build_pos_z;
	h_Direction build_dir;

	ui_Painter* ui_painter;
	ui_MainMenu* main_menu;

	int frame_count;

};

#pragma once
#include "hex.hpp"
#include "fwd.hpp"
#include <QtOpenGL>

#include "player.hpp"

class h_MainLoop : public QGLWidget
{
	Q_OBJECT

public:
	static void Start();

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

protected:
	h_MainLoop( QGLFormat format );
	virtual ~h_MainLoop() override;

	virtual void initializeGL() override;
	virtual void resizeGL(int w , int h) override;
	virtual void paintGL()  override;

	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void keyReleaseEvent(QKeyEvent* e) override;
	void focusInEvent(QFocusEvent *) override;
	void focusOutEvent( QFocusEvent *) override;
	void closeEvent(QCloseEvent* e) override;

	virtual void initializeOverlayGL() override {}
	virtual void resizeOverlayGL(int w, int h) override {}
	virtual void paintOverlayGL() override {}

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

private:
	QMainWindow* window_;
	QCursor cursor_;
	QTime startup_time_;
	QSettings settings_;
	bool keys_[ 512 ];
	bool use_mouse_;

	int screen_width_, screen_height_;

	r_WorldRenderer* world_renderer_;
	h_World* world_;
	h_Player* player_;

	bool game_started_;

	m_Vec3 cam_ang_, cam_pos_;

	short build_pos_x_, build_pos_y_, build_pos_z_;
	h_Direction build_dir_;

	ui_Painter* ui_painter_;
	ui_MainMenu* main_menu_;

	int frame_count_;
};

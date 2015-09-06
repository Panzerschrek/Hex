#pragma once
#include "hex.hpp"
#include "fwd.hpp"
#include "vec.hpp"

#include <QtGlobal>
#include <QObject>
#include <QtOpenGL>

class h_MainLoop : public QGLWidget
{
	Q_OBJECT

public:
	static void Start();

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

protected:
	h_MainLoop( const h_SettingsPtr& settings, const QGLFormat& format );
	virtual ~h_MainLoop() override;

	virtual void initializeGL() override;
	virtual void resizeGL(int w , int h) override;
	virtual void paintGL()  override;

	virtual void mousePressEvent(QMouseEvent* e) override;
	virtual void mouseReleaseEvent(QMouseEvent* e) override;
	virtual void mouseMoveEvent(QMouseEvent* e) override;
	virtual void keyPressEvent(QKeyEvent* e) override;
	virtual void keyReleaseEvent(QKeyEvent* e) override;
	virtual void focusInEvent(QFocusEvent *) override;
	virtual void focusOutEvent( QFocusEvent *) override;
	virtual void closeEvent(QCloseEvent* e) override;

	virtual void initializeOverlayGL() override {}
	virtual void resizeOverlayGL(int w, int h) override {}
	virtual void paintOverlayGL() override {}

public://main menu interface logic
	void Quit();
	void StartGame();

private:
	void Input();
	void GetBuildPos();

private:
	h_SettingsPtr settings_;

	QMainWindow* window_;
	QCursor cursor_;
	QTime startup_time_;
	bool keys_[ 512 ];
	bool use_mouse_;

	int screen_width_, screen_height_;

	r_WorldRendererPtr world_renderer_;
	h_WorldPtr world_;
	h_PlayerPtr player_;

	bool game_started_;

	m_Vec3 cam_pos_, cam_ang_;

	short build_pos_x_, build_pos_y_, build_pos_z_;
	h_Direction build_dir_;

	ui_Painter* ui_painter_;
	ui_MainMenu* main_menu_;

	int frame_count_;
};

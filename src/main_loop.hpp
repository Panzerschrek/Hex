#pragma once

#include "hex.hpp"
#include "fwd.hpp"

#include <QtGlobal>
#include <QObject>
#include <QtOpenGL>

class h_MainLoop final : public QGLWidget
{
	Q_OBJECT

public:
	static void Start();

protected:
	h_MainLoop( const h_SettingsPtr& settings, const QGLFormat& format );
	virtual ~h_MainLoop() override;

	virtual void initializeGL() override;
	virtual void resizeGL(int w , int h) override;
	virtual void paintGL() override;

	virtual void mousePressEvent(QMouseEvent* e) override;
	virtual void mouseReleaseEvent(QMouseEvent* e) override;
	virtual void mouseMoveEvent(QMouseEvent* e) override;
	virtual void keyPressEvent(QKeyEvent* e) override;
	virtual void keyReleaseEvent(QKeyEvent* e) override;
	virtual void closeEvent(QCloseEvent* e) override;

	virtual void initializeOverlayGL() override;
	virtual void resizeOverlayGL(int w, int h) override;
	virtual void paintOverlayGL() override;

public://main menu interface logic
	void Quit();
	void StartGame();

private:
	void UpdateCursor();
	void ProcessMenuKey( QKeyEvent* e, bool pressed );

private:
	const h_SettingsPtr settings_;

	QCursor cursor_;

	int screen_width_, screen_height_;

	h_WorldHeaderPtr world_header_;
	r_WorldRendererPtr world_renderer_;
	h_WorldPtr world_;
	h_PlayerPtr player_;

	bool game_started_;

	std::unique_ptr<ui_Painter> ui_painter_;
	std::unique_ptr<ui_MenuBase> root_menu_;
};

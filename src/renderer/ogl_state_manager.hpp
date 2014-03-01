#ifndef OGL_STATE_MANAGER_H
#define OGL_STATE_MANAGER_H

#include "ph.h"


class r_OGLState
{
	public:

	void InitialState();
	bool blend;
	bool cull_face;
	bool depth_test;
	bool program_point_size;

	GLenum blend_func[2];
	float clear_color[4];
	float clear_depth;
	GLenum cull_face_mode;
	GLenum depth_func;
	bool depth_mask;
	GLfloat line_width;

};

class r_OGLStateManager
{
	public:
	void BlendFunc( GLenum sfactor, GLenum dfactor );
	void ClearColor( float red, float green, float blue, float alpha );
	void ClearDepth( float depth );
	void CullFace( GLenum mode );
	void DepthFunc( GLenum func );
	void LineWidth( float width );

	void EnableBlend();
	void DisableBlend();
	void EnableFaceCulling();
	void DisableFaceCulling();
	void EnableDepthTest();
	void DisableDepthTest();
	void DepthMask( bool mask );
	void EnableProgramPointSize();
	void DisableProgramPointSize();

	void SetState( 	const r_OGLState& state );
	void GetState( r_OGLState* state );
	private:

	r_OGLState state;

};


#endif//OGL_STATE_MANAGER_H

#ifndef OGL_STATE_MANAGER_H
#define OGL_STATE_MANAGER_H

#include "ph.h"


class r_OGLState
{
	public:
	r_OGLState(){}
	~r_OGLState(){}
	//constructor for full initialazing
	r_OGLState( bool blend, bool cull_face, bool depth_test, bool program_point_size,
				const GLenum* blend_func, const float* clear_color, float clear_depth, GLenum cull_face_mode, bool depth_mask );

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
};

class r_OGLStateManager
{
	public:
	static void BlendFunc( GLenum sfactor, GLenum dfactor );
	static void ClearColor( float red, float green, float blue, float alpha );
	static void ClearDepth( float depth );
	static void CullFace( GLenum mode );
	static void DepthFunc( GLenum func );

	static void EnableBlend();
	static void DisableBlend();
	static void EnableFaceCulling();
	static void DisableFaceCulling();
	static void EnableDepthTest();
	static void DisableDepthTest();
	static void DepthMask( bool mask );
	static void EnableProgramPointSize();
	static void DisableProgramPointSize();

	static void SetState( const r_OGLState& state );//FORCE set state
	static void UpdateState( const r_OGLState& state );
	static void GetState( r_OGLState* state );
	private:

	static r_OGLState state;

};


#endif//OGL_STATE_MANAGER_H

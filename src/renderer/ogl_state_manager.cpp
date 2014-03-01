#include "ogl_state_manager.hpp"

void r_OGLState::InitialState()
{
    blend= false;
    cull_face= false;
    depth_test= false;
    program_point_size= false;

    clear_color[0]= clear_color[1]= clear_color[2]= clear_color[3]= 0.0f;
    clear_depth= 1.0f;
    cull_face_mode= GL_BACK;
    depth_func= GL_LESS;
    depth_mask= true;
    line_width= 1.0f;

}


void r_OGLStateManager::BlendFunc( GLenum sfactor, GLenum dfactor )
 {
 	if( state.blend_func[0] != sfactor || state.blend_func[1] != dfactor )
 	{
 		state.blend_func[0]= sfactor;
 		state.blend_func[1]= dfactor;
 		glBlendFunc( sfactor, dfactor );
 	}
 }
void r_OGLStateManager::ClearColor( float red, float green, float blue, float alpha )
 {
 	if( red != state.clear_color[0] || green != state.clear_color[1] || blue != state.clear_color[2] || alpha != state.clear_color[3] )
 	{
 		state.clear_color[0]= red;
 		state.clear_color[1]= green;
 		state.clear_color[2]= blue;
 		state.clear_color[3]= alpha;
 		glClearColor( red, green, blue, alpha );
 	}
 }

void r_OGLStateManager::ClearDepth( float depth )
 {
 	if( state.clear_depth != depth )
	{
		state.clear_depth= depth;
		glClearDepth( depth );
	}
 }

void r_OGLStateManager::CullFace( GLenum mode )
 {
 	if( state.cull_face_mode != mode )
 	{
 		state.cull_face_mode= mode;
 		glCullFace( mode );
 	}
 }
void r_OGLStateManager::DepthFunc( GLenum func )
 {
 	if( state.depth_func != func )
 	{
 		state.depth_func= func;
 		glDepthFunc( func );
 	}
 }
void r_OGLStateManager::LineWidth( float width )
 {
 	if( state.line_width != width )
 	{
 		state.line_width= width;
 		glLineWidth( width );
 	}
 }

void r_OGLStateManager::EnableBlend()
 {
 	if( ! state.blend )
 	{
 		state.blend= true;
 		glEnable( GL_BLEND );
 	}
 }
void r_OGLStateManager::DisableBlend()
 {
 	if( state.blend )
 	{
 		state.blend= false;
 		glDisable( GL_BLEND );
 	}
 }

void r_OGLStateManager::EnableFaceCulling()
 {
 	if( !state.cull_face )
 	{
 		state.cull_face= true;
 		glEnable( GL_CULL_FACE );
 	}
 }
void r_OGLStateManager::DisableFaceCulling()
 {
 	if( state.cull_face )
 	{
 		state.cull_face= false;
 		glDisable( GL_CULL_FACE );
 	}
 }
void r_OGLStateManager::EnableDepthTest()
 {
 	if( !state.depth_test )
 	{
 		state.depth_test= true;
 		glEnable( GL_DEPTH_TEST );
 	}
 }
void r_OGLStateManager::DisableDepthTest()
 {
 	if( state.depth_test )
 	{
 		state.depth_test= false;
 		glDisable( GL_DEPTH_TEST );
 	}
 }
void r_OGLStateManager::DepthMask( bool mask )
 {
 	if( state.depth_mask != mask )
 	{
 		state.depth_mask= mask;
 		glDepthMask( mask );
 	}
 }
void r_OGLStateManager::EnableProgramPointSize()
 {
 	if( !state.program_point_size )
 	{
 		state.program_point_size= true;
 		glEnable( GL_PROGRAM_POINT_SIZE );
 	}
 }

 void r_OGLStateManager::DisableProgramPointSize()
 {
 	if( state.program_point_size )
 	{
 		state.program_point_size= false;
 		glDisable( GL_PROGRAM_POINT_SIZE );
 	}
 }

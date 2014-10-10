#include "ogl_state_manager.hpp"


r_OGLState r_OGLStateManager::state;

void r_OGLState::InitialState()
{
    blend= false;
    cull_face= false;
    depth_test= false;
    program_point_size= false;

    blend_func[0]= blend_func[1]= GL_ONE;
    clear_color[0]= clear_color[1]= clear_color[2]= clear_color[3]= 0.0f;
    clear_depth= 1.0f;
    cull_face_mode= GL_BACK;
    depth_func= GL_LEQUAL;
    depth_mask= true;
}

r_OGLState::r_OGLState( bool blend_in, bool cull_face_in, bool depth_test_in, bool program_point_size_in,
					const GLenum* blend_func_in, const float* clear_color_in, float clear_depth_in, GLenum cull_face_mode_in, bool depth_mask_in ):

blend(blend_in), cull_face(cull_face_in), depth_test(depth_test_in), program_point_size(program_point_size_in),
blend_func{ blend_func_in[0], blend_func_in[1] }, clear_color{ clear_color_in[0], clear_color_in[1], clear_color_in[2], clear_color_in[3] },
clear_depth(clear_depth_in), cull_face_mode(cull_face_mode_in), depth_mask(depth_mask_in)
{
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

void r_OGLStateManager::SetState( const r_OGLState& state )
{
    r_OGLStateManager::state= state;
    glBlendFunc( state.blend_func[0], state.blend_func[1] );
    glClearColor( state.clear_color[0], state.clear_color[1], state.clear_color[2], state.clear_color[3] );
    glClearDepth( state.clear_depth );
    glCullFace( state.cull_face_mode );
    glDepthFunc( state.depth_func );
    glDepthMask( state.depth_mask );

    if( state.blend )
        glEnable( GL_BLEND );
    else
        glDisable( GL_BLEND );

    if( state.cull_face )
        glEnable( GL_CULL_FACE );
    else
        glDisable( GL_CULL_FACE );

    if( state.depth_test )
        glEnable( GL_DEPTH_TEST );
    else
        glDisable( GL_DEPTH_TEST );

    if( state.program_point_size )
        glEnable( GL_PROGRAM_POINT_SIZE );
    else
        glDisable( GL_PROGRAM_POINT_SIZE );
}


void r_OGLStateManager::UpdateState( const r_OGLState& state )
{
	r_OGLState* old_state= &r_OGLStateManager::state;

	if( old_state->blend != state.blend )
		( state.blend ? glEnable : glDisable )( GL_BLEND );

	if( old_state->cull_face != state.cull_face )
		( state.cull_face ? glEnable : glDisable )( GL_CULL_FACE );

	if( old_state->depth_test != state.depth_test )
		( state.depth_test ? glEnable : glDisable )( GL_DEPTH_TEST );

	if( old_state->program_point_size != state.program_point_size )
		( state.program_point_size ? glEnable : glDisable )( GL_PROGRAM_POINT_SIZE );

	if( old_state->blend_func[0] != state.blend_func[0] || old_state->blend_func[1] != state.blend_func[1] )
		glBlendFunc( state.blend_func[0], state.blend_func[1] );

	if( old_state->clear_color[0] != state.clear_color[0] || old_state->clear_color[1] != state.clear_color[1] ||
		old_state->clear_color[2] != state.clear_color[2] || old_state->clear_color[2] != state.clear_color[2] )
		 glClearColor( state.clear_color[0], state.clear_color[1], state.clear_color[2], state.clear_color[3] );

	if( old_state->clear_depth != state.clear_depth )
    	glClearDepth( state.clear_depth );

	if( old_state->cull_face_mode != state.cull_face_mode )
    	glCullFace( state.cull_face_mode );

	if( old_state->depth_func != state.depth_func )
    	glDepthFunc( state.depth_func );
	if( old_state->depth_mask != state.depth_mask )
    	glDepthMask( state.depth_mask );


	r_OGLStateManager::state= state;
}

void r_OGLStateManager::GetState( r_OGLState* state )
{
    *state= r_OGLStateManager::state;
}

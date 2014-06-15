/*
*This file is part of FREG.
*
*FREG is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*FREG is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with FREG. If not, see <http://www.gnu.org/licenses/>.*/
#ifndef FUNC_DECLARATIONS_H
#define FUNC_DECLARATIONS_H
#include "ph.h"
/*shaders*/
extern PFNGLCREATESHADERPROC			glCreateShader;
extern PFNGLDELETESHADERPROC			glDeleteShader;
extern PFNGLSHADERSOURCEPROC			glShaderSource;
extern PFNGLCOMPILESHADERPROC			glCompileShader;
extern PFNGLRELEASESHADERCOMPILERPROC	glReleaseShaderCompiler;
extern PFNGLSHADERBINARYPROC			glShaderBinary;
extern PFNGLGETSHADERIVPROC				glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC		glGetShaderInfoLog;

/*programs*/
extern PFNGLCREATEPROGRAMPROC		glCreateProgram;
extern PFNGLDELETEPROGRAMPROC		glDeleteProgram;
extern PFNGLDETACHSHADERPROC		glDetachShader;
extern PFNGLATTACHSHADERPROC		glAttachShader;
extern PFNGLLINKPROGRAMPROC			glLinkProgram;
extern PFNGLUSEPROGRAMPROC			glUseProgram;
extern PFNGLGETPROGRAMIVPROC		glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC	glGetProgramInfoLog;
extern PFNGLGETATTRIBLOCATIONPROC	glGetAttribLocation;

/*attributes*/
extern PFNGLVERTEXATTRIBPOINTERPROC		glVertexAttribPointer;
extern PFNGLVERTEXATTRIBIPOINTERPROC		glVertexAttribIPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC	glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC 	glDisableVertexAttribArray;
extern PFNGLBINDATTRIBLOCATIONPROC			glBindAttribLocation;

/*VBO*/
extern PFNGLGENBUFFERSPROC			glGenBuffers;
extern PFNGLBINDBUFFERPROC			glBindBuffer;
extern PFNGLBUFFERDATAPROC			glBufferData;
extern PFNGLBUFFERSUBDATAPROC		glBufferSubData;
extern PFNGLGENVERTEXARRAYSPROC		glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC		glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC	glDeleteVertexArrays;
extern PFNGLDELETEBUFFERSPROC		glDeleteBuffers;
extern PFNGLMULTIDRAWELEMENTSPROC  glMultiDrawElements;
extern PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;
extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glMultiDrawElementsBaseVertex;
extern PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays;
extern PFNGLTEXBUFFERPROC glTexBuffer;

/*uniforms*/
extern PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation;
extern PFNGLUNIFORM1IPROC			glUniform1i;
extern PFNGLUNIFORMMATRIX4FVPROC	glUniformMatrix4fv;
extern PFNGLUNIFORMMATRIX3FVPROC	glUniformMatrix3fv;
extern PFNGLUNIFORM3FPROC			glUniform3f;
extern PFNGLUNIFORM1FPROC          glUniform1f;
extern PFNGLDRAWBUFFERSPROC         glDrawBuffers;


/*textures*/
extern PFNGLACTIVETEXTUREPROC	glActiveTexture;//есть в системном заголовочном файле
extern PFNGLGENERATEMIPMAPPROC	glGenerateMipmap;
extern PFNGLTEXIMAGE3DPROC      glTexImage3D;//есть в системном заголовочном файле
extern PFNGLTEXSUBIMAGE3DPROC  glTexSubImage3D;


/*FBO*/
extern PFNGLGENFRAMEBUFFERSPROC			glGenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC			glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTUREPROC		glFramebufferTexture;
extern PFNGLBINDFRAGDATALOCATIONPROC	glBindFragDataLocation;
extern PFNGLMINSAMPLESHADINGPROC			glMinSampleShading;

extern PFNGLDRAWELEMENTSINSTANCEDPROC	glDrawElementsInstanced;

extern PFNGLWAITSYNCPROC glWaitSync;
extern PFNGLFENCESYNCPROC glFenceSync;
//extern GLAPI const GLubyte * APIENTRY (*glGetStringi) (GLenum name, GLuint index);
//extern GLAPI void APIENTRY  ( * glClampColor ) (GLenum target, GLenum clamp);
//extern PFNWGLSWAPINTERVALEXTPROC wglSwapInterval;


void GetGLFunctions(void);
#endif//_NN_FUNC_DECLARATIONS_H_

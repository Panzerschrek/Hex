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
#ifndef PH_H
#define PH_H

#include <QtOpenGL/QGLWidget>
#include <QThread>
#include <QMutex>
#include <QTime>

#include "win_bitmap.h"
#ifndef _WIN32
#include <unistd.h>
#include "win_bitmap.h"
#else
#include <windows.h>
#endif

#include "glcorearb.h"
#include "glext.h"

#ifdef _WIN32
//#include "wglext.h"
#else
//#include <X11/X.h>
//#include <X11/keysym.h>
//#include "glxext.h"
#endif

#include "func_declarations.h"
#include <stdio.h>
#include <time.h>

#define MACRO_TO_STR(X) #X
#define MACRO_VALUE_TO_STR(X) MACRO_TO_STR(X)



//#define max( y, x )  ((x)>(y) ? (x) : (y) )
//#define min( y, x )  ((x)>(y) ? (y) : (x) )


#define R_MAX_SUB 32
#define R_MAX_SUB_LOG2 5
#define R_MAX_KIND 32
#define R_MAX_KIND_LOG2 5

#endif//PH_H

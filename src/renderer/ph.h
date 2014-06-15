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

//contains main gl.h hrader
#include <QtOpenGL/QGLWidget>


#include "win_bitmap.h"
#ifndef _WIN32
#include <unistd.h>
#include "win_bitmap.h"
#else
#include <windows.h>
#endif

//#include "glcorearb.h"
#include "glext.h"


#include "func_declarations.h"
#include <stdio.h>
#include <time.h>

#define MACRO_TO_STR(X) #X
#define MACRO_VALUE_TO_STR(X) MACRO_TO_STR(X)


#endif//PH_H

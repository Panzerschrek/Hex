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
*along with FREG. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FMD_FORMAT_H
#define FMD_FORMAT_H
#include <QtCore>


#define FMD_END_OF_FILE_SIGNATURE 0x0202

#define FMD_LATEST_MAJOR_VERSION 	0
#define FMD_LATEST_MINOR_VERSION 	8
#define FMD_INDEX_SIZE 				2

#define VERTEX_CONVERTION_SCALE 1024.0f
#define NORMAL_CONVERTION_SCALE 127.0f
#define TEX_COORD_CONVERTION_SCALE 2048.0f

#pragma pack( push, 1 )

struct fmd_FileHeader
{
    qint8 	format_code[4];	// должен быть 'F', 'M', 'D', 0
    quint8 	version_number[2];
    quint8 	reserved[10];
};

struct fmd_Vertex
{
    qint16 	coord[3];
    qint16 	tex_coord[2];
    qint8 	normal[3];
    qint8 	reserved[3];
};

#pragma pack( pop )

#endif//FMD_FORMAT_H

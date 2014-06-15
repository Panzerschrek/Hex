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

#ifndef WIN_BITMAP_H
#define WIN_BITMAP_H

#include <stdint.h>
#include <QtGlobal>

#pragma pack(push,2)
typedef struct//14 bytes
{
    quint16	bfType;
    quint32	bfSize;
    quint16	bfReserved1;
    quint16	bfReserved2;
    quint32	bfOffBits;
} r_BitmapFileHeader;
#pragma pack(pop)

#pragma pack ( push, 1 )
typedef struct//40 bytes
{
    quint32	biSize;
    qint32	    biWidth;
    qint32	    biHeight;
    quint16	biPlanes;
    quint16	biBitCount;
    quint32	biCompression;
    quint32	biSizeImage;
    qint32	    biXPelsPerMeter;
    qint32	    biYPelsPerMeter;
    quint32	biClrUsed;
    quint32	biClrImportant;
} r_BitmapInfoHeader;
#pragma pack(pop)

#ifndef BI_RGB
#define BI_RGB 0
#endif

#endif//WIN_BITMAP_H

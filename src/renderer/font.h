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
#ifndef FONT_H
#define FONT_H

#include "ph.h"
#include "texture.h"

#define R_MAX_LETTER_NUMBER 8192
#define R_LETTER_CLAMP_MASK 0x1FFF

enum r_FontPage: unsigned char
{
    R_FONT_PAGE_ASCII=  0,
    R_FONT_PAGE_LATIN,
    R_FONT_PAGE_CYRILLIC,
    R_FONT_PAGE_GREEK,
    R_FONT_PAGE_RESERVED0,
    R_FONT_PAGE_RESERVED1
};

#define R_FONT_MAX_TEXTURES (R_FONT_PAGE_RESERVED1 + 1)
class r_Font
{
public:

    unsigned char  TextureNum       ( unsigned short letter ) const;
    unsigned short TexCoordTop      ( unsigned short letter ) const;
    unsigned short TexCoordBottom   ( unsigned short letter ) const;
    unsigned short TexCoordLeft     ( unsigned short letter ) const;
    unsigned short TexCoordRight    ( unsigned short letter ) const;
    float LetterWidth               ( unsigned short letter ) const;
    unsigned char  LetterHeight() const;


    int AddFontTexture ( r_FontPage font_page, r_Texture* texture );
    int LoadFontPage   ( r_FontPage font_page, const char* file_name, const char* texture_file_name = NULL );
    const r_Texture* FontTexture( r_FontPage font_page ) const;

    r_Font();
    ~r_Font();
    private:
        r_Texture   *font_textures[ R_FONT_MAX_TEXTURES ];
        bool       textures_porper[ R_FONT_MAX_TEXTURES ];

        unsigned char letter_height;

        unsigned char  letter_tex_number         [ R_MAX_LETTER_NUMBER ];//номер текстуры
        unsigned short letter_tex_coord_top      [ R_MAX_LETTER_NUMBER ];
        unsigned short letter_tex_coord_bottom   [ R_MAX_LETTER_NUMBER ];
        unsigned short letter_tex_coord_left     [ R_MAX_LETTER_NUMBER ];
        unsigned short letter_tex_coord_right    [ R_MAX_LETTER_NUMBER ];
        float letter_relative_width [ R_MAX_LETTER_NUMBER ];//= width/height
};

unsigned  int r_Char2Hex( const char* s );


inline const r_Texture* r_Font::FontTexture( r_FontPage font_page ) const
{
    return font_textures[ font_page ];
}

inline unsigned char  r_Font::LetterHeight() const
{
    return letter_height;
}
inline float r_Font::LetterWidth ( unsigned short letter ) const
{
    return letter_relative_width[ letter & R_LETTER_CLAMP_MASK ];
}

inline unsigned char  r_Font::TextureNum( unsigned short letter ) const
{
    return letter_tex_number[ letter & R_LETTER_CLAMP_MASK ];
}

inline unsigned short r_Font::TexCoordTop ( unsigned short letter ) const
{
    return letter_tex_coord_top[ letter & R_LETTER_CLAMP_MASK ];
}

inline unsigned short r_Font::TexCoordBottom( unsigned short letter ) const
{
    return letter_tex_coord_bottom[ letter & R_LETTER_CLAMP_MASK ];
}

inline unsigned short r_Font::TexCoordLeft( unsigned short letter ) const
{
    return letter_tex_coord_left[ letter & R_LETTER_CLAMP_MASK ];
}

inline unsigned short r_Font::TexCoordRight( unsigned short letter ) const
{
    return letter_tex_coord_right[ letter & R_LETTER_CLAMP_MASK ];
}

#endif//FONT_H

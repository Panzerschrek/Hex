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
#ifndef FONT_CPP
#define FONT_CPP

#include "font.h"

unsigned  int r_Char2Hex( const char* s )
{
    unsigned int result= 0;
    unsigned int i=0;
    do
    {
        result*=16;

        if( s[i] >= '0' && s[i] <= '9' )
            result+= s[i] - '0';
        else if ( s[i] >= 'a' && s[i] <= 'f' )
            result+= 10 + s[i] - 'a';
        else if ( s[i] >= 'A' && s[i] <= 'F' )
            result+= 10 + s[i] - 'A';
        else
            break;

        i++;
    }while(s[i] != 0 );
    return result;
}

r_Font::r_Font()
{
    unsigned int i;
    for( i= 0; i< R_FONT_MAX_TEXTURES; i++ )
    {
        textures_porper[i]= true;
        font_textures[i]= NULL;
    }

    for( i= 0; i< R_MAX_LETTER_NUMBER; i++ )
    {

        letter_tex_number[i]=
            letter_tex_coord_top[i]=
                letter_tex_coord_bottom[i]=
                    letter_tex_coord_left[i]=
                        letter_tex_coord_right[i]=
                            letter_relative_width[i]= 0.0;
    }
}

r_Font::~r_Font()
{
    unsigned int i;
     for( i= 0; i< R_FONT_MAX_TEXTURES; i++ )
    {
        if( textures_porper[i] && font_textures[i] != NULL )
            delete font_textures[i];
    }
}

int r_Font::LoadFontPage( r_FontPage font_page, const char* file_name, const char* texture_file_name )
{
    //если текстура уже есть и передано имя новой текстуры
    if( texture_file_name != NULL && font_textures[ font_page ] != NULL )
        return 1;

    //если текстуры нету и передано имя новой
    if( texture_file_name != NULL && font_textures[ font_page ] == NULL )
    {
        font_textures[ font_page ]= new r_Texture;
        font_textures[ font_page ]->Load( texture_file_name );
        font_textures[ font_page ]->SetFiltration( GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST );
    }
    else//если нету не имени, ни новой   текстуры
        return 2;

    FILE* f= fopen( file_name, "rt" );
    if( f == NULL )
        return 3;

    unsigned int tmp;
    fscanf( f, "%u", &tmp );// высота шрифта
    letter_height= (unsigned char) tmp;

    unsigned int number_of_letter_width_variation;

    fscanf( f, "%d", &number_of_letter_width_variation );//количество стор в файле описания шрифта

    unsigned int cuttent_letter_num;
    unsigned char current_letter_width;
    float current_letter_width_f;
    unsigned short letter;
    unsigned short current_tc_x, current_tc_y;
    char temp_buf[16];
    for( unsigned int i=0;  i< number_of_letter_width_variation; i++ )
    {


        fscanf( f, "%u", &cuttent_letter_num );//их количество

         fscanf( f, "%d", &tmp );// ширина букв в этой строке
        current_letter_width= (unsigned char) tmp;

        fscanf( f, "%u", &tmp );//текстурные координаты
        current_tc_x= (unsigned short) tmp;

        fscanf( f, "%u", &tmp );
        current_tc_y= (unsigned short) tmp;

        current_letter_width_f= (float)current_letter_width / (float)letter_height;
        for( unsigned int j= 0; j< cuttent_letter_num; j++ )
        {
            fscanf( f, "%s ", temp_buf );
            if( temp_buf[0] == '0' && ( temp_buf[1] == 'X' || temp_buf[1] == 'x' ) )//если подаётся код символа
            {
                letter= r_Char2Hex( temp_buf + 2 );
            }
            else//иначе - прямое кодирование символа
            {
                temp_buf[0]&= 127;//обрезаем то, чего нет в ASCII
                letter= (unsigned short)temp_buf[0];//только для ASCII символов
            }

            letter_tex_number       [ letter ]=  (unsigned char) font_page;
            letter_tex_coord_bottom [ letter ]= current_tc_y;
            letter_tex_coord_top    [ letter ]= current_tc_y + letter_height;
            letter_tex_coord_left   [ letter ]= current_tc_x;
            letter_tex_coord_right  [ letter ]= current_tc_x + current_letter_width;
            letter_relative_width   [ letter ]= current_letter_width_f;

            current_tc_x+= current_letter_width;
        }
    }
    return 0;
}
#endif//FONT_CPPP

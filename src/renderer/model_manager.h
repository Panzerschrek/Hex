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
#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "ph.h"
#include "fmd_format.h"
#include "polygon_buffer.h"
#include "glsl_program.h"

struct r_Model
{
    fmd_Vertex* vertices;
    quint16* indeces;
    unsigned int vertex_count, index_count;
};
int rLoadModel( r_Model* model, const char* file_name );
void rTransformModel( r_Model* model_dst, const r_Model* model_src, const qint16* mat );
void rShiftModelIndeces( r_Model* model, unsigned short shift );
class r_LocalModelList;


class r_ModelManager
{
public:

enum:
    unsigned char
    {
        PLATE= 0,
        DOOR,
        RABBIT,
        DWARF,
        PICK,
        PILE,
        CLOCK,
        MODEL_COUNT= 40
    };

    void ClearModelList();
    void ReserveData( r_LocalModelList* model_list );
    void AllocateMemory();
    void AddModels( r_LocalModelList* model_list );
    void GenerateInstanceDataBuffer();
    void LoadModels();
    void LoadModels2();
    void DrawModels( const r_GLSLProgram* shader );
    void InitVertexBuffer();


    static unsigned char InitModelTable();
private:
    static unsigned char GetModelId( unsigned char kind, unsigned char direction );
    static unsigned char model_table[];


	unsigned int model_count;
    r_Model models[ MODEL_COUNT ];
    unsigned int vertex_count[ MODEL_COUNT ];//количество вершин в каждой модели
    unsigned int index_count[ MODEL_COUNT ];//количество индексов в каждой модели
    unsigned int index_pointer[ MODEL_COUNT ];//смещение индексов модедей в индексном буффере
    unsigned int vertex_pointer[ MODEL_COUNT ];//смещение вершин моделей
    unsigned int index_buffer_size, vertex_buffer_size;

    short* model_instance_data;
    unsigned int instance_total_model_count;//количество моделей, физически отображаемых
    unsigned int model_buffer_size;//размер model_instance_data
    unsigned int instance_data_pointer[ MODEL_COUNT ];//указатель для каждого типа моделе на место в массиве данных для инстансинга
    unsigned int instance_model_count[ MODEL_COUNT ];//количество моделей каждого типа
    r_PolygonBuffer model_buffer;
    GLuint texture_buffer, tex;
    friend class r_LocalModelList;

    public:

    r_ModelManager()
    {
    	model_count= MODEL_COUNT;
    	model_instance_data= NULL;
        model_buffer_size= 0;
        for( int i=0; i< MODEL_COUNT; i++ )
        {
            instance_model_count[i]= 0;
            models[i].vertex_count= 0;
            models[i].index_count= 0;
        }
    }
};

class r_LocalModelList
{
public:
    void AddModel( unsigned char kind, unsigned char direction, unsigned char packed_light, unsigned char tex_id, short x, short y, short z );
    void Clear();
    void AllocateMemory();
    void SetModelCount( unsigned int n )
    {
        total_model_count= n;
    }
    r_LocalModelList()
    {
        list_size= 0;
        tex_id= NULL;
        coord= NULL;
        packed_light= NULL;
        model_id= NULL;
        total_model_count= 0;
    }

private:
    unsigned int model_count[ r_ModelManager::MODEL_COUNT ];
    unsigned int total_model_count, list_size, pointer;
    quint8* model_id;
    quint8* tex_id;
    qint16* coord;
    quint8* packed_light;

    friend class r_ModelManager;
};




#endif//MODEL_MANAGER_H

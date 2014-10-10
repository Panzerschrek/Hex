#ifndef BLOCK_HPP
#define BLOCK_HPP
#include "hex.hpp"

class h_Block
{
public:

    static h_BlockType GetGetBlockTypeByName( const char* name );
    static h_Direction GetDirectionByName( const char* name );
    h_Block( h_BlockType block_type= AIR, unsigned short additional_data= 0 );
    ~h_Block() {}

    h_BlockType Type();
    h_TransparencyType Transparency();
    unsigned short AdditionalData();

protected:
    h_BlockType type;
    unsigned short additional_data;//for water level\etc.
};


inline h_BlockType h_Block::Type()
{
    return type;
}

inline unsigned short h_Block::AdditionalData()
{
    return additional_data;
}

inline h_Block::h_Block( h_BlockType block_type, unsigned short additional_data )
{
    type= block_type;
    this->additional_data= additional_data;
}



class h_LiquidBlock: public h_Block
{
    /*h_Block::additional_data in this class is water pressure
    */
public:
    h_LiquidBlock();
    h_LiquidBlock( h_BlockType block_type, unsigned short liquid_level );

    unsigned short LiquidLevel() const;
    void SetLiquidLevel( unsigned short l );
    void IncreaseLiquidLevel( unsigned short l );
    void DecreaseLiquidLevel( unsigned short l );

    unsigned char x, y, z, reserved;//relative liquid block coordinates ( in chunk )
};

inline h_LiquidBlock::h_LiquidBlock( h_BlockType block_type, unsigned short liquid_level ):
    h_Block( block_type, liquid_level )  {}
inline h_LiquidBlock::h_LiquidBlock():
    h_Block( WATER, H_MAX_WATER_LEVEL )  {} //water block with pressure = 0 and max water level

inline unsigned short h_LiquidBlock::LiquidLevel() const
{
    return additional_data;
}
inline void h_LiquidBlock::SetLiquidLevel( unsigned short l )
{
    additional_data= l;
}
inline void h_LiquidBlock::IncreaseLiquidLevel( unsigned short l )
{
	additional_data+= l;
}
inline void h_LiquidBlock::DecreaseLiquidLevel( unsigned short l )
{
	additional_data-= l;
}


class h_LightSource : public h_Block
{
	//h_Block::AdditionalData in this class is light power
	public:
	h_LightSource( h_BlockType block_type, unsigned char light_level= H_MAX_FIRE_LIGHT );
	unsigned char LightLevel();
	void SetLightLevel( unsigned char level );

	unsigned char x, y, z, reserved;//relative light source block coordinates ( in chunk )

};

inline h_LightSource::h_LightSource( h_BlockType block_type, unsigned char light_level ):
h_Block( block_type, light_level ){}

inline unsigned char h_LightSource::LightLevel()
{
	return h_Block::additional_data;
}

inline void h_LightSource::SetLightLevel( unsigned char level )
{
	h_Block::additional_data= level;
}



#endif//BLOCK_HPP

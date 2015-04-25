#pragma once
#include "hex.hpp"

class h_Block
{
public:

	static h_BlockType GetGetBlockTypeByName( const char* name );
	static h_Direction GetDirectionByName( const char* name );

	h_Block( h_BlockType type= AIR, unsigned short additional_data= 0 );
	~h_Block() {}

	h_BlockType Type() const;
	h_TransparencyType Transparency() const;
	unsigned short AdditionalData() const;

protected:
	h_BlockType type_;
	unsigned short additional_data_;//for water level\etc.
};

inline h_BlockType h_Block::Type() const
{
	return type_;
}

inline unsigned short h_Block::AdditionalData() const
{
	return additional_data_;
}

inline h_Block::h_Block( h_BlockType type, unsigned short additional_data )
	: type_(type)
	, additional_data_(additional_data)
{
}

class h_LiquidBlock: public h_Block
{
	// h_Block::additional_data_ in this class is water pressure
public:
	h_LiquidBlock();
	h_LiquidBlock( h_BlockType type, unsigned short liquid_level );

	unsigned short LiquidLevel() const;
	void SetLiquidLevel( unsigned short l );
	void IncreaseLiquidLevel( unsigned short l );
	void DecreaseLiquidLevel( unsigned short l );

	unsigned char x_, y_, z_, reserved_; // relative liquid block coordinates ( in chunk )
};

inline h_LiquidBlock::h_LiquidBlock( h_BlockType type, unsigned short liquid_level )
	: h_Block( type, liquid_level )
{
}

inline h_LiquidBlock::h_LiquidBlock()
	: h_Block( WATER, H_MAX_WATER_LEVEL )
{
}

inline unsigned short h_LiquidBlock::LiquidLevel() const
{
	return additional_data_;
}

inline void h_LiquidBlock::SetLiquidLevel( unsigned short l )
{
	additional_data_= l;
}

inline void h_LiquidBlock::IncreaseLiquidLevel( unsigned short l )
{
	additional_data_+= l;
}
inline void h_LiquidBlock::DecreaseLiquidLevel( unsigned short l )
{
	additional_data_-= l;
}


class h_LightSource : public h_Block
{
	//h_Block::AdditionalData in this class is light power
public:
	h_LightSource( h_BlockType type, unsigned char light_level= H_MAX_FIRE_LIGHT );
	unsigned char LightLevel();
	void SetLightLevel( unsigned char level );

	unsigned char x_, y_, z_, reserved_; // relative light source block coordinates ( in chunk )
};

inline h_LightSource::h_LightSource( h_BlockType type, unsigned char light_level )
	: h_Block( type, light_level )
{
}

inline unsigned char h_LightSource::LightLevel()
{
	return h_Block::additional_data_;
}

inline void h_LightSource::SetLightLevel( unsigned char level )
{
	h_Block::additional_data_= level;
}

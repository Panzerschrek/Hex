#pragma once
#include "hex.hpp"
#include "math_lib/fixed.hpp"

class h_Block
{
public:
	static h_BlockType GetGetBlockTypeByName( const char* name );
	static const char* GetBlockName( h_BlockType type );
	static h_Direction GetDirectionByName( const char* name );

	h_Block( h_BlockType type= h_BlockType::Air, unsigned short additional_data= 0 );
	~h_Block() {}

	h_BlockType Type() const;
	h_TransparencyType Transparency() const;
	unsigned short AdditionalData() const;

protected:
	h_BlockType type_;
	unsigned short additional_data_;//for water level\etc.
};

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

class h_LightSource : public h_Block
{
	//h_Block::AdditionalData in this class is light power
public:
	h_LightSource( h_BlockType type, unsigned char light_level= H_MAX_FIRE_LIGHT );
	unsigned char LightLevel() const;
	void SetLightLevel( unsigned char level );

	unsigned char x_, y_, z_, reserved_; // relative light source block coordinates ( in chunk )
};

class h_FailingBlock
{
public:
	h_FailingBlock(
		h_Block* block,
		unsigned char x, unsigned char y, unsigned char z,
		unsigned int failig_start_tick );

	h_Block* GetBlock();
	const h_Block* GetBlock() const;

	void CalcZ( unsigned int tick );

	unsigned char GetX() const;
	unsigned char GetY() const;
	fixed16_t GetZ() const;

private:
	h_Block* block_;
	unsigned char x_, y_;
	unsigned char failing_start_z_;
	unsigned int failig_start_tick_;
	fixed16_t z_;
};

class h_FailingBlockStub : public h_Block
{
public:
	h_FailingBlockStub();

	h_FailingBlock* upper_block;
	h_FailingBlock* lower_block;
};

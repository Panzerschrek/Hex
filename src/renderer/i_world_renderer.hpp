#pragma once

class r_IWorldRenderer
{
public:
	virtual ~r_IWorldRenderer() {}

	virtual void Update()= 0;
	virtual void UpdateChunk( unsigned int X, unsigned int Y, bool immediately= false )= 0;
	virtual void UpdateChunkWater( unsigned int X, unsigned int Y, bool immediately= false )= 0;
	virtual void UpdateWorldPosition( int longitude, int latitude ) = 0;
};

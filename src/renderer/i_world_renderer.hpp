#pragma once

class r_IWorldRenderer
{
public:
	virtual ~r_IWorldRenderer() {}

	virtual void Update()= 0;
	virtual void UpdateChunk( unsigned short X, unsigned short Y )= 0;
	virtual void UpdateChunkWater( unsigned short X, unsigned short Y )= 0;
	virtual void UpdateWorldPosition( int longitude, int latitude ) = 0;
};

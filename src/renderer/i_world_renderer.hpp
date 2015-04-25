#pragma once

class r_IWorldRenderer
{
public:
	virtual ~r_IWorldRenderer() {}

	virtual void Update()= 0;
	virtual void UpdateChunk( unsigned short,  unsigned short )= 0;
	virtual void UpdateChunkWater( unsigned short,  unsigned short )= 0;
	virtual void FullUpdate()= 0;
};

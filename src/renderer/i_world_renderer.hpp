#ifndef I_WORLD_RENDERER_HPP
#define I_WORLD_RENDERER_HPP

class r_IWorldRenderer
{
public:
	virtual ~r_IWorldRenderer(){}

	virtual void Update()= 0;
};

#endif//I_WORLD_RENDERER_HPP
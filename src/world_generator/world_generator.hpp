#pragma once


struct g_WorldGenerationParameters
{
	// Longitude + latitude
	int beginning[2];

	// Size in chunks.
	unsigned int size[2];
};

class g_WorldGenerator
{
public:
	explicit g_WorldGenerator(const g_WorldGenerationParameters& parameters);

	void Generate();
	void DumpDebugResult();

private:

	g_WorldGenerationParameters parameters_;
};

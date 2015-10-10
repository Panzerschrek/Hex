#pragma once
#include <functional>
#include <string>
#include <vector>

#include "../math_lib/fixed.hpp"

struct g_WorldGenerationParameters
{
	std::string world_dir;

	// Size in units.
	unsigned int size[2];

	// Logarithm of generation cell in chunks.
	unsigned int cell_size_log2;

	unsigned int seed;
};

struct g_TreePlantingPoint
{
	// Negative coordinates means, that point does not exist.
	int x;
	int y;
};

class g_WorldGenerator
{
public:
	// Callback for trees planting.
	// Coordinates - world absolute
	typedef std::function< void( int x, int y) > PlantTreeCallback;

	explicit g_WorldGenerator(const g_WorldGenerationParameters& parameters);

	void Generate();
	void DumpDebugResult();

	unsigned char GetGroundLevel( int x, int y ) const;
	unsigned char GetSeaLevel() const;

	void PlantTreesForChunk( int longitude, int latitude, const PlantTreeCallback& plant_tree_callback ) const;

private:

	enum class Biome : unsigned char
	{
		Sea= 0,
		ContinentalShelf,
		SeaBeach,
		Plains,
		Foothills,
		Mountains,
		LastBiome,
	};

	// For debugging.
	static const unsigned char c_biomes_colors_[ size_t(Biome::LastBiome) * 4 ];

	// Amplitude of additional noise, in blocks.
	static const fixed8_t c_biomes_noise_amplitude_[ size_t(Biome::LastBiome) ];

	void BuildPrimaryHeightmap();
	void BuildSecondaryHeightmap();
	void BuildBiomesMap();
	void BuildNoiseAmplitudeMap();
	void GenTreePlantingMatrix();

	// returns interpolated heightmap value * 256
	fixed8_t HeightmapValueInterpolated( int x, int y, fixed8_t& out_noise_amplitude ) const;

private:
	g_WorldGenerationParameters parameters_;

	// Heightmaps - stores as fixed 8.8 values.

	std::vector<unsigned short> primary_heightmap_;
	const fixed8_t primary_heightmap_sea_level_             =  44 << 8;
	const fixed8_t primary_heightmap_mountains_bottom_level_= 144 << 8;

	std::vector<unsigned short> secondary_heightmap_;
	const fixed8_t secondary_heightmap_sea_bottom_level_     = 16 << 8;
	const fixed8_t secondary_heightmap_sea_level_            = 44 << 8;
	const fixed8_t secondary_heightmap_mountain_bottom_level_= 56 << 8;
	const fixed8_t secondary_heightmap_mountain_top_level_   = 88 << 8;

	std::vector<Biome> biomes_map_;
	std::vector<fixed8_t> noise_amplitude_map_;

	struct
	{
		std::vector<g_TreePlantingPoint> points_grid_;
		unsigned int size[2]; // Must be power of two.
		unsigned int grid_size[2];
		unsigned int grid_cell_size;
	} tree_planting_matrix_;
};

#pragma once
#include <functional>
#include <string>
#include <vector>

struct g_WorldGenerationParameters
{
	std::string world_dir;

	// Size in units.
	unsigned int size[2];
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

	enum class Biome
	{
		Sea= 0,
		ContinentalShelf,
		SeaBeach,
		Plains,
		Mountains,
		LastBiome,
	};

	// For debugging.
	static const unsigned char c_biomes_colors_[ size_t(Biome::LastBiome) * 4 ];

	void BuildPrimaryHeightmap();
	void BuildSecondaryHeightmap();
	void BuildBiomesMap();
	void GenTreePlantingMatrix();

	// returns interpolated heightmap value * 256
	unsigned int HeightmapValueInterpolated( int x, int y ) const;

private:
	g_WorldGenerationParameters parameters_;

	std::vector<unsigned char> primary_heightmap_;
	const unsigned char primary_heightmap_sea_level_= 44;

	std::vector<unsigned char> secondary_heightmap_;
	const unsigned char secondary_heightmap_sea_level_= 48;
	const unsigned char secondary_heightmap_sea_bottom_level_= 20;
	const unsigned char secondary_heightmap_mountain_top_level_= 110;

	std::vector<Biome> biomes_map_;

	struct
	{
		std::vector<g_TreePlantingPoint> points_grid_;
		unsigned int size[2]; // Must be power of two.
		unsigned int grid_size[2];
		unsigned int grid_cell_size;
	} tree_planting_matrix_;
};

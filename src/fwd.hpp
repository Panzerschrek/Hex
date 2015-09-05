#pragma once
#include <memory>

class h_Settings;
typedef std::shared_ptr<h_Settings> h_SettingsPtr;

class h_MainLoop;
class h_Player;

class h_World;
class h_Chunk;
class h_ChunkLoader;

class r_WorldRenderer;
class r_IWorldRenderer;
class r_ChunkInfo;
class r_Text;

class ui_Painter;
class ui_MainMenu;

// Blocks classes
class h_Block;
class h_LiquidBlock;
class h_LightSource;

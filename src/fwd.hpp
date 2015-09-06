#pragma once
#include <memory>

class h_Settings;
typedef std::shared_ptr<h_Settings> h_SettingsPtr;

class h_MainLoop;
class h_Player;
typedef std::shared_ptr<h_Player> h_PlayerPtr;
typedef std::weak_ptr<h_Player> h_PlayerWeakPtr;

class h_World;
typedef std::shared_ptr<h_World> h_WorldPtr;
typedef std::shared_ptr<const h_World> h_WorldConstPtr;
class h_Chunk;
class h_ChunkLoader;

class r_IWorldRenderer;
typedef std::shared_ptr<r_IWorldRenderer> r_IWorldRendererPtr;
typedef std::weak_ptr<r_IWorldRenderer> r_IWorldRendererWeakPtr;
class r_WorldRenderer;
typedef std::shared_ptr<r_WorldRenderer> r_WorldRendererPtr;

class r_IWorldRenderer;
class r_ChunkInfo;
class r_Text;

class ui_Painter;
class ui_MainMenu;

// Blocks classes
class h_Block;
class h_LiquidBlock;
class h_LightSource;
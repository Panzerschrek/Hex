#pragma once
#include <functional>
#include <memory>

class h_Settings;
typedef std::shared_ptr<h_Settings> h_SettingsPtr;

class h_MainLoop;
class h_Player;
typedef std::shared_ptr<h_Player> h_PlayerPtr;
typedef std::shared_ptr<const h_Player> h_PlayerConstPtr;
typedef std::weak_ptr<h_Player> h_PlayerWeakPtr;

typedef std::function<void( float relative_progress )> h_LongLoadingCallback;

class h_World;
typedef std::shared_ptr<h_World> h_WorldPtr;
typedef std::shared_ptr<const h_World> h_WorldConstPtr;
class h_Chunk;
class h_ChunkLoader;
struct h_WorldHeader;
typedef std::shared_ptr<h_WorldHeader> h_WorldHeaderPtr;

struct p_WorldPhysMesh;
typedef std::shared_ptr<p_WorldPhysMesh> p_WorldPhysMeshPtr;
typedef std::shared_ptr<const p_WorldPhysMesh> p_WorldPhysMeshConstPtr;

class g_WorldGenerator;

class r_IWorldRenderer;
typedef std::shared_ptr<r_IWorldRenderer> r_IWorldRendererPtr;
typedef std::weak_ptr<r_IWorldRenderer> r_IWorldRendererWeakPtr;
class r_WorldRenderer;
typedef std::shared_ptr<r_WorldRenderer> r_WorldRendererPtr;

class r_ChunkInfo;
typedef std::unique_ptr<r_ChunkInfo> r_ChunkInfoPtr;

class r_WorldVBOCluster;
typedef std::shared_ptr<r_WorldVBOCluster> r_WorldVBOClusterPtr;
class r_WorldVBOClusterGPU;
typedef std::unique_ptr<r_WorldVBOClusterGPU> r_WorldVBOClusterGPUPtr;
class r_WVB;

class r_WeatherEffectsParticleManager;

class r_Text;

class ui_Painter;
class ui_MenuBase;

// Blocks classes
class h_Block;
class h_LiquidBlock;
class h_LightSource;
class h_FailingBlock;

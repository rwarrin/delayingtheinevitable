#ifndef DTI_H

/** TODO(rick):
 * - Gameplay
 *   - Game Over
 *     - Player falls in hole
 *     - Player touches Enemy
 *
 * - Game
 *   - Timer for time survived?
 *
 * - Renderer
 *
 * - Assets
 *
 * - Platform
 *   - Audio
 **/

enum texture_id
{
    Texture_None,

    Texture_Test,
    Texture_TestAlpha,

    Texture_DudeFacingNorth,
    Texture_DudeFacingSouth,
    Texture_DudeFacingEast,
    Texture_DudeFacingWest,

    Texture_Enemy,

    Texture_Tile01,

    Texture_Lava01,

    Texture_Logo,
    Texture_TextPlay,
    Texture_TextExit,

    Texture_Count,
};

static char *TextureFileNames[] =
{
    0,

    "structured_art.bmp",
    "structured_art_alpha.bmp",

    "dude32_north.bmp",
    "dude32_south.bmp",
    "dude32_east.bmp",
    "dude32_west.bmp",

    "enemy32.bmp",

    "tile32_01.bmp",

    "tile32_lava_01.bmp",

    "logo.bmp",
    "play.bmp",
    "exit.bmp",
};

struct bitmap
{
    s32 Width;
    s32 Height;

    u32 RedShift;
    u32 GreenShift;
    u32 BlueShift;
    u32 AlphaShift;

    u8 *Data;
};

enum tile_cell_type
{
    TileCell_Open,
    TileCell_Closed,

    TileCell_Count,
};

struct tile_cell
{
    tile_cell_type CellType;
    texture_id TextureID;
    v2 P;
};

struct world_map
{
    u32 OpenTileCount;
    u16 *OpenTiles;

    u32 TileCount;
    tile_cell *Tiles;

    v2 GridTileDim;
    v2 GridDim;
};

struct entity
{
    v2 P;
    v2 dP;
    texture_id TextureID;

    f32 UpdateTimeDelay;
    f32 TimeSinceLastUpdate;
};

enum game_mode
{
    GameMode_None,

    GameMode_MainMenu,
    GameMode_PlayGame,

    GameMode_Count
};

struct game_state
{
    memory_arena PermanentArena;

    entity *Player;

    u32 EnemyCount;
    entity Enemies[8];

    v2 ScreenDim;
    world_map *WorldMap;

    game_mode GameMode;
    s32 MainMenuHotIndex;
    s32 MainMenuItemCount;

    b32 BeginNewGame;

    b32 IsInitialized;
};

struct transient_state
{
    memory_arena TransientArena;

    u32 TextureCount;
    bitmap *Textures[32];

    b32 IsInitialized;
};

#define DTI_H
#endif

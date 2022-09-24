#include "dti.h"
#include "dti_renderer.cpp"
#include "dti_render_group.h"

#include "dti_render_group.cpp"

static platform_api *GlobalPlatformAPI;

inline u32
GetPixelShiftAmount(u32 Mask)
{
    unsigned long Index;
    _BitScanForward(&Index, Mask);
    return((u32)Index);
}

static bitmap *
LoadBitmapFromFile(memory_arena *Arena, char *FileName)
{
    bitmap *Result = 0;

    platform_file_result FileResult = GlobalPlatformAPI->PlatformReadFile(FileName);
    if(FileResult.Size)
    {
        bitmap_header *BitmapHeader = (bitmap_header *)FileResult.Data;

        Result = PushStruct(Arena, bitmap);
        Result->Width = BitmapHeader->InfoHeader.Width;
        Result->Height = Absolute(BitmapHeader->InfoHeader.Height);
        Result->RedShift = GetPixelShiftAmount(BitmapHeader->InfoHeader.RedMask);
        Result->GreenShift = GetPixelShiftAmount(BitmapHeader->InfoHeader.GreenMask);
        Result->BlueShift = GetPixelShiftAmount(BitmapHeader->InfoHeader.BlueMask);
        Result->AlphaShift = GetPixelShiftAmount(~(BitmapHeader->InfoHeader.RedMask |
                                                   BitmapHeader->InfoHeader.GreenMask |
                                                   BitmapHeader->InfoHeader.BlueMask));
        Result->Data = (u8 *)PushSize(Arena, BitmapHeader->InfoHeader.ImageSize);
        Copy(FileResult.Data + BitmapHeader->Header.DataOffset,
             Result->Data,
             BitmapHeader->InfoHeader.ImageSize);
    }

    return(Result);
}

static void
RemoveTile(world_map *WorldMap, u32 IndexToRemove)
{
    tile_cell *Tile = WorldMap->Tiles + IndexToRemove;
    Tile->CellType = TileCell_Closed;
    Tile->TextureID = Texture_Lava01;
    WorldMap->OpenTiles[IndexToRemove] = WorldMap->OpenTiles[WorldMap->OpenTileCount--];
}

inline bitmap *
GetBitmap(transient_state *TransientState, texture_id TextureID)
{
    bitmap *Result = TransientState->Textures[Texture_None];
    if((TextureID >= 0) && (TextureID < TransientState->TextureCount))
    {
        Result = TransientState->Textures[TextureID];
    }
    return(Result);
}

inline v2
WorldToGridPosition(world_map *WorldMap, v2 Coords)
{
    v2 Result = {(f32)((s32)(Coords.x / WorldMap->GridTileDim.w)),
                 (f32)((s32)(Coords.y / WorldMap->GridTileDim.h))};
    return(Result);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    GlobalPlatformAPI = &Memory->PlatformAPI;
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        GlobalPRNGState = 1048217;

        InitializeArena(&GameState->PermanentArena,
                        (u8 *)Memory->PermanentStorage + sizeof(*GameState),
                        Memory->PermanentStorageSize - sizeof(*GameState));

        GameState->ScreenDim = V2(1280, 720);

        GameState->WorldMap = PushStruct(&GameState->PermanentArena, world_map);
        GameState->WorldMap->GridTileDim = V2(32, 32);
        GameState->WorldMap->GridDim = V2(Ceil(GameState->ScreenDim.w / GameState->WorldMap->GridTileDim.w),
                                          Ceil(GameState->ScreenDim.h / GameState->WorldMap->GridTileDim.h));
        GameState->WorldMap->TileCount = GameState->WorldMap->GridDim.w * GameState->WorldMap->GridDim.h;
        GameState->WorldMap->Tiles = PushArray(&GameState->PermanentArena, GameState->WorldMap->TileCount, tile_cell);
        GameState->WorldMap->OpenTiles = PushArray(&GameState->PermanentArena, GameState->WorldMap->TileCount, u16);

        GameState->GameMode = GameMode_MainMenu;

        GameState->IsInitialized = true;
    }

    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    if(!TransientState->IsInitialized)
    {
        InitializeArena(&TransientState->TransientArena,
                        (u8 *)Memory->TransientStorage + sizeof(*TransientState),
                        Memory->TransientStorageSize - sizeof(*TransientState));

        Assert(ArrayCount(TextureFileNames) == Texture_Count);
        Assert(Texture_Count < ArrayCount(TransientState->Textures));
        for(u32 TextureIndex = 0; TextureIndex < Texture_Count; ++TextureIndex)
        {
            bitmap **Texture = TransientState->Textures + TextureIndex;
            if(TextureIndex == Texture_None)
            {
                *Texture = PushStruct(&TransientState->TransientArena, bitmap);
                (*Texture)->Width = 0;
                (*Texture)->Height = 0;
                (*Texture)->Data = 0;
            }
            else
            {
                *Texture = LoadBitmapFromFile(&TransientState->TransientArena, TextureFileNames[TextureIndex]);
            }
            ++TransientState->TextureCount;
        }

        TransientState->IsInitialized = true;
    }

    if(GameState->GameMode == GameMode_PlayGame)
    {
        if(GameState->BeginNewGame)
        {
            u16 *TileState = GameState->WorldMap->OpenTiles;
            tile_cell *Tile = GameState->WorldMap->Tiles;
            for(u32 TileY = 0; TileY < GameState->WorldMap->GridDim.y; ++TileY)
            {
                for(u32 TileX = 0; TileX < GameState->WorldMap->GridDim.x; ++TileX)
                {
                    *TileState++ = GameState->WorldMap->OpenTileCount++;

                    Tile->P = V2(TileX, TileY);
                    Tile->CellType = TileCell_Open;
                    Tile->TextureID = (texture_id)(Texture_Tile01);
                    ++Tile;
                }
            }

            GameState->Player = PushStruct(&GameState->PermanentArena, entity);
            GameState->Player->P = GameState->WorldMap->GridDim * 0.5f;
            GameState->Player->TextureID = Texture_DudeFacingNorth;

            for(u32 EnemyIndex = 0; EnemyIndex < MIN(ArrayCount(GameState->Enemies), 1); ++EnemyIndex)
            {
                entity *Enemy = GameState->Enemies + GameState->EnemyCount++;
                Enemy->P = V2(0, 0);
                Enemy->TextureID = Texture_Enemy;
            }

            GameState->BeginNewGame = false;
        }

        f32 MovedP = 7.5f * Input->dt;
        entity *Player = GameState->Player;
        if(IsDown(Input->Keyboard_A))
        {
            GameState->Player->TextureID = Texture_DudeFacingWest;
            Player->dP.x -= MovedP;
        }
        if(IsDown(Input->Keyboard_D))
        {
            GameState->Player->TextureID = Texture_DudeFacingEast;
            Player->dP.x += MovedP;
        }
        if(IsDown(Input->Keyboard_W))
        {
            GameState->Player->TextureID = Texture_DudeFacingNorth;
            Player->dP.y -= MovedP;
        }
        if(IsDown(Input->Keyboard_S))
        {
            GameState->Player->TextureID = Texture_DudeFacingSouth;
            Player->dP.y += MovedP;
        }

        v2 PlayerdPGridCoord = V2((f32)((s32)Player->P.x + (s32)Player->dP.x),
                                  (f32)((s32)Player->P.y + (s32)Player->dP.y));
        b32 PlayerMoved = ((Player->P.x != PlayerdPGridCoord.x) ||
                           (Player->P.y != PlayerdPGridCoord.y));
        b32 PlayerPrecisionMove = false;
        if(PlayerMoved)
        {
            Player->P = PlayerdPGridCoord;
        }
        else
        {
            if(WasPressed(Input->Keyboard_A))
            {
                GameState->Player->TextureID = Texture_DudeFacingWest;
                --Player->P.x;
                PlayerPrecisionMove = true;
            }
            if(WasPressed(Input->Keyboard_D))
            {
                GameState->Player->TextureID = Texture_DudeFacingEast;
                ++Player->P.x;
                PlayerPrecisionMove = true;
            }
            if(WasPressed(Input->Keyboard_W))
            {
                GameState->Player->TextureID = Texture_DudeFacingNorth;
                --Player->P.y;
                PlayerPrecisionMove = true;
            }
            if(WasPressed(Input->Keyboard_S))
            {
                GameState->Player->TextureID = Texture_DudeFacingSouth;
                ++Player->P.y;
                PlayerPrecisionMove = true;
            }
        }

        if(PlayerMoved || PlayerPrecisionMove)
        {
            Player->dP.x = Player->dP.y = 0;
            u32 IndexToRemove = GetNextRandomNumber() % GameState->WorldMap->OpenTileCount;
            RemoveTile(GameState->WorldMap, IndexToRemove);
        }

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransientState->TransientArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransientState->TransientArena, Kilobytes(128));

        PushClear(RenderGroup, V4(1, 0, 1, 1));

        tile_cell *TileToRender = GameState->WorldMap->Tiles;
        for(u32 TileY = 0; TileY < GameState->WorldMap->GridDim.y; ++TileY)
        {
            for(u32 TileX = 0; TileX < GameState->WorldMap->GridDim.x; ++TileX)
            {
                if(TileToRender->CellType == TileCell_Open)
                {
                    PushBitmap(RenderGroup, GetBitmap(TransientState, TileToRender->TextureID),
                               V2(TileToRender->P.x * GameState->WorldMap->GridTileDim.w,
                                  TileToRender->P.y * GameState->WorldMap->GridTileDim.h),
                               V4(1, 1, 1, 1));
                }
                else if(TileToRender->CellType == TileCell_Closed)
                {
                    PushBitmap(RenderGroup, GetBitmap(TransientState, TileToRender->TextureID),
                               V2(TileToRender->P.x * GameState->WorldMap->GridTileDim.w,
                                  TileToRender->P.y * GameState->WorldMap->GridTileDim.h),
                               V4(1, 1, 1, 1));
                }
                ++TileToRender;
            }
        }

        for(u32 EnemyIndex = 0; EnemyIndex < GameState->EnemyCount; ++EnemyIndex)
        {
            entity *Enemy = GameState->Enemies + EnemyIndex;

            // TODO(rick): do we even need this garbage anymore?
            Enemy->TimeSinceLastUpdate += Input->dt;
            if(Enemy->TimeSinceLastUpdate >= Enemy->UpdateTimeDelay)
            {
                Enemy->TimeSinceLastUpdate = 0;
                v2 MovementVector = GameState->Player->P - Enemy->P;
                MovementVector = Normalize(MovementVector) * Input->dt * 3;
                Enemy->P += MovementVector;
            }

            PushBitmap(RenderGroup, GetBitmap(TransientState, Enemy->TextureID),
                       V2(Enemy->P.x * GameState->WorldMap->GridTileDim.w,
                          Enemy->P.y * GameState->WorldMap->GridTileDim.h),
                       V4(1, 1, 1, 1));
        }

        PushBitmap(RenderGroup, GetBitmap(TransientState, GameState->Player->TextureID),
                   V2(GameState->Player->P.x * GameState->WorldMap->GridTileDim.w,
                      GameState->Player->P.y * GameState->WorldMap->GridTileDim.h),
                   V4(1, 1, 1, 1));

        PushRect(RenderGroup, V2(10, 10), V2(100, 100), V4(0, 1, 0, 0.1));

        RenderGroupToOutput(RenderGroup, FrameBuffer);
        EndTemporaryMemory(RenderMemory);
        ValidateArena(&TransientState->TransientArena);
    }
    else if(GameState->GameMode == GameMode_MainMenu)
    {
        GameState->MainMenuItemCount = 2;
        if(WasPressed(Input->Keyboard_ArrowUp))
        {
            ++GameState->MainMenuHotIndex;
            if(GameState->MainMenuHotIndex >= GameState->MainMenuItemCount)
            {
                GameState->MainMenuHotIndex = 0;
            }
        }
        else if(WasPressed(Input->Keyboard_ArrowDown))
        {
            --GameState->MainMenuHotIndex;
            if(GameState->MainMenuHotIndex < 0)
            {
                GameState->MainMenuHotIndex = GameState->MainMenuItemCount - 1;
            }
        }

        if(WasPressed(Input->Keyboard_Return))
        {
            switch(GameState->MainMenuHotIndex)
            {
                case 0:
                {
                    GameState->MainMenuHotIndex = 0;
                    GameState->GameMode = GameMode_PlayGame;
                    GameState->BeginNewGame = true;
                } break;
                case 1:
                {
                    Memory->GameWantsExit = true;
                } break;
            }
        }

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransientState->TransientArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransientState->TransientArena, Kilobytes(128));

        PushClear(RenderGroup, V4(0, 0, 0, 1));

        PushBitmap(RenderGroup, GetBitmap(TransientState, Texture_Logo), V2(10, 10), V4(1, 1, 1, 1));

        PushBitmap(RenderGroup, GetBitmap(TransientState, Texture_TextPlay), V2(320, 300), ((GameState->MainMenuHotIndex == 0) ? V4(1, 0, 0, 1) : V4(1, 1, 1, 1)));
        PushBitmap(RenderGroup, GetBitmap(TransientState, Texture_TextExit), V2(320, 332), ((GameState->MainMenuHotIndex == 1) ? V4(1, 0, 0, 1) : V4(1, 1, 1, 1)));

        RenderGroupToOutput(RenderGroup, FrameBuffer);
        EndTemporaryMemory(RenderMemory);
        ValidateArena(&TransientState->TransientArena);
    }
}

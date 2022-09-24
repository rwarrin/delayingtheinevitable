#include <intrin.h>
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef s8 b32;
typedef float f32;
typedef double f64;
typedef size_t mem_size;
typedef intptr_t smm;
typedef uintptr_t umm;

#define MIN(A, B) ((A) < (B) ? A : B)
#define MAX(A, B) ((A) > (B) ? A : B)

#define Assert(Condition) do { if(!(Condition)) { *(s32 *)0 = 0; } } while(0)
#define InvalidDefaultCase default: { Assert(!"Invalid Default Case"); } break
#define InvalidCodePath Assert(!"Invalid Code Path")

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma pack(push, 1)
struct bitmap_header
{
    struct
    {
        u16 Signature;
        u32 FileSize;
        u16 Reserved0;
        u16 Reserved1;
        u32 DataOffset;
    } Header;
    struct
    {
        u32 HeaderSize;
        s32 Width;
        s32 Height;
        u16 Planes;
        u16 BitsPerPixel;
        u32 Compression;
        u32 ImageSize;
        u32 XPelsPerMeter;
        u32 YPelsPerMeter;
        u32 ColorsUsed;
        u32 ColorsImportant;
        u32 RedMask;
        u32 GreenMask;
        u32 BlueMask;
        u32 AlphaMask;
    } InfoHeader;
};
#pragma pack(pop)

struct frame_buffer
{
    s32 Width;
    s32 Height;
    u8 *Data;
};

struct platform_file_result
{
    u32 Size;
    u8 *Data;
};

#define PLATFORM_READ_FILE(name) platform_file_result name(char *FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FREE_FILE(name) void name(platform_file_result *File)
typedef PLATFORM_FREE_FILE(platform_free_file);

struct platform_api
{
    platform_read_file *PlatformReadFile;
    platform_free_file *PlatformFreeFile;
};

struct platform_memory
{
    u32 PermanentStorageSize;
    void *PermanentStorage;

    u32 TransientStorageSize;
    void *TransientStorage;

    platform_api PlatformAPI;

    b32 GameWantsExit;

    u64 TicksElapsed;
    u64 MSElapsed;
    u64 Frequency;
};

#define Kilobytes(Size) (Size * 1024LL)
#define Megabytes(Size) (Kilobytes(Size) * 1024LL)
#define Gigabytes(Size) (Megabytes(Size) * 1024LL)
#define Terabytes(Size) (Gigabytes(Size) * 1024LL)

struct memory_arena
{
    u8 *Base;
    u32 Used;
    u32 Size;

    s32 TempCount;
};

struct temporary_memory
{
    u32 Used;
    memory_arena *Arena;
};

static void
InitializeArena(memory_arena *Arena, void *Base, u32 Size)
{
    Assert(Arena);
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
    Arena->Size = Size;
    Arena->TempCount = 0;
}

inline void
ValidateArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

static temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result = {0};
    Result.Arena = Arena;
    Result.Used = Arena->Used;
    ++Arena->TempCount;
    return(Result);
}

static void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Arena->Used = TempMem.Used;
    --Arena->TempCount;
}

#define PushStruct(Arena, Type, ...) (Type *)PushSize_(Arena, sizeof(Type), ## __VA_ARGS__)
#define PushArray(Arena, Count, Type, ...) (Type *)PushSize_(Arena, sizeof(Type)*Count, ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

static void *
PushSize_(memory_arena *Arena, u32 Size, u32 Alignment = 4)
{
    // NOTE(rick): Alignment must be a power of 2
    Assert((Alignment & (Alignment - 1)) == 0);

    void *Result = 0;

    umm ResultPointer = (umm)(Arena->Base + Arena->Used);
    umm AlignmentOffset = 0;
    umm AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
        Size += (u32)AlignmentOffset;
    }

    Assert((Arena->Used + Size) <= Arena->Size);
    if((Arena->Used + Size) <= Arena->Size)
    {
        Result = (void *)(ResultPointer + AlignmentOffset);
        Arena->Used += Size;
    }

    return(Result);
}

struct button_state
{
    b32 IsDown;
    u32 TransitionCount;
};

struct platform_input
{
    f32 dt;

    f32 MouseX;
    f32 MouseY;
    f32 MouseZ;

    union
    {
        button_state MouseButtons[6];
        struct
        {
            button_state MouseButton_Left;
            button_state MouseButton_Right;
            button_state MouseButton_Unused;
            button_state MouseButton_Middle;
            button_state MouseButton_Button4;
            button_state MouseButton_Button5;
        };
    };

    union
    {
        button_state KeyboardButtons[9];
        struct
        {
            button_state Keyboard_A;
            button_state Keyboard_D;
            button_state Keyboard_S;
            button_state Keyboard_W;

            button_state Keyboard_Escape;
            button_state Keyboard_Space;
            button_state Keyboard_Return;

            button_state Keyboard_ArrowUp;
            button_state Keyboard_ArrowDown;
        };
    };
};

#define WasPressed(Button) (!Button.IsDown && (Button.TransitionCount != 0))
#define IsDown(Button) (Button.IsDown)

static u32 GlobalPRNGState;
inline u32
GetNextRandomNumber()
{
    u32 NewState = GlobalPRNGState;
    NewState ^= NewState >> 13;
    NewState ^= NewState << 17;
    NewState ^= NewState << 5;
    GlobalPRNGState = NewState;
    return(NewState);
}

inline void
Copy(void *SrcIn, void *DestIn, u32 Size)
{
    u8 *Src = (u8 *)SrcIn;
    u8 *Dest = (u8 *)DestIn;
    while(Size--)
    {
        *Dest++ = *Src++;
    }
}

#define ZeroStruct(Struct) ZeroSize(Struct, sizeof(Struct))
#define ZeroArray(Array) ZeroSize(Array, sizeof(Array))
inline void
ZeroSize(void *SrcIn, u32 Size)
{
    u8 *Src = (u8 *)SrcIn;
    while(Size--)
    {
        *Src++ = 0;
    }
}

#define GAME_UPDATE_AND_RENDER(name) void name(frame_buffer *FrameBuffer, platform_memory *Memory, platform_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);


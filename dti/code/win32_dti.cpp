#include "dti_platform.h"
#include "dti_math.h"

#include "dti.cpp"

#include "win32_dti.h"
#include <stdio.h>
#include <windows.h>

static b32 GlobalRunning;
static win32_back_buffer GlobalBackBuffer;

static PLATFORM_READ_FILE(Win32PlatformReadFile)
{
    platform_file_result Result = {0};

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ,
                                    0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(FileHandle)
    {
        Result.Size = GetFileSize(FileHandle, 0);
        Result.Data = (u8 *)VirtualAlloc(0, Result.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

        if(Result.Data)
        {
            DWORD BytesRead = 0;
            if(ReadFile(FileHandle, Result.Data, Result.Size, &BytesRead, 0) &&
               BytesRead == Result.Size)
            {
                // TODO(rick): Do something?
            }
            else
            {
                VirtualFree(Result.Data, 0, MEM_RELEASE);
                Result.Size = 0;
                Result.Data = 0;
            }
        }
        else
        {
            Result.Size = 0;
            Result.Data = 0;
        }

        CloseHandle(FileHandle);
    }

    return(Result);
}

static PLATFORM_FREE_FILE(Win32PlatformFreeFile)
{
    if(File)
    {
        File->Size = 0;
        VirtualFree(File->Data, 0, MEM_RELEASE);
        File->Data = 0;
    }
}

static win32_rect_dim
Win32GetClientRect(HWND Window)
{
    RECT ClientRect = {0};
    GetClientRect(Window, &ClientRect);

    win32_rect_dim Result = {0};
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

static void
Win32ResizeDIBSection(win32_back_buffer *BackBuffer, s32 Width, s32 Height)
{
    if(BackBuffer->Data)
    {
        VirtualFree(BackBuffer->Data, 0, MEM_RELEASE);
    }

    BackBuffer->BitmapHeader.InfoHeader.HeaderSize = sizeof(BackBuffer->BitmapHeader.InfoHeader);
    BackBuffer->BitmapHeader.InfoHeader.Width = Width;
    BackBuffer->BitmapHeader.InfoHeader.Height = -Height; // NOTE(rick): Bottom up bitmaps
    BackBuffer->BitmapHeader.InfoHeader.Planes = 1;
    BackBuffer->BitmapHeader.InfoHeader.BitsPerPixel = 32;
    BackBuffer->BitmapHeader.InfoHeader.Compression = BI_RGB;

    BackBuffer->Width = Width;
    BackBuffer->Height = Height;

    u32 BitsPerPixel = 32;
    u32 DataSize = BitsPerPixel * Width * Height;
    BackBuffer->Data = (u8 *)VirtualAlloc(0, DataSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(BackBuffer->Data);
}

static void
Win32DisplayBufferInWindow(win32_back_buffer *BackBuffer, HDC DeviceContext)
{
    StretchDIBits(DeviceContext,
                  0, 0, BackBuffer->Width, BackBuffer->Height,
                  0, 0, BackBuffer->Width, BackBuffer->Height,
                  (void *)BackBuffer->Data,
                  (BITMAPINFO *)&BackBuffer->BitmapHeader.InfoHeader,
                  DIB_RGB_COLORS, SRCCOPY);
}

static void
Win32ProcessKeyboardInput(button_state *ButtonState, b32 IsDown)
{
    if(ButtonState->IsDown)
    {
        ++ButtonState->TransitionCount;
    }

    ButtonState->IsDown = IsDown;
}

static void
Win32ProcessPendingMessages(platform_input *Input)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                u32 KeyCode = (u32)Message.wParam;
                b32 IsDown = (b32)((Message.lParam & (1 << 31)) == 0);
                b32 WasDown = (b32)((Message.lParam & (1 << 30)) != 0);

                if(IsDown != WasDown)
                {
                    if(KeyCode == 'A')      { Win32ProcessKeyboardInput(&Input->Keyboard_A, IsDown); }
                    else if(KeyCode == 'D') { Win32ProcessKeyboardInput(&Input->Keyboard_D, IsDown); }
                    else if(KeyCode == 'S') { Win32ProcessKeyboardInput(&Input->Keyboard_S, IsDown); }
                    else if(KeyCode == 'W') { Win32ProcessKeyboardInput(&Input->Keyboard_W, IsDown); }
                    else if(KeyCode == VK_ESCAPE) { Win32ProcessKeyboardInput(&Input->Keyboard_Escape, IsDown); }
                    else if(KeyCode == VK_SPACE) { Win32ProcessKeyboardInput(&Input->Keyboard_Space, IsDown); }
                    else if(KeyCode == VK_RETURN) { Win32ProcessKeyboardInput(&Input->Keyboard_Return, IsDown); }
                    else if(KeyCode == VK_UP) { Win32ProcessKeyboardInput(&Input->Keyboard_ArrowUp, IsDown); }
                    else if(KeyCode == VK_DOWN) { Win32ProcessKeyboardInput(&Input->Keyboard_ArrowDown, IsDown); }
#if 0
                    if(IsPressed)
                    {
                        OutputDebugStringA("IsPressed\n");
                    }
                    else if(IsHeld)
                    {
                        OutputDebugStringA("IsHeld\n");
                    }
#endif
                }
            } break;
            case WM_MOUSEWHEEL:
            {
                s16 Delta = HIWORD(Message.wParam);
                s32 ScrollAmount = Delta / WHEEL_DELTA;
                Input->MouseZ = ScrollAmount;
            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

static LRESULT CALLBACK
Win32WindowsCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        case WM_SIZE:
        {
            s32 Height = HIWORD(LParam);
            s32 Width = LOWORD(LParam);

            // TODO(rick): Get rid of this eventually the client rect
            // width/height actually come from the LParam!
            win32_rect_dim ClientRect = Win32GetClientRect(Window);
            Assert(Height == ClientRect.Height);
            Assert(Width == ClientRect.Width);

            Win32ResizeDIBSection(&GlobalBackBuffer, Width, Height);
        } break;
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

int
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    WNDCLASSEXA WindowClass = {0};
    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowsCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "win32_dti";

    if(!RegisterClassExA(&WindowClass))
    {
        MessageBox(0, "Failed to register window class.", "Error", MB_OK);
        return(-1);
    }

    HWND Window = CreateWindowExA(0,
                                  WindowClass.lpszClassName, "Delaying the inevitable",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  1280, 720,
                                  0, 0, Instance, 0);

    if(!Window)
    {
        MessageBox(0, "Failed to create window.", "Error", MB_OK);
        return(-2);
    }

    if(timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        MessageBox(0, "Failed to get high res timer.", "Error", MB_OK);
    }

    f32 TargetFPS = 60.0f;
    f32 TargetSecondsPerFrame = 1.0f / 60.0f;
    f32 TargetMSPerFrame = TargetSecondsPerFrame * 1000;

    platform_memory PlatformMemory = {0};
    PlatformMemory.PermanentStorageSize = Megabytes(1);
    PlatformMemory.TransientStorageSize = Megabytes(16);
    void *PlatformStorage = VirtualAlloc((void *)Terabytes(2),
                                         PlatformMemory.PermanentStorageSize + PlatformMemory.TransientStorageSize,
                                         MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    PlatformMemory.PermanentStorage = (void *)((u8 *)PlatformStorage + 0);
    PlatformMemory.TransientStorage = (void *)((u8 *)PlatformStorage + PlatformMemory.PermanentStorageSize);

    PlatformMemory.PlatformAPI.PlatformReadFile = Win32PlatformReadFile;
    PlatformMemory.PlatformAPI.PlatformFreeFile = Win32PlatformFreeFile;

    platform_input PlatformInput = {0};
    PlatformInput.dt = TargetSecondsPerFrame;

    win32_rect_dim ClientRect = Win32GetClientRect(Window);
    Win32ResizeDIBSection(&GlobalBackBuffer, ClientRect.Width, ClientRect.Height);

    LARGE_INTEGER PerformanceFrequency = {0};
    QueryPerformanceFrequency(&PerformanceFrequency);

    LARGE_INTEGER EndTime;
    QueryPerformanceCounter(&EndTime);

    PlatformMemory.TicksElapsed = PlatformMemory.MSElapsed = 0;
    PlatformMemory.Frequency = EndTime.QuadPart;

    GlobalRunning = true;
    while(GlobalRunning)
    {
        LARGE_INTEGER StartTime = EndTime;

        POINT MousePosition; GetCursorPos(&MousePosition);
        ScreenToClient(Window, &MousePosition);
        PlatformInput.MouseX = MousePosition.x;
        PlatformInput.MouseY = MousePosition.x;
        PlatformInput.MouseZ = 0;

        for(u32 KeyboardButtonIndex = 0;
            KeyboardButtonIndex < ArrayCount(PlatformInput.KeyboardButtons);
            ++KeyboardButtonIndex)
        {
            PlatformInput.KeyboardButtons[KeyboardButtonIndex].TransitionCount = 0;
        }

        Win32ProcessPendingMessages(&PlatformInput);

        GameUpdateAndRender(&GlobalBackBuffer.FrameBuffer, &PlatformMemory, &PlatformInput);

        if(PlatformMemory.GameWantsExit)
        {
            GlobalRunning = false;
        }

        HDC DeviceContext = GetDC(Window);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext);
        ReleaseDC(Window, DeviceContext);

        QueryPerformanceCounter(&EndTime);
        f32 SecondsElapsed = (f32)((f32)(EndTime.QuadPart - StartTime.QuadPart) / (f32)PerformanceFrequency.QuadPart);
        f32 MillisecondsElapsed = SecondsElapsed * 1000.0f;

        s32 MSToSleep = (s32)(TargetMSPerFrame - MillisecondsElapsed);
        if(MSToSleep > 0)
        {
            Sleep(MSToSleep);
        }

        QueryPerformanceCounter(&EndTime);
        f32 FinalSecondsElapsed = (f32)((f32)(EndTime.QuadPart - StartTime.QuadPart) / (f32)PerformanceFrequency.QuadPart);
        f32 FinalMillisecondsElapsed = FinalSecondsElapsed * 1000.0f;
        PlatformMemory.TicksElapsed += (EndTime.QuadPart - StartTime.QuadPart);
        PlatformMemory.MSElapsed += FinalMillisecondsElapsed;

        char Buffer[256] = {0};
        snprintf(Buffer, ArrayCount(Buffer), "%dfps %.2fms/f %.2fms/u\n",
                 (s32)(1000.0f / FinalMillisecondsElapsed),
                 FinalMillisecondsElapsed,
                 MillisecondsElapsed);
        OutputDebugString(Buffer);
        QueryPerformanceCounter(&EndTime);
    }

    return(0);
}

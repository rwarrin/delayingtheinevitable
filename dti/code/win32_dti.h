#ifndef WIN32_DTI_H

struct win32_rect_dim
{
    s32 Width;
    s32 Height;
};

struct win32_back_buffer
{
    bitmap_header BitmapHeader;
    union
    {
        frame_buffer FrameBuffer;
        struct
        {
            u32 Width;
            u32 Height;
            u8 *Data;
        };
    };
};

#define WIN32_DTI_H
#endif

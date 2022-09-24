#ifndef DTI_RENDER_GROUP_H

struct render_group
{
    u8 *PushBufferBase;
    mem_size MaxPushBufferSize;
    mem_size PushBufferSize;
};

enum render_entry_type
{
    RenderEntryType_render_entry_none,

    RenderEntryType_render_entry_clear,
    RenderEntryType_render_entry_rectangle,
    RenderEntryType_render_entry_bitmap,

    RenderEntryType_render_entry_count
};

struct render_entry_header
{
    render_entry_type Type;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_rectangle
{
    v4 Color;
    v2 P;
    v2 Dim;
};

struct render_entry_bitmap
{
    v2 P;
    bitmap *Bitmap;
    v4 Color;
};

#define DTI_RENDER_ENTRY_H
#endif

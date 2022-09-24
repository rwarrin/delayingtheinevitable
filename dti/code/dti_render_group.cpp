
static render_group *
AllocateRenderGroup(memory_arena *Arena, mem_size MaxPushBufferSize)
{
    render_group *Result = PushStruct(Arena, render_group, 16);
    Result->PushBufferSize = 0;
    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferBase = (u8 *)PushSize(Arena, MaxPushBufferSize);

    return(Result);
}

#define PushRenderElement(Group, Type, ...) (Type *)PushRenderElement_(Group, sizeof(Type), RenderEntryType_##Type, ## __VA_ARGS__)
static void *
PushRenderElement_(render_group *RenderGroup, mem_size Size, render_entry_type Type)
{
    void *Result = 0;

    Size += sizeof(render_entry_header);

    if((RenderGroup->PushBufferSize + Size) <= RenderGroup->MaxPushBufferSize)
    {
        render_entry_header *Header = (render_entry_header *)(RenderGroup->PushBufferBase + RenderGroup->PushBufferSize);
        Header->Type = Type;
        Result = Header + 1;
        RenderGroup->PushBufferSize += Size;
    }
    else
    {
        InvalidCodePath;
    }

    return(Result);
}

static void
PushClear(render_group *RenderGroup, v4 Color)
{
    render_entry_clear *Entry = PushRenderElement(RenderGroup, render_entry_clear);
    if(Entry)
    {
        Entry->Color = Color;
    }
}

static void
PushRect(render_group *RenderGroup, v2 P, v2 Dim, v4 Color)
{
    render_entry_rectangle *Entry = PushRenderElement(RenderGroup, render_entry_rectangle);
    if(Entry)
    {
        Entry->Color = Color;
        Entry->P = P;
        Entry->Dim = Dim;
    }
}

static void
PushBitmap(render_group *RenderGroup, bitmap *Bitmap, v2 P, v4 Color = V4(1, 1, 1, 1))
{
    render_entry_bitmap *Entry = PushRenderElement(RenderGroup, render_entry_bitmap);
    if(Entry)
    {
        Entry->P = P;
        Entry->Bitmap = Bitmap;
        Entry->Color = Color;
    }
}

static void
RenderGroupToOutput(render_group *RenderGroup, frame_buffer *FrameBuffer)
{
    u32 PushBufferAt = 0;
    while(PushBufferAt < RenderGroup->PushBufferSize)
    {
        render_entry_header *Header = (render_entry_header *)(RenderGroup->PushBufferBase + PushBufferAt);
        PushBufferAt += sizeof(*Header);
        switch(Header->Type)
        {
            case RenderEntryType_render_entry_clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)(RenderGroup->PushBufferBase + PushBufferAt);
                DrawClearSIMD(FrameBuffer, Entry->Color);
                PushBufferAt += sizeof(*Entry);
            } break;

            case RenderEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)(RenderGroup->PushBufferBase + PushBufferAt);
                DrawRectangleSIMD(FrameBuffer, Entry->P, Entry->Dim, Entry->Color);
                PushBufferAt += sizeof(*Entry);
            } break;

            case RenderEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)(RenderGroup->PushBufferBase + PushBufferAt);
                DrawBitmapSIMD(FrameBuffer, Entry->Bitmap, Entry->P, Entry->Color);
                PushBufferAt += sizeof(*Entry);
            } break;

            InvalidDefaultCase;
        }
    }
}


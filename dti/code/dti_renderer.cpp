static void
DrawCoolBackground(frame_buffer *FrameBuffer)
{
    u32 *Pixel = (u32 *)FrameBuffer->Data;
    for(u32 Y = 0; Y < FrameBuffer->Height; ++Y)
    {
        for(u32 X = 0; X < FrameBuffer->Width; ++X)
        {
            *Pixel++ = 0xFF000000 + ((u8)X << 16) + ((u8)(X % (Y+1)) << 8) + (u8)Y;
        }
    }
}

static void
DrawClear(frame_buffer *FrameBuffer, v4 Color)
{
    s32 MinX = 0;
    s32 MaxX = FrameBuffer->Width;
    s32 MinY = 0;
    s32 MaxY = FrameBuffer->Width;

    u32 PixelColor = ( ((s32)(Color.a * 255.0f) << 24) |
                       ((s32)(Color.r * 255.0f) << 16) |
                       ((s32)(Color.g * 255.0f) << 8 ) |
                       ((s32)(Color.b * 255.0f) << 0 ) );

    u32 *Pixel = (u32 *)FrameBuffer->Data;
    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        for(u32 X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = PixelColor;
        }
    }
}

static void
DrawClearSIMD(frame_buffer *FrameBuffer, v4 Color)
{
    u32 PixelColor = ( ((s32)(Color.a * 255.0f) << 24) |
                       ((s32)(Color.r * 255.0f) << 16) |
                       ((s32)(Color.g * 255.0f) << 8 ) |
                       ((s32)(Color.b * 255.0f) << 0 ) );
    __m128i Pixel4x = _mm_set1_epi32(PixelColor);

    u32 BufferLength = FrameBuffer->Width * FrameBuffer->Height;
    u32 *Pixel = (u32 *)FrameBuffer->Data;
    u32 Remaining = BufferLength;
    for( ; Remaining >= 4; Remaining -= 4, Pixel += 4)
    {
        *(__m128i *)Pixel = Pixel4x;
    }

    for(u32 ToGo = 0; ToGo < Remaining; ++ToGo)
    {
        *Pixel++ = PixelColor;
    }
}

static void
DrawRectangle(frame_buffer *FrameBuffer, v2 P, v2 Dim, v4 Color)
{
    s32 MinX = (s32)(P.x);
    s32 MaxX = (s32)(P.x + Dim.w);
    s32 MinY = (s32)(P.y);
    s32 MaxY = (s32)(P.y + Dim.h);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MaxX > FrameBuffer->Width)
    {
        MaxX = FrameBuffer->Width;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxY > FrameBuffer->Height)
    {
        MaxY = FrameBuffer->Height;
    }

    f32 Inv255 = 1.0f / 255.0f;
    Color.rgb = Color.rgb * Color.a;

    u32 *DestRow = (u32 *)FrameBuffer->Data + (MinY * FrameBuffer->Width);
    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *DestPixel = DestRow + MinX;
        for(u32 X = MinX; X < MaxX; ++X)
        {
            v4 DestV4 = {
                (f32)((*DestPixel >> 16) & 0xFF),
                (f32)((*DestPixel >> 8) & 0xFF),
                (f32)((*DestPixel >> 0) & 0xFF),
                (f32)((*DestPixel >> 24) & 0xFF)
            };
            DestV4.rgb = DestV4.rgb * Inv255;
            v3 OutRGB = ((1.0f - Color.a)*DestV4.rgb) + Color.rgb;

            *DestPixel++ = ( (255 << 24) |
                             ((s32)(OutRGB.r * 255.0f) << 16) |
                             ((s32)(OutRGB.g * 255.0f) << 8) |
                             ((s32)(OutRGB.b * 255.0f) << 0) );
        }

        DestRow += FrameBuffer->Width;
    }
}

static void
DrawRectangleSIMD(frame_buffer *FrameBuffer, v2 P, v2 Dim, v4 Color)
{
    s32 MinX = (s32)(P.x);
    s32 MaxX = (s32)(P.x + Dim.w);
    s32 MinY = (s32)(P.y);
    s32 MaxY = (s32)(P.y + Dim.h);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MaxX > FrameBuffer->Width)
    {
        MaxX = FrameBuffer->Width;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxY > FrameBuffer->Height)
    {
        MaxY = FrameBuffer->Height;
    }

    f32 Inv255 = 1.0f / 255.0f;
    Color.rgb = Color.rgb * Color.a;

    __m128 _255 = _mm_set1_ps(255.0f);
    __m128 _One = _mm_set1_ps(1.0f);
    __m128 _Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128i _MaskFF = _mm_set1_epi32(0xFF);

    __m128 ColorInOriginalA = _mm_set1_ps(Color.a);
    __m128 ColorInA = _mm_set1_ps(Color.a);
    __m128 ColorInR = _mm_set1_ps(Color.r);
    __m128 ColorInG = _mm_set1_ps(Color.g);
    __m128 ColorInB = _mm_set1_ps(Color.b);

    __m128i MaxPixelX = _mm_set1_epi32(MaxX);

    u32 *DestRow = (u32 *)FrameBuffer->Data + (MinY * FrameBuffer->Width);
    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *DestPixel = DestRow + MinX;
        for(u32 X = MinX; X < MaxX; X += 4)
        {
            // NOTE(rick): Create mask
            __m128i PixelX = _mm_setr_epi32(X, X+1, X+2, X+3); 
            __m128i WriteMask = _mm_cmplt_epi32(PixelX, MaxPixelX);

            // NOTE(rick): Load dest and convert to floating point 0-1 scale
            __m128i OriginalDest = _mm_load_si128((__m128i *)DestPixel);
            __m128 OriginalDestA = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), _MaskFF)), _Inv255);
            __m128 OriginalDestR = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), _MaskFF)), _Inv255);
            __m128 OriginalDestG = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest,  8), _MaskFF)), _Inv255);
            __m128 OriginalDestB = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest,  0), _MaskFF)), _Inv255);

            // NOTE(rick): Alpha blend
            __m128 DestAlphaContrib = _mm_sub_ps(_One, ColorInOriginalA);
            __m128 ResultA = _mm_add_ps(_mm_mul_ps(DestAlphaContrib, OriginalDestA), ColorInA);
            __m128 ResultR = _mm_add_ps(_mm_mul_ps(DestAlphaContrib, OriginalDestR), ColorInR);
            __m128 ResultG = _mm_add_ps(_mm_mul_ps(DestAlphaContrib, OriginalDestG), ColorInG);
            __m128 ResultB = _mm_add_ps(_mm_mul_ps(DestAlphaContrib, OriginalDestB), ColorInB);

            // NOTE(rick): Convert to integer 0-255
            __m128i ResultIntA = _mm_cvtps_epi32(_mm_mul_ps(ResultA, _255));
            __m128i ResultIntR = _mm_cvtps_epi32(_mm_mul_ps(ResultR, _255));
            __m128i ResultIntG = _mm_cvtps_epi32(_mm_mul_ps(ResultG, _255));
            __m128i ResultIntB = _mm_cvtps_epi32(_mm_mul_ps(ResultB, _255));

            // NOTE(Rick): Reconstruct 4 4-byte pixels
            __m128i PixelsA = _mm_slli_epi32(ResultIntA, 24);
            __m128i PixelsR = _mm_slli_epi32(ResultIntR, 16);
            __m128i PixelsG = _mm_slli_epi32(ResultIntG,  8);
            __m128i PixelsB = _mm_slli_epi32(ResultIntB,  0);
            __m128i Output = _mm_or_si128(_mm_or_si128(PixelsA, PixelsR), _mm_or_si128(PixelsG, PixelsB));
            
            // NOTE(rick): Apply write mask and store
            Output = _mm_or_si128(_mm_and_si128(WriteMask, Output),
                                  _mm_andnot_si128(WriteMask, OriginalDest));
            *(__m128i *)DestPixel = Output;
            DestPixel += 4;
        }

        DestRow += FrameBuffer->Width;
    }
}

static void
DrawBitmap(frame_buffer *FrameBuffer, bitmap *Bitmap, v2 P, v4 Color)
{
    s32 MinDestX = P.x;
    if(MinDestX < 0)
    {
        MinDestX = 0;
    }

    s32 MaxDestX = P.x + Bitmap->Width;
    if(MaxDestX > FrameBuffer->Width)
    {
        MaxDestX = FrameBuffer->Width;
    }

    s32 MinDestY = P.y;
    if(MinDestY < 0)
    {
        MinDestY = 0;
    }

    s32 MaxDestY = P.y + Bitmap->Height;
    if(MaxDestY > FrameBuffer->Height)
    {
        MaxDestY = FrameBuffer->Height;
    }

    s32 SourceMinX = 0;
    if(P.x < MinDestX)
    {
        SourceMinX = (s32)-P.x;
    }

    s32 SourceMinY = 0;
    if(P.y < MinDestY)
    {
        SourceMinY = (s32)-P.y;
    }

    u32 *DestRow = (u32 *)FrameBuffer->Data + (MinDestY * FrameBuffer->Width);
    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height - 1) - SourceMinY) * Bitmap->Width);
    for(s32 Y = MinDestY; Y < MaxDestY; ++Y)
    {
        u32 *DestPixel = DestRow + MinDestX;
        u32 *SourcePixel = SourceRow + SourceMinX;
        for(s32 X = MinDestX; X < MaxDestX; ++X)
        {
            u32 SourceColor = *SourcePixel++;
            v4 SourcePixelV4 = { (f32)((SourceColor >> Bitmap->RedShift) & 0xFF),
                                 (f32)((SourceColor >> Bitmap->GreenShift) & 0xFF),
                                 (f32)((SourceColor >> Bitmap->BlueShift) & 0xFF),
                                 (f32)((SourceColor >> Bitmap->AlphaShift) & 0xFF) };

            f32 SourceAlpha = (SourcePixelV4.a / 255.0f) * Color.a;
            SourcePixelV4.rgb = SourcePixelV4.rgb * SourceAlpha;
            SourcePixelV4.a = SourcePixelV4.a * SourceAlpha;

            SourcePixelV4.r *= Color.r*Color.a;
            SourcePixelV4.g *= Color.g*Color.a;
            SourcePixelV4.b *= Color.b*Color.a;

            v4 DestPixelV4 = { (f32)((*DestPixel >> 16) & 0xFF),
                               (f32)((*DestPixel >>  8) & 0xFF),
                               (f32)((*DestPixel >>  0) & 0xFF),
                               (f32)((*DestPixel >> 24) & 0xFF) };
            DestPixelV4.rgb = SourcePixelV4.rgb + ((1.0f - SourceAlpha)*DestPixelV4.rgb);
            DestPixelV4.a = SourcePixelV4.a + ((1.0f - SourceAlpha)*DestPixelV4.a);

            *DestPixel++ = ( ((s32)DestPixelV4.a << 24) |
                             ((s32)DestPixelV4.r << 16) |
                             ((s32)DestPixelV4.g <<  8) |
                             ((s32)DestPixelV4.b <<  0) );
        }
        DestRow += FrameBuffer->Width;
        SourceRow -= Bitmap->Width;
    }
}

static void
DrawBitmapSIMD(frame_buffer *FrameBuffer, bitmap *Bitmap, v2 P, v4 Color)
{
    s32 MinDestX = P.x;
    if(MinDestX < 0)
    {
        MinDestX = 0;
    }

    s32 MaxDestX = P.x + Bitmap->Width;
    if(MaxDestX > FrameBuffer->Width)
    {
        MaxDestX = FrameBuffer->Width;
    }

    s32 MinDestY = P.y;
    if(MinDestY < 0)
    {
        MinDestY = 0;
    }

    s32 MaxDestY = P.y + Bitmap->Height;
    if(MaxDestY > FrameBuffer->Height)
    {
        MaxDestY = FrameBuffer->Height;
    }

    s32 SourceMinX = 0;
    if(P.x < MinDestX)
    {
        SourceMinX = (s32)-P.x;
    }

    s32 SourceMinY = 0;
    if(P.y < MinDestY)
    {
        SourceMinY = (s32)-P.y;
    }

    __m128 _One = _mm_set1_ps(1.0f);
    __m128 _255 = _mm_set1_ps(255.0f);
    __m128 _Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128i _MaskFF = _mm_set1_epi32(0xFF);

    __m128 ColorInOriginalA = _mm_set1_ps(Color.a);
    __m128 ColorInA = _mm_set1_ps(Color.a);
    __m128 ColorInR = _mm_set1_ps(Color.r);
    __m128 ColorInG = _mm_set1_ps(Color.g);
    __m128 ColorInB = _mm_set1_ps(Color.b);

    __m128i MaxPixelX = _mm_set1_epi32(MaxDestX);

    u32 *DestRow = (u32 *)FrameBuffer->Data + (MinDestY * FrameBuffer->Width);
    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height - 1) - SourceMinY) * Bitmap->Width);
    for(s32 Y = MinDestY; Y < MaxDestY; ++Y)
    {
        u32 *DestPixel = DestRow + MinDestX;
        u32 *SourcePixel = SourceRow + SourceMinX;
        for(s32 X = MinDestX; X < MaxDestX; X += 4)
        {
            // NOTE(rick): Create mask
            __m128i PixelX = _mm_setr_epi32(X, X+1, X+2, X+3);
            __m128i WriteMask = _mm_cmplt_epi32(PixelX, MaxPixelX);

            // NOTE(rick): Load source and convert to float 0-1
            __m128i OriginalSrc = _mm_load_si128((__m128i *)SourcePixel);
            __m128 SourceInA = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalSrc, Bitmap->AlphaShift), _MaskFF)), _Inv255);
            __m128 SourceInR = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalSrc, Bitmap->RedShift), _MaskFF)), _Inv255);
            __m128 SourceInG = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalSrc, Bitmap->GreenShift), _MaskFF)), _Inv255);
            __m128 SourceInB = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalSrc, Bitmap->BlueShift), _MaskFF)), _Inv255);

            // NOTE(rick): Pre-multiply alpha
            __m128 SourceAlphaContrib = _mm_mul_ps(SourceInA, ColorInOriginalA);
            //__m128 SourceA = _mm_mul_ps(SourceInA, _mm_mul_ps(ColorInA, ColorInOriginalA));
            __m128 SourceR = _mm_mul_ps(SourceInR, _mm_mul_ps(ColorInR, ColorInOriginalA));
            __m128 SourceG = _mm_mul_ps(SourceInG, _mm_mul_ps(ColorInG, ColorInOriginalA));
            __m128 SourceB = _mm_mul_ps(SourceInB, _mm_mul_ps(ColorInB, ColorInOriginalA));

            // NOTE(rick): Modulate source by Color
            SourceInA = _mm_mul_ps(SourceInA, _mm_mul_ps(ColorInA, ColorInA));
            SourceInR = _mm_mul_ps(SourceInR, _mm_mul_ps(ColorInR, ColorInA));
            SourceInG = _mm_mul_ps(SourceInG, _mm_mul_ps(ColorInG, ColorInA));
            SourceInB = _mm_mul_ps(SourceInB, _mm_mul_ps(ColorInB, ColorInA));

            // NOTE(rick): Load dest and convert to float 0-1
            __m128i Dest = _mm_load_si128((__m128i *)DestPixel);
            __m128 DestA = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Dest, 24), _MaskFF)), _Inv255);
            __m128 DestR = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Dest, 16), _MaskFF)), _Inv255);
            __m128 DestG = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Dest,  8), _MaskFF)), _Inv255);
            __m128 DestB = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(Dest,  0), _MaskFF)), _Inv255);

            // NOTE(rick): Alpha blend source and dest
            __m128 DestAlphaContrib = _mm_sub_ps(DestA, SourceInA);
            __m128i ResultIntA = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(SourceInA, _mm_mul_ps(DestA, DestAlphaContrib)), _255));
            __m128i ResultIntR = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(SourceR, _mm_mul_ps(DestR, DestAlphaContrib)), _255));
            __m128i ResultIntG = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(SourceG, _mm_mul_ps(DestG, DestAlphaContrib)), _255));
            __m128i ResultIntB = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(SourceB, _mm_mul_ps(DestB, DestAlphaContrib)), _255));

            // NOTE(rick): Reconstruct 4 4-byte pixels
            __m128i PixelsA = _mm_slli_epi32(ResultIntA, 24);
            __m128i PixelsR = _mm_slli_epi32(ResultIntR, 16);
            __m128i PixelsG = _mm_slli_epi32(ResultIntG,  8);
            __m128i PixelsB = _mm_slli_epi32(ResultIntB,  0);
            __m128i Pixels = _mm_or_si128(_mm_or_si128(PixelsA, PixelsR), _mm_or_si128(PixelsG, PixelsB));

            // NOTE(rick): Apply write mask and store
            __m128i Output = _mm_or_si128(_mm_and_si128(WriteMask, Pixels),
                                          _mm_andnot_si128(WriteMask, Dest));
            *(__m128i *)DestPixel = Output;

            SourcePixel += 4;
            DestPixel += 4;
        }
        DestRow += FrameBuffer->Width;
        SourceRow -= Bitmap->Width;
    }
}


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define ZeroStruct(Struct) ZeroSize(Struct, sizeof(Struct))
#define ZeroArray(Array) ZeroSize(Array, sizeof(Array))
inline void
ZeroSize(void *SrcIn, uint32_t Size)
{
    uint8_t *Src = (uint8_t *)SrcIn;
    while(Size--)
    {
        *Src++ = 0;
    }
}

inline void
Copy(void *SrcIn, void *DestIn, uint32_t Size)
{
    int8_t *Src = (int8_t *)SrcIn;
    int8_t *Dest = (int8_t *)DestIn;
    while(Size--)
    {
        *Dest++ = *Src++;
    }
}

#pragma pack(push, 1)
struct master_chunk
{
    union
    {
        char FileIDStr[4];
        uint32_t FileID;
    };
    uint32_t DataSize;
    union
    {
        char FormatIDStr[4];
        uint32_t FormatID;
    };
};

struct chunk_header
{
    union
    {
        char IDStr[4];
        uint32_t ID;
    };
    uint32_t ChunkSize;
};

struct format_chunk
{
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AverageBytesPerSec;
    uint16_t Alignment;
    uint16_t BitsPerSample;
    uint16_t cbSize;
    uint16_t ValidBitsPerSample;
    uint32_t ChannelMask;
    uint8_t SubFormat[16];
};

struct fact_chunk
{
    uint32_t SampleLength;
};

struct data_chunk
{
    uint8_t *Data;
};
#pragma pack(pop)

#define AsU32(A, B, C, D) ( (A << 0) | (B << 8) | (C << 16) | (D << 24) )

struct sound
{
    int8_t *Samples;
    int32_t Size;
    int32_t At;
    int32_t Loop;
    int32_t Playing;
};

static sound
LoadSound(char *FileName)
{
    uint32_t FileSize = 0;
    uint8_t *FileData = 0;

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        FileData = (uint8_t *)malloc(FileSize);
        fread(FileData, 1, FileSize, File);

        fclose(File);
    }

    uint32_t RIFFFile = AsU32('R', 'I', 'F', 'F');
    uint32_t WAVEFormat = AsU32('W', 'A', 'V', 'E');
    uint32_t ChunkID_Fmt = AsU32('f', 'm', 't', ' ');
    uint32_t ChunkID_Data = AsU32('d', 'a', 't', 'a');

    sound Sound = {0};

    uint8_t *ParseAt = FileData;
    if(FileData && FileSize)
    {
        master_chunk *MasterChunk = (master_chunk *)ParseAt;
        if((MasterChunk->FileID == RIFFFile) &&
           (MasterChunk->FormatID == WAVEFormat))
        {
            ParseAt += sizeof(*MasterChunk);

            Sound.Samples = (int8_t *)malloc(MasterChunk->DataSize);
            int8_t *SampleWriter = Sound.Samples;
            ZeroSize(SampleWriter, MasterChunk->DataSize);

            while((ParseAt - FileData) < FileSize)
            {
                chunk_header *Header = (chunk_header *)ParseAt;
                ParseAt += sizeof(*Header);

                if(ChunkID_Fmt == Header->ID)
                {
                    format_chunk *FormatChunk = (format_chunk *)ParseAt;
                    int a = 5;
                    // TODO(rick): Do we care??
                }
                else if(ChunkID_Data == Header->ID)
                {
                    Copy(ParseAt, SampleWriter, Header->ChunkSize);
                    SampleWriter += Header->ChunkSize;
                    Sound.Size += Header->ChunkSize;
                }
                else
                {
                    // TODO(rick): Do we care??
                }

                ParseAt += Header->ChunkSize;
            }
        }
    }

    return(Sound);
}

#ifndef WAVELOADER_LIBRARY 
int main(void)
{
    char *FileName = "bloop_01.wav";
    sound Sound = LoadSound(FileName);

    return(0);
}
#endif

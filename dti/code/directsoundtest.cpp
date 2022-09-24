#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <dsound.h>

#define Absolute(Value) ((Value) > 0 ? Value : -Value)

#define WAVELOADER_LIBRARY
#include "waveloadertest.cpp"

static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

int main(void)
{
#if 0
    sound Sound = LoadSound("song.wav");
    Sound.Playing = true;
    Sound.Loop = false;
#elif 0
    sound Sound = LoadSound("step_test.wav");
    Sound.Playing = true;
    Sound.Loop = true;
#else
    uint32_t SoundCount = 0;
    uint32_t MaxSoundCount = 8;
    sound Sounds[8] = {0};

    sound SoundSong = LoadSound("song.wav");
    SoundSong.Playing = true;
    SoundSong.Loop = false;

    sound SoundStep = LoadSound("step_test.wav");
    SoundStep.Playing = true;
    SoundStep.Loop = true;

    Sounds[SoundCount++] = SoundSong;
    Sounds[SoundCount++] = SoundStep;
#endif

    HWND Window = GetForegroundWindow();
    if (Window == NULL)
    {
        fprintf(stderr, "Failed to get window\n");
        //Window = GetDesktopWindow();
    }

    if(timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        fprintf(stderr, "Failed to get high res timer.\n");
    }

    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);

    LARGE_INTEGER Last;
    QueryPerformanceCounter(&Last);

    float TargetFPS = 60;
    float TargetSecondsPerFrame = (1.0f / TargetFPS);
    float TargetMSPerFrame = TargetSecondsPerFrame * 1000.0f;

    int32_t SamplesPerSecond = 48000;
    int32_t BytesPerSample = sizeof(uint16_t)*2;
    int32_t SecondaryBufferSize = SamplesPerSecond*BytesPerSample;

    HMODULE DirectSoundDLL = LoadLibrary("dsound.dll");
    if(DirectSoundDLL)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DirectSoundDLL, "DirectSoundCreate");
        if(DirectSoundCreate)
        {
            LPDIRECTSOUND DirectSound;
            if(SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
            {
                WAVEFORMATEX WaveFormat = {0};
                WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
                WaveFormat.nChannels = 2;
                WaveFormat.nSamplesPerSec = 48000;
                WaveFormat.wBitsPerSample = 16;
                WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
                WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
                WaveFormat.cbSize = 0;

                if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
                {
                    DSBUFFERDESC BufferDescription = {0};
                    BufferDescription.dwSize = sizeof(BufferDescription);
                    BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    BufferDescription.dwBufferBytes = 0;
                    BufferDescription.dwReserved = 0;
                    BufferDescription.lpwfxFormat = 0;
                    BufferDescription.guid3DAlgorithm = DS3DALG_DEFAULT;

                    LPDIRECTSOUNDBUFFER PrimaryBuffer;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                    {
                        if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                        {
                        }
                        else
                        {
                            fprintf(stderr, "PrimaryBuffer->SetFormat() failed\n");
                        }
                    }
                    else
                    {
                        fprintf(stderr, "CreateSoundBuffer() Primary failed\n");
                    }
                }
                else
                {
                    fprintf(stderr, "SetCooperativeLevel() failed\n");
                }

                DSBUFFERDESC BufferDescription = {0};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = 0|DSBCAPS_GETCURRENTPOSITION2 ;
                BufferDescription.dwBufferBytes = SecondaryBufferSize;
                BufferDescription.dwReserved = 0;
                BufferDescription.lpwfxFormat = &WaveFormat;
                BufferDescription.guid3DAlgorithm = DS3DALG_DEFAULT;

                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
                {
                }
                else
                {
                    fprintf(stderr, "CreateSoundBuffer() Primary failed\n");
                }
            }
            else
            {
                fprintf(stderr, "DirectSoundCreate() failed\n");
            }
        }
        else
        {
            fprintf(stderr, "Failed to load DirectSoundCreate.\n");
        }
    }
    else
    {
        fprintf(stderr, "Failed to load direct sound library.\n");
    }

    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    uint32_t RunningSampleIndex = 0;
    int32_t SoundIsValid = false;

    DWORD ExpectedAudioBytesPerFrame = (TargetSecondsPerFrame * SamplesPerSecond) * BytesPerSample;
    DWORD SafetyAudioBytes = ((SamplesPerSecond * BytesPerSample / TargetFPS) / 3);
    for(;;)
    {
        if(GetAsyncKeyState(VK_LMENU))
        {
            Sounds[0].At = SamplesPerSecond*BytesPerSample*140;
        }

        DWORD PlayCursor;
        DWORD WriteCursor;
        if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
            if(!SoundIsValid)
            {
                RunningSampleIndex = WriteCursor / BytesPerSample;
                SoundIsValid = true;
            }

            if(SoundIsValid)
            {
                int32_t ByteToLock = (RunningSampleIndex*BytesPerSample) % SecondaryBufferSize;

                DWORD ExpectedFrameBoundryByte = PlayCursor + ExpectedAudioBytesPerFrame;

                DWORD SafeWriteCursor = WriteCursor;
                if(SafeWriteCursor < PlayCursor)
                {
                    SafeWriteCursor += SecondaryBufferSize;
                }
                assert(SafeWriteCursor >= PlayCursor);
                SafeWriteCursor += SafetyAudioBytes;

                int32_t AudioIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundryByte);

                int32_t TargetAudioCursor = ByteToLock;
                if(AudioIsLowLatency)
                {
                    TargetAudioCursor = (ExpectedFrameBoundryByte + ExpectedAudioBytesPerFrame);
                }
                else
                {
                    TargetAudioCursor = (WriteCursor + ExpectedAudioBytesPerFrame + SafetyAudioBytes);
                }
                TargetAudioCursor = TargetAudioCursor % SecondaryBufferSize;

                int32_t BytesToLockForFrame = 0;
                if(ByteToLock > TargetAudioCursor)
                {
                    BytesToLockForFrame = (SecondaryBufferSize - ByteToLock);
                    BytesToLockForFrame += TargetAudioCursor;
                }
                else
                {
                    BytesToLockForFrame = TargetAudioCursor - ByteToLock;
                }

                DWORD UnwrappedWriteCursor = WriteCursor;
                if(UnwrappedWriteCursor < PlayCursor)
                {
                    UnwrappedWriteCursor += SecondaryBufferSize;
                }
                DWORD AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                float AudioLatencySeconds = ((float)AudioLatencyBytes / (float)BytesPerSample) / (float)SamplesPerSecond;

                LARGE_INTEGER ClocksUntilAudioBegin;
                QueryPerformanceCounter(&ClocksUntilAudioBegin);
                float SecondsFromBeginToAudio = float(ClocksUntilAudioBegin.QuadPart - Last.QuadPart) / (float)(Freq.QuadPart);

                printf("BTL:%u TC:%u BTW:%u - PC:%u WC:%u Delta:%u (%fs) - SP:%u%%\n",
                       ByteToLock, TargetAudioCursor, BytesToLockForFrame,
                       PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds,
                       (uint32_t)(((float)Sounds[0].At / (float)Sounds[0].Size)*100));
                VOID *Region1;
                DWORD Region1Size;
                VOID *Region2;
                DWORD Region2Size;

                if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToLockForFrame,
                                                         &Region1, &Region1Size,
                                                         &Region2, &Region2Size,
                                                         0)))
                {
                    DWORD Region1SampleCount = Region1Size / BytesPerSample;
                    DWORD Region2SampleCount = Region2Size / BytesPerSample;
                    ZeroSize(Region1, Region1Size);
                    ZeroSize(Region2, Region2Size);

                    for(uint32_t SoundIndex = 0; SoundIndex < SoundCount; ++SoundIndex)
                    {
                        sound *Sound_ = Sounds + SoundIndex;
                        sound Sound = *Sound_;
                        if(Sound.Playing)
                        {
                            assert(Sound.At <= Sound.Size);
                            int32_t SoundAt = Sound.At;
                            int16_t *SoundSample = (int16_t *)(Sound.Samples + SoundAt);

                            int16_t *Region1Output = (int16_t *)Region1;
                            for(DWORD SampleIndex = 0;
                                SampleIndex < Region1SampleCount;
                                ++SampleIndex)
                            {
                                if(SoundAt >= Sound.Size)
                                {
                                    if(Sound.Loop)
                                    {
                                        SoundAt = Sound.At % Sound.Size;
                                        SoundSample = (int16_t *)(Sound.Samples + SoundAt);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                *Region1Output++ += *SoundSample++;
                                *Region1Output++ += *SoundSample++;
                                SoundAt += BytesPerSample;
                            }
                            assert(SoundAt <= Sound.Size);

                            int16_t *Region2Output = (int16_t *)Region2;
                            for(DWORD SampleIndex = 0;
                                SampleIndex < Region2SampleCount;
                                ++SampleIndex)
                            {
                                if(SoundAt >= Sound.Size)
                                {
                                    if(Sound.Loop)
                                    {
                                        SoundAt = Sound.At % Sound.Size;
                                        SoundSample = (int16_t *)(Sound.Samples + SoundAt);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                *Region2Output++ += *SoundSample++;
                                *Region2Output++ += *SoundSample++;
                                SoundAt += BytesPerSample;
                            }
                            assert(SoundAt <= Sound.Size);

                            Sound.At = SoundAt;
                            if(Sound.At > Sound.Size)
                            {
                                if(Sound.Loop)
                                {
                                    Sound.At = Sound.At % Sound.Size;
                                }
                                else
                                {
                                    Sound.Playing = false;
                                    Sound.At = 0;
                                }
                            }
                            assert(Sound.At <= Sound.Size);
                            *Sound_ = Sound;
                        }
                    }

                    RunningSampleIndex += (Region1SampleCount + Region2SampleCount);

                    if(SUCCEEDED(GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size)))
                    {
                    }
                    else
                    {
                        fprintf(stderr, "GlobalSecondaryBuffer->Unlock() failed\n");
                    }
                }
                else
                {
                    fprintf(stderr, "GlobalSecondaryBuffer->Lock() failed\n");
                }
            }
        }
        else
        {
            SoundIsValid = false;
            fprintf(stderr, "GetCurrentPosition() failed\n");
        }

        LARGE_INTEGER End;
        QueryPerformanceCounter(&End);

        float SecondsElapsed = (float)(End.QuadPart - Last.QuadPart) / (float)(Freq.QuadPart);
        float MSElapsed = SecondsElapsed * 1000.0f;

        if(MSElapsed < TargetMSPerFrame)
        {
            DWORD MSToSleep = TargetMSPerFrame - MSElapsed;
            Sleep(MSToSleep);
        }

        QueryPerformanceCounter(&End);
        float ActualTime = ((float)(End.QuadPart - Last.QuadPart) / (float)(Freq.QuadPart)) * 1000.0f;
        //printf("%2.2f ms/f\n", ActualTime);

        Last = End;
    }

    return 0;
}

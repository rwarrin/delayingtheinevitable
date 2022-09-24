#ifndef DTI_MATH_H

#include <math.h>

union v2
{
    struct
    {
        f32 x;
        f32 y;
    };
    struct
    {
        f32 w;
        f32 h;
    };
    f32 E[2];
};

inline v2
V2(f32 A, f32 B)
{
    v2 Result;
    Result.x = A;
    Result.y = B;
    return(Result);
}

inline v2
operator *(v2 A, f32 B)
{
    v2 Result;
    Result.x = A.x * B;
    Result.y = A.y * B;
    return(Result);
}

inline v2
operator *(f32 A, v2 B)
{
    v2 Result = B*A;
    return(Result);
}

inline v2
operator -(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    return(Result);
}

inline v2
operator +(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    return(Result);
}

inline v2&
operator +=(v2 &A, v2 B)
{
    A.x = A.x + B.x;
    A.y = A.y + B.y;
    return(A);
}

inline f32
Inner(v2 A, v2 B)
{
    f32 Result = 0.0f;
    Result = A.x*B.x + A.y*B.y;
    return(Result);
}

inline f32
LengthSq(v2 A)
{
    f32 Result = Inner(A, A);
    return(Result);
}

inline f32
Length(v2 A)
{
    f32 Result = sqrtf(LengthSq(A));
    return(Result);
}

inline v2
Normalize(v2 A)
{
    v2 Result = A * (1.0f / Length(A));
    return(Result);
}

union v3
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
    };
    struct
    {
        f32 r;
        f32 g;
        f32 b;
    };
    f32 E[3];
};

inline v3
V3(f32 A, f32 B, f32 C)
{
    v3 Result;
    Result.x = A;
    Result.y = B;
    Result.z = C;
    return(Result);
}

inline v3
operator +(v3 A, v3 B)
{
    v3 Result = {0};
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return(Result);
}

inline v3
operator *(v3 A, f32 B)
{
    v3 Result = {0};
    Result.x = A.x * B;
    Result.y = A.y * B;
    Result.z = A.z * B;
    return(Result);
}

inline v3
operator *(f32 A, v3 B)
{
    v3 Result = B*A;
    return(Result);
}

union v4
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct
    {
        v3 xyz;
        f32 _unused0;
    };
    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    struct
    {
        v3 rgb;
        f32 _unused1;
    };
    f32 E[4];
};

inline v4
V4(f32 A, f32 B, f32 C, f32 D)
{
    v4 Result;
    Result.x = A;
    Result.y = B;
    Result.z = C;
    Result.w = D;
    return(Result);
}

inline s32
Absolute(s32 Val)
{
    s32 Result = Val;
    if(Result < 0)
    {
        Result = -Result;
    }
    return(Result);
}

inline u32
Ceil(u32 Value)
{
    u32 Result = ((Value >= 0) ? (u32)((f32)Value + 0.5f) : (u32)((f32)Value - 0.5f));
    return(Result);
}

#define DTI_MATH_H
#endif

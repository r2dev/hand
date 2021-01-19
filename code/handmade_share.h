/* date = January 19th 2021 0:24 pm */

#ifndef HANDMADE_SHARE_H
#define HANDMADE_SHARE_H

#include "handmade_intrinsics.h"
#include "handmade_math.h"

inline b32
StringsAreEqual(char* A, char* B) {
    b32 Result = A == B;
    while (*A && *B &&  *A++ == *B++) {
        
    }
    Result= (*A == 0 && *B == 0);
    return(Result);
}

inline b32
StringsAreEqual(umm ALenght, char* A, char* B) {
    char *At = B;
    b32 Result = true;
    for (u32 Index = 0; Index < ALenght; ++Index, ++At) {
        if (*At == 0 || A[Index] != *At) {
            Result = false;
            break;
        }
    }
    Result = (*At == 0);
    return(Result);
}

inline b32
StringsAreEqual(memory_index ALenght, char* A, memory_index BLength, char* B) {
    b32 Result = ALenght == BLength;
    if (Result) {
        Result = true;
        for (u32 Index = 0; Index < ALenght; ++Index) {
            if (A[Index] != B[Index]) {
                Result = false;
                break;
            }
        }
    }
    return(Result);
}

inline b32
IsEndOfLine(char C) {
    b32 Result = false;
    Result = (C == '\n' || C == '\r');
    return(Result);
}

inline b32
IsWhiteSpace(char C) {
    b32 Result = false;
    Result = (C  == ' ' || C == '\t' || C == '\f' || C == '\v' || IsEndOfLine(C));
    return(Result);
}


#endif //HANDMADE_SHARE_H

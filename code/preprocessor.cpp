#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static char *
ReadEntireFileIntoMemoryAndNullTerminate(char *Filename) {
    char *Result = 0;
    FILE *File = fopen(Filename, "r");
    if (File) {
        // get file size
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);
        
        Result = (char *)malloc(FileSize + 1);
        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;
        fclose(File);
    }
    
    return(Result);
}

enum token_type {
    Token_Unknown,
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    
    Token_Asterisk,
    Token_Semicolon,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBraces,
    Token_CloseBraces,
    
    Token_Identifier,
    Token_String,
    Token_EndOfStream,
};

struct token {
    char *Text;
    size_t TextLength;
    token_type Type;
};

struct tokenizer {
    char *At;
};

inline bool
TokenEqual(token Token, char *Match) {
    char *At = Match;
    for (int Index = 0; Index < Token.TextLength; ++Index) {
        if (*At == 0 || *At != Token.Text[Index]) {
            return(false);
        }
        At++;
    }
    
    bool Result = (*At == 0);
    return(Result);
    
}

inline bool
IsEndOfLine(char C) {
    bool Result = false;
    Result = (C == '\n' || C == '\r');
    return(Result);
}

inline bool
IsWhiteSpace(char C) {
    bool Result = false;
    Result = (C  == ' ' || C == '\t' || C == '\f' || C == '\v' || IsEndOfLine(C));
    return(Result);
}

static void
EatAllWhiteSpace(tokenizer *Tokenizer) {
    for (;;) {
        if (IsWhiteSpace(Tokenizer->At[0])) {
            ++Tokenizer->At;
        } else if (Tokenizer->At[0] == '/' && Tokenizer->At[1] == '/') {
            Tokenizer->At += 2;
            while (Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0])) {
                Tokenizer->At++;
            }
        } else if (Tokenizer->At[0] == '/' && Tokenizer->At[1] == '*') {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !(Tokenizer->At[0] == '*' && Tokenizer->At[1] == '/')) {
                Tokenizer->At++;
            }
            if (Tokenizer->At[0] == '*') {
                Tokenizer->At += 2;
            }
        } else {
            break;
        }
    }
}

inline bool
IsAlpha(char C) {
    bool Result = ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'));
    return(Result);
}

inline bool
IsNumber(char C) {
    bool Result = (( C >= '0') && (C <= '9'));
    return(Result);
}

static token
GetToken(tokenizer *Tokenizer) {
    EatAllWhiteSpace(Tokenizer);
    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;
    switch(Tokenizer->At[0]) {
        case '(': {Token.Type = Token_OpenParen; ++Tokenizer->At;} break;
        case ')': {Token.Type = Token_CloseParen; ++Tokenizer->At;} break;
        case '{': {Token.Type = Token_OpenBraces; ++Tokenizer->At;} break;
        case '}': {Token.Type = Token_CloseBraces; ++Tokenizer->At;} break;
        case ':': {Token.Type = Token_Colon; ++Tokenizer->At;} break;
        case ';': {Token.Type = Token_Semicolon; ++Tokenizer->At;} break;
        case '*': {Token.Type = Token_Asterisk; ++Tokenizer->At;} break;
        case '[': {Token.Type = Token_OpenBracket; ++Tokenizer->At;} break;
        case ']': {Token.Type = Token_CloseBracket; ++Tokenizer->At;} break;
        case '\0': {Token.Type = Token_EndOfStream; ++Tokenizer->At;} break;
        case '"': {
            ++Tokenizer->At;
            
            Token.Text = Tokenizer->At;
            while(Tokenizer->At[0] && Tokenizer->At[0] != '"') {
                if (Tokenizer->At[0] == '\\' && Tokenizer->At[1]) {
                    ++Tokenizer->At;
                }
                ++Tokenizer->At;
            }
            Token.Type = Token_String;
            Token.TextLength = Tokenizer->At - Token.Text;
            if (Tokenizer->At[0] == '"') {
                ++Tokenizer->At;
            }
        } break;
        default: {
            if (IsAlpha(Tokenizer->At[0])) {
                Token.Text = Tokenizer->At;
                while(IsAlpha(Tokenizer->At[0]) || IsNumber(Tokenizer->At[0]) || Tokenizer->At[0] == '_') {
                    ++Tokenizer->At;
                }
                Token.Type = Token_Identifier;
                Token.TextLength = Tokenizer->At - Token.Text;
            }
#if 0
            else if (IsNumeric(Tokenizer->At[0])) {
                ParseNumber(Tokenizer);
            }
#endif
            
            else {
                Token.Type = Token_Unknown;
                ++Tokenizer->At;
            }
        }
    }
    return(Token);
}
static void
ParseIntrospectionParams(tokenizer *Tokenizer) {
    for (;;) {
        token Token = GetToken(Tokenizer);
        if (Token.Type == Token_CloseParen || Token.Type == Token_EndOfStream) {
            break;
        }
    }
}

inline bool
RequireToken(tokenizer *Tokenizer, token_type TokenType) {
    token Token = GetToken(Tokenizer);
    bool Result = (Token.Type == TokenType);
    return Result;
}

static void
ParseMember(tokenizer *Tokenizer, token StructTypeToken, token MemberTypeToken, bool IsPointer = false) {
    token TypeToken = GetToken(Tokenizer);
    switch(TypeToken.Type) {
        case Token_Identifier: {
            printf("  {%s, MetaType_%.*s, \"%.*s\", (u32)&((%.*s *)0)->%.*s},\n", 
                   IsPointer? "MetaMemberFlag_IsPointer": "0",
                   (int)MemberTypeToken.TextLength, MemberTypeToken.Text,
                   (int)TypeToken.TextLength, TypeToken.Text,
                   (int)StructTypeToken.TextLength, StructTypeToken.Text,
                   (int)TypeToken.TextLength, TypeToken.Text);
        } break;
        case Token_Asterisk: {
            ParseMember(Tokenizer, StructTypeToken, MemberTypeToken, true);
        } break;
    }
}

struct meta_struct {
    char *Name;
    meta_struct *Next;
};
static meta_struct *FirstMetaStruct;

static void
ParseStruct(tokenizer *Tokenizer) {
    token NameToken = GetToken(Tokenizer);
    if (NameToken.Type == Token_Identifier) {
        
    } else {
        fprintf(stderr, "Error: Missing struct name. \n");
    }
    
    if (RequireToken(Tokenizer, Token_OpenBraces)) {
        
        //meta_struct
        meta_struct *MetaStruct = (meta_struct*)malloc(sizeof(meta_struct));
        MetaStruct->Name = (char*)malloc(NameToken.TextLength + 1);
        memcpy(MetaStruct->Name, NameToken.Text, NameToken.TextLength);
        MetaStruct->Name[NameToken.TextLength] = 0;
        MetaStruct->Next = FirstMetaStruct;
        FirstMetaStruct = MetaStruct;
        
        printf("member_definition MembersOf_%.*s[] = \n", (int)NameToken.TextLength, NameToken.Text);
        printf("{\n");
        for (;;) {
            token MemberToken = GetToken(Tokenizer);
            if (MemberToken.Type == Token_CloseBraces) {
                break;
            }
            else if (MemberToken.Type == Token_Identifier) {
                ParseMember(Tokenizer, NameToken, MemberToken);
            }
        }
        printf("};\n");
    } else {
        fprintf(stderr, "Error: Missing braces. \n");
    }
}

static void
ParseIntrospectable(tokenizer *Tokenizer) {
    if (RequireToken(Tokenizer, Token_OpenParen)) {
        ParseIntrospectionParams(Tokenizer);
        token TypeToken = GetToken(Tokenizer);
        if (TokenEqual(TypeToken, "struct")) {
            ParseStruct(Tokenizer);
        } else if (TokenEqual(TypeToken, "union")) {
        } else {
            fprintf(stderr, "Error: Introspect only support struct");
        }
    } else {
        fprintf(stderr, "Error: Missing parentheses. \n");
    }
}

int main(int ArgCount, char **Args) {
    char *FileName[] = {
        "handmade_sim_region.h",
        "handmade_math.h",
        "handmade_world.h",
        "handmade_platform.h"
    };
    for (int FileIndex = 0; FileIndex < (sizeof(FileName) / sizeof(FileName[0])); ++FileIndex) {
        char *LoadedContent = ReadEntireFileIntoMemoryAndNullTerminate(FileName[FileIndex]);
        tokenizer Tokenizer = {};
        Tokenizer.At = LoadedContent;
        bool Parsing = true;
        while (Parsing) {
            token Token = GetToken(&Tokenizer);
            switch(Token.Type) {
                case Token_Unknown: {
                } break;
                case Token_EndOfStream: {
                    Parsing = false;
                } break;
                case Token_Identifier: {
                    if (TokenEqual(Token, "Introspect")) {
                        ParseIntrospectable(&Tokenizer);
                    }
                } break;
                default: {
                    //printf("%d: %.*s\n", Token.Type, (int)Token.TextLength, Token.Text);
                } break;
            }
        }
    }
    printf("#define META_HANDLE_TYPE_DUMP(MemberPtr, LineIndent) \\\n");
    for (meta_struct *MetaStruct = FirstMetaStruct; MetaStruct; MetaStruct = MetaStruct->Next) {
        printf("    case MetaType_%s: {DEBUGTextLine(Member->Name);DEBUGDumpStruct(ArrayCount(MembersOf_%s), MembersOf_%s, MemberPtr, LineIndent + 1);} break; %s\n", MetaStruct->Name, MetaStruct->Name, MetaStruct->Name, (MetaStruct->Next? "\\": ""));
    }
}
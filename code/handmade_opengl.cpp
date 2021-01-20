#include "handmade_render_group.h"

#define SRGB8_ALPHA8_EXT 0x8C43
#define GL_FRAMEBUFFER_SRGB 0x8DB9

#define GL_SHADING_LANGUAGE_VERSION             0x8B8C

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

struct opengl_info {
    char *Vendor;
    char *Renderer;
    char *Version;
    char *ShadingLanguageVersion;
    char *Extensions;
    
    b32 GL_EXT_texture_sRGB;
    b32 GL_EXT_framebuffer_sRGB;
};

internal opengl_info
OpenGLGetInfo(b32 ModernContext) {
    opengl_info Result = {};
    Result.Vendor = (char*)glGetString(GL_VENDOR);
    Result.Renderer = (char*)glGetString(GL_RENDERER);
    Result.Version = (char*)glGetString(GL_VERSION);
    if (ModernContext) {
        Result.ShadingLanguageVersion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    } else {
        Result.ShadingLanguageVersion = "none";
    }
    Result.Extensions = (char*)glGetString(GL_EXTENSIONS);
    char *At = Result.Extensions;
    
    while(*At) {
        while (IsWhiteSpace(*At)) {++At;};
        char *End = At;
        while(*End && !IsWhiteSpace(*End)) {++End;};
        umm Count = End - At;
        
        if      (StringsAreEqual(Count, At, "GL_EXT_texture_sRGB")){ Result.GL_EXT_texture_sRGB = true; }
        else if (StringsAreEqual(Count, At, "GL_EXT_framebuffer_sRGB")){ Result.GL_EXT_framebuffer_sRGB = true; }
        
        At = End;
    }
    
    return(Result);
}

internal void
OpenGLInit(b32 ModernContext) {
    opengl_info Info = OpenGLGetInfo(ModernContext);
    
    DefaultInternalTextureFormat = GL_RGBA8;
    if (Info.GL_EXT_texture_sRGB) {
        DefaultInternalTextureFormat = SRGB8_ALPHA8_EXT;
    }
    if (Info.GL_EXT_framebuffer_sRGB) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
}

inline void 
OpenGLSetScreenSpace(s32 Width, s32 Height) {
    glMatrixMode(GL_PROJECTION);
    
    r32 Proj[] = {
        SafeRatio1(2.0f, (r32)Width), 0, 0, 0,
        0, SafeRatio1(2.0f, (r32)Height), 0, 0,
        0, 0, 1.0f, 0,
        -1.0f, -1.0f, 0, 1.0f,
    };
    glLoadMatrixf(Proj);
    
}
inline void
OpenGLRectangle(v2 MinP, v2 MaxP, v4 Color) {
    glBegin(GL_TRIANGLES);
    glColor4f(Color.r, Color.g, Color.b, Color.a);
    
    glTexCoord2f(0, 0);
    glVertex2f(MinP.x, MinP.y);
    glTexCoord2f(1.0f, 0);
    glVertex2f(MaxP.x, MinP.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.x, MaxP.y);
    
    glTexCoord2f(0, 0);
    glVertex2f(MinP.x, MinP.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.x, MaxP.y);
    glTexCoord2f(0, 1.0f);
    glVertex2f(MinP.x, MaxP.y);
    
    glEnd();
}

inline void
OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, s32 Pitch, s32 WindowWidth, s32 WindowHeight) {
    
    Assert(Pitch == (Width * 4));
    glViewport(0, 0, Width, Height);
    
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, DefaultInternalTextureFormat, Width, Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glEnable(GL_TEXTURE_2D);
    
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    OpenGLSetScreenSpace(Width, Height);
    
    v4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
    v2 MinP = {0, 0};
    v2 MaxP = {(r32)Width, (r32)Height};
    
    OpenGLRectangle(MinP, MaxP, Color);
}

global_variable u32 TextBindCount = 0;

internal void
OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight) {
    // Passing windowWidth and height
    glViewport(0, 0, Commands->Width, Commands->Height);
    
    //glViewport(0, 0, WindowWidth, WindowHeight);
    glEnable(GL_TEXTURE_2D);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    OpenGLSetScreenSpace(Commands->Width, Commands->Height);
    
	for (u32 BaseAddress = 0; BaseAddress < Commands->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(Commands->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(*Header);
		void* Data = Header + 1;
        
		//todo null
		r32 NullPixelsToMeters = 1.0f;
        
		switch (Header->Type) {
            case RenderGroupEntryType_render_entry_clear: {
                render_entry_clear* Entry = (render_entry_clear*)Data;
                
                glClearColor(Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);
                glClear(GL_COLOR_BUFFER_BIT);
                
                BaseAddress += sizeof(*Entry);
            } break;
            
            case RenderGroupEntryType_render_entry_rectangle: {
                render_entry_rectangle* Entry = (render_entry_rectangle*)Data;
                glDisable(GL_TEXTURE_2D);
                
                OpenGLRectangle(Entry->P, Entry->P + Entry->Dim, Entry->Color);
                glEnable(GL_TEXTURE_2D);
                BaseAddress += sizeof(*Entry);
            } break;
            case RenderGroupEntryType_render_entry_bitmap: {
                render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
                
                v2 XAxis = {1, 0};
                v2 YAxis = {0, 1};
                v2 MinP = Entry->P;
                v2 MaxP = MinP + Entry->Size.x * XAxis + Entry->Size.y * YAxis;
                
                glBindTexture(GL_TEXTURE_2D, (GLuint)Entry->Bitmap->TextureHandle);
                
				OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                
                BaseAddress += sizeof(*Entry);
                
            } break;
            
            case RenderGroupEntryType_render_entry_coordinate_system: {
            } break;
			InvalidDefaultCase;
		}
	}
}
PLATFORM_ALLOCATE_TEXTURE(Win32AllocateTexture) {
    GLuint Handle;
    glGenTextures(1, &Handle);
    
    glBindTexture(GL_TEXTURE_2D, Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, DefaultInternalTextureFormat, Width, Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Assert(sizeof(Handle) <= sizeof(void *));
    return(Handle);
}

PLATFORM_DEALLOCATE_TEXTURE(Win32DeallocateTexture) {
    GLuint Handle = (GLuint)Texture;
    glDeleteTextures(1, &Handle);
}

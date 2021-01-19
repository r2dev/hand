#include "handmade_render_group.h"
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

global_variable u32 TextBindCount = 0;

internal void
OpenGLRenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget) {
    glViewport(0, 0, OutputTarget->Width, OutputTarget->Height);
    
    glEnable(GL_TEXTURE_2D);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    r32 a = SafeRatio1(2.0f, (r32)OutputTarget->Width);
    r32 b = SafeRatio1(2.0f, (r32)OutputTarget->Height);
    r32 Proj[] = {
        a, 0, 0, 0,
        0, b, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1,
    };
    glLoadMatrixf(Proj);
    
	for (u32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
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
                
                if (Entry->Bitmap->Handle) {
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                } else {
                    Entry->Bitmap->Handle = ++TextBindCount;
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                    glTexImage2D(GL_TEXTURE_2D, 0, DefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                }
                
                
				OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                
                BaseAddress += sizeof(*Entry);
                
            } break;
            
            case RenderGroupEntryType_render_entry_coordinate_system: {
            } break;
			InvalidDefaultCase;
		}
	}
}

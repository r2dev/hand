render_group*
AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize, real32 MetersToPixels) {
	render_group* Result = PushStruct(Arena, render_group);

	Result->MetersToPixels = MetersToPixels;

	Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);
	Result->MaxPushBufferSize = MaxPushBufferSize;

	Result->DefaultBasis = PushStruct(Arena, render_basis);
	Result->DefaultBasis->P = v3{ 0, 0, 0 };

	Result->PieceCount = 0;

	Result->PushBufferSize = 0;
	return(Result);
}
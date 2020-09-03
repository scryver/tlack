internal void
reserve_space(Assembler *assembler, umm size)
{
    umm newMax = assembler->codeAt + size;
    if (assembler->codeData.size < newMax)
    {
        umm newSize = 2 * assembler->codeData.size;
        while (newSize < newMax)
        {
            newSize *= 2;
        }
        assembler->codeData = reallocate_buffer(assembler->allocator, assembler->codeData, newSize);
        i_expect(assembler->codeData.data);
    }
}

internal void
emit(Assembler *assembler, u8 value)
{
    reserve_space(assembler, 1);
    assembler->codeData.data[assembler->codeAt++] = value;
}

internal void
emit_bytes(Assembler *assembler, u32 byteCount, u8 *bytes)
{
    reserve_space(assembler, byteCount);
    for (u32 index = 0; index < byteCount; ++index)
    {
        assembler->codeData.data[assembler->codeAt++] = bytes[index];
    }
}

internal void
emit_u16(Assembler *assembler, u16 value)
{
    reserve_space(assembler, 2);
    *(u16 *)(assembler->codeData.data + assembler->codeAt) = value;
    assembler->codeAt += 2;
}

internal void
emit_u32(Assembler *assembler, u32 value)
{
    reserve_space(assembler, 4);
    *(u32 *)(assembler->codeData.data + assembler->codeAt) = value;
    assembler->codeAt += 4;
}

internal void
emit_u64(Assembler *assembler, u64 value)
{
    reserve_space(assembler, 8);
    *(u64 *)(assembler->codeData.data + assembler->codeAt) = value;
    assembler->codeAt += 8;
}

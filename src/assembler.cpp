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

internal AsmSymbol *
create_global_sym(Assembler *assembler, AsmSymbolKind kind, String name, umm address)
{
    i_expect(assembler->globalCount < MAX_GLOBAL_SYMBOLS);
    AsmSymbol *result = assembler->globals + assembler->globalCount++;
    result->kind = kind;
    result->name = name;
    result->operand.kind = AsmOperand_Address;
    result->operand.symbol = result;
    result->operand.oAddress = address;
    
    mmap_put(&assembler->globalMap, result->name.data, result);
    
    return result;
}

internal AsmSymbol *
create_local_sym(Assembler *assembler, AsmSymbolKind kind, String name)
{
    i_expect(assembler->localCount < MAX_LOCAL_SYMBOLS);
    AsmSymbol *result = assembler->locals + assembler->localCount++;
    result->kind = kind;
    result->name = name;
    result->operand.kind = AsmOperand_FrameOffset;
    result->operand.symbol = result;
    result->operand.oFrameOffset = -8 * (s64)assembler->localCount;
    return result;
}

internal AsmSymbol *
find_symbol(Assembler *assembler, String name)
{
    AsmSymbol *result = 0;
    for (u32 onePast = assembler->localCount; onePast > 0; --onePast)
    {
        AsmSymbol *test = assembler->locals + onePast - 1;
        if (test->name == name)
        {
            result = test;
        }
    }
    
    if (!result)
    {
        result = (AsmSymbol *)mmap_get(&assembler->globalMap, name.data);
    }
    return result;
}

internal Assembler *
create_assembler(MemoryAllocator *allocator, umm initialSize, umm initialOffset = 0)
{
    i_expect(initialOffset < initialSize);
    Assembler *assembler = allocate_struct(allocator, Assembler, default_memory_alloc());
    assembler->allocator = allocator;
    assembler->codeData.data = (u8 *)allocate_size(allocator, initialSize, default_memory_alloc());
    if (assembler->codeData.data)
    {
        assembler->codeData.size = initialSize;
        assembler->codeAt = initialOffset;
    }
    initialize_free_registers(assembler);
    return assembler;
}

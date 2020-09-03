struct Assembler
{
    MemoryAllocator *allocator;
    
    Buffer codeData;
    umm codeAt;
    
    u32 freeRegisterMask;
};

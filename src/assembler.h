enum AsmOperandKind
{
    AsmOperand_None,
    AsmOperand_Register,
    AsmOperand_FrameOffset,
    AsmOperand_Immediate,
    AsmOperand_Address,
};

struct AsmOperand
{
    AsmOperandKind kind;
    union {
        Register oRegister;
        s64      oFrameOffset;
        s64      oImmediate;
        s64      oAddress;
    };
};

enum AsmSymbolKind
{
    AsmSymbol_None,
    AsmSymbol_Var,
    AsmSymbol_Func,
};

struct AsmSymbol
{
    AsmSymbolKind kind;
    String name;
    AsmOperand operand; // NOTE(michiel): FrameOffset or Address (maybe Immediate for const later on?)
};

#define MAX_GLOBAL_SYMBOLS  1024
#define MAX_LOCAL_SYMBOLS   1024

struct Assembler
{
    MemoryAllocator *allocator;
    
    MemoryMap  globalMap; // NOTE(michiel): [name.data] = symbol *
    u32 globalCount;
    AsmSymbol globals[MAX_GLOBAL_SYMBOLS];
    u32 localCount;
    AsmSymbol locals[MAX_LOCAL_SYMBOLS];
    
    Buffer codeData;
    umm codeAt;
    
    u32 freeRegisterMask;
};

internal b32
is_8bit(s64 value)
{
    return (value >= (s64)S8_MIN) && (value <= (s64)S8_MAX);
}

internal b32
is_32bit(s64 value)
{
    return (value >= (s64)S32_MIN) && (value <= (s64)S32_MAX);
}

internal b32
is_32bit_unsigned(s64 value)
{
    return (value >= 0) && (value <= (s64)(u64)U32_MAX);
}

internal void initialize_free_registers(Assembler *assembler);

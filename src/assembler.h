
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

internal AsmSymbol *
create_global_sym(Assembler *assembler, AsmSymbolKind kind, String name, umm address)
{
    i_expect(assembler->globalCount < MAX_GLOBAL_SYMBOLS);
    AsmSymbol *result = assembler->globals + assembler->globalCount++;
    result->kind = kind;
    result->name = name;
    result->operand.kind = AsmOperand_Address;
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

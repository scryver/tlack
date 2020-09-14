struct AsmSymbol;

enum AsmOperandKind
{
    AsmOperand_None,
    AsmOperand_LockedRegister, // NOTE(michiel): Borrowed allocation
    AsmOperand_Register,
    AsmOperand_FrameOffset,
    AsmOperand_Immediate,
    AsmOperand_Address,
};

struct AsmOperand
{
    AsmOperandKind kind;
    AsmSymbol *symbol;         // NOTE(michiel): Can be 0
    union {
        Register oRegister;
        s64      oFrameOffset;
        s64      oImmediate;
        s64      oAddress;
    };
};

internal b32
operator ==(AsmOperand &a, AsmOperand &b)
{
    b32 result = ((a.kind == b.kind) &&
                  (a.oAddress == b.oAddress)); // TODO(michiel): Keep an eye on this one
    return result;
}

internal b32
operator !=(AsmOperand &a, AsmOperand &b)
{
    b32 result = ((a.kind != b.kind) ||
                  (a.oAddress != b.oAddress)); // TODO(michiel): Keep an eye on this one
    return result;
}

enum AsmSymbolKind
{
    AsmSymbol_None,
    AsmSymbol_Var,
    AsmSymbol_Func,
};

struct AsmSymbol
{
    AsmSymbolKind kind;
    Register loadedRegister;
    String name;
    AsmOperand operand; // NOTE(michiel): FrameOffset or Address (maybe Immediate for const later on?)
};

struct AsmStatementResult
{
    AsmSymbol *symbol;
    AsmOperand original;
    AsmOperand result;
};

#define MAX_GLOBAL_SYMBOLS  1024
#define MAX_LOCAL_SYMBOLS   1024
#define MAX_REGISTERS         32 // TODO(michiel): Jup, should not be done like this

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
    
    AsmOperand *registerUsage[MAX_REGISTERS]; // NOTE(michiel): Only valid when bit in freeRegisterMask is not set
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

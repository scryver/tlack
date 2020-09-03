enum InstructionKind
{
    Instr_None,
    Instr_Zero,
    Instr_Move,
    Instr_Negate,
    Instr_Invert,
    Instr_Not,
    Instr_Add,
    Instr_Subtract,
    Instr_Multiply,
    Instr_Divide,
    Instr_Jump,
    Instr_Return,
};

enum OperandKind
{
    Operand_None,
    Operand_Integer,
    Operand_Register,
    Operand_RelAddress,
    Operand_AbsAddress,
};

struct Operand
{
    OperandKind kind;
    union {
        s64 constantInt;
        u64 registerIdx;
        s64 relativeAddr;
        u64 absoluteAddr;
    };
};

struct Instruction
{
    InstructionKind kind;
    Operand dest;
    Operand src1;
    Operand src2;
};

// TODO(michiel): Add raw data
struct OpBuilder
{
    u64 nextRegIdx;
    Buffer instrStorage;
    umm instrAt;
};

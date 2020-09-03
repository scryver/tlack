
internal Instruction *
create_instruction(OpBuilder *builder, InstructionKind kind)
{
    umm newMax = builder->instrAt + sizeof(Instruction);
    if (builder->instrStorage.size < newMax)
    {
        umm newSize = 2 * builder->instrStorage.size;
        builder->instrStorage = reallocate_buffer(builder->instrStorage, newSize);
        i_expect(builder->instrStorage.data);
    }
    Instruction *result = (Instruction *)(builder->instrStorage.data + builder->instrAt);
    builder->instrAt += sizeof(Instruction);
    
    result->kind = kind;
    return result;
}

internal Operand
constant_operand(s64 value)
{
    Operand result = {};
    result.kind = Operand_Integer;
    result.constantInt = value;
    return result;
}

internal Operand
register_operand(u64 regIdx)
{
    Operand result = {};
    result.kind = Operand_Register;
    result.registerIdx = regIdx;
    return result;
}

internal Operand
rel_address_operand(s64 relAddress)
{
    Operand result = {};
    result.kind = Operand_RelAddress;
    result.relativeAddr = relAddress;
    return result;
}

internal Operand
abs_address_operand(u64 absAddress)
{
    Operand result = {};
    result.kind = Operand_AbsAddress;
    result.absoluteAddr = absAddress;
    return result;
}

internal Operand
instr_zero(OpBuilder *builder)
{
    Instruction *zeroOp = create_instruction(builder, Instr_Zero);
    zeroOp->dest = register_operand(builder->nextRegIdx++);
    return zeroOp->dest;
}

internal Operand
instr_move(OpBuilder *builder, Operand a)
{
    Instruction *moveOp = create_instruction(builder, Instr_Move);
    moveOp->src1 = a;
    moveOp->dest = register_operand(builder->nextRegIdx++);
    return moveOp->dest;
}

internal Operand
instr_negate(OpBuilder *builder, Operand a)
{
    Instruction *negOp = create_instruction(builder, Instr_Negate);
    negOp->src1 = a;
    negOp->dest = register_operand(builder->nextRegIdx++);
    return negOp->dest;
}

internal Operand
instr_invert(OpBuilder *builder, Operand a)
{
    Instruction *invOp = create_instruction(builder, Instr_Invert);
    invOp->src1 = a;
    invOp->dest = register_operand(builder->nextRegIdx++);
    return invOp->dest;
}

internal Operand
instr_not(OpBuilder *builder, Operand a)
{
    Instruction *notOp = create_instruction(builder, Instr_Not);
    notOp->src1 = a;
    notOp->dest = register_operand(builder->nextRegIdx++);
    return notOp->dest;
}

internal Operand
instr_add(OpBuilder *builder, Operand a, Operand b)
{
    Instruction *addOp = create_instruction(builder, Instr_Add);
    addOp->src1 = a;
    addOp->src2 = b;
    addOp->dest = register_operand(builder->nextRegIdx++);
    return addOp->dest;
}

internal Operand
instr_sub(OpBuilder *builder, Operand a, Operand b)
{
    Instruction *subOp = create_instruction(builder, Instr_Subtract);
    subOp->src1 = a;
    subOp->src2 = b;
    subOp->dest = register_operand(builder->nextRegIdx++);
    return subOp->dest;
}

internal Operand
instr_mul(OpBuilder *builder, Operand a, Operand b)
{
    Instruction *mulOp = create_instruction(builder, Instr_Multiply);
    mulOp->src1 = a;
    mulOp->src2 = b;
    mulOp->dest = register_operand(builder->nextRegIdx++);
    return mulOp->dest;
}

internal Operand
instr_div(OpBuilder *builder, Operand a, Operand b)
{
    Instruction *divOp = create_instruction(builder, Instr_Divide);
    divOp->src1 = a;
    divOp->src2 = b;
    divOp->dest = register_operand(builder->nextRegIdx++);
    return divOp->dest;
}

internal void
instr_jump(OpBuilder *builder, Operand a)
{
    Instruction *jmpOp = create_instruction(builder, Instr_Jump);
    jmpOp->src1 = a;
}

internal void
instr_return(OpBuilder *builder, Operand a)
{
    Instruction *retOp = create_instruction(builder, Instr_Return);
    retOp->src1 = a;
}

//
// NOTE(michiel): AST
//

internal Operand
instr_from_expression(OpBuilder *builder, Expression *expression)
{
    Operand result = {};
    
    switch (expression->kind)
    {
        case Expr_Int: {
            if (expression->intConst)
            {
                result = instr_move(builder, constant_operand(expression->intConst));
            }
            else
            {
                result = instr_zero(builder);
            }
        } break;
        
        case Expr_Unary: {
            Operand operand = instr_from_expression(builder, expression->unary.operand);
            switch (expression->unary.op)
            {
                case Unary_Minus: {
                    result = instr_negate(builder, operand);
                } break;
                
                case Unary_Flip: {
                    result = instr_invert(builder, operand);
                } break;
                
                case Unary_Not: {
                    result = instr_not(builder, operand);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        } break;
        
        case Expr_Binary: {
            Operand left  = instr_from_expression(builder, expression->binary.left);
            Operand right = instr_from_expression(builder, expression->binary.right);
            switch (expression->binary.op)
            {
                case Binary_Add: {
                    result = instr_add(builder, left, right);
                } break;
                
                case Binary_Sub: {
                    result = instr_sub(builder, left, right);
                } break;
                
                case Binary_Mul: {
                    result = instr_mul(builder, left, right);
                } break;
                
                case Binary_Div: {
                    result = instr_div(builder, left, right);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

internal void
instr_from_statement(OpBuilder *builder, Statement *statement)
{
    switch (statement->kind)
    {
        case Stmt_Return: {
            Operand result = {};
            if (statement->expression)
            {
                result = instr_from_expression(builder, statement->expression);
            }
            instr_return(builder, result);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
instr_from_stmt_block(OpBuilder *builder, StmtBlock *block)
{
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        instr_from_statement(builder, block->statements[stmtIdx]);
    }
}

internal void
instr_from_function(OpBuilder *builder, Function *function)
{
    // TODO(michiel): Add label (function->name)
    instr_from_stmt_block(builder, function->body);
}

internal void
instr_from_ast(OpBuilder *builder, Ast *ast)
{
    instr_from_function(builder, ast->program.main);
}

//
// NOTE(michiel): Printing
//

internal String
string_from_operand(Operand operand, u32 maxDestSize, u8 *dest)
{
    String result = {};
    switch (operand.kind)
    {
        case Operand_Integer   : { result = string_fmt(maxDestSize, dest, "%ld", operand.constantInt); } break;
        case Operand_Register  : { result = string_fmt(maxDestSize, dest, "r%lu", operand.registerIdx); } break;
        case Operand_RelAddress: { result = string_fmt(maxDestSize, dest, "@%+ld", operand.relativeAddr); } break;
        case Operand_AbsAddress: { result = string_fmt(maxDestSize, dest, "@%lu", operand.absoluteAddr); } break;
        INVALID_DEFAULT_CASE;
    }
    return result;
}

internal void
print_operand(FileStream *output, Operand operand)
{
    u8 stringBuf[128];
    String label = string_from_operand(operand, sizeof(stringBuf), stringBuf);
    print(output, "%.*s", STR_FMT(label));
}

internal void
print_instructions(FileStream *output, OpBuilder *builder)
{
    umm index = 0;
    output->indent = 1;
    while (index < builder->instrAt)
    {
        Instruction *instr = (Instruction *)(builder->instrStorage.data + index);
        index += sizeof(Instruction);
        
        switch (instr->kind)
        {
            case Instr_Zero: {
                println_begin(output, "zero ");
                print_operand(output, instr->dest);
                println_end(output);
            } break;
            
            case Instr_Move: {
                println_begin(output, "mov ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            case Instr_Negate: {
                println_begin(output, "neg ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            case Instr_Invert: {
                println_begin(output, "inv ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            case Instr_Not: {
                println_begin(output, "not ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            case Instr_Add: {
                println_begin(output, "add ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                print(output, ", ");
                print_operand(output, instr->src2);
                println_end(output);
            } break;
            
            case Instr_Subtract: {
                println_begin(output, "sub ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                print(output, ", ");
                print_operand(output, instr->src2);
                println_end(output);
            } break;
            
            case Instr_Multiply: {
                println_begin(output, "mul ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                print(output, ", ");
                print_operand(output, instr->src2);
                println_end(output);
            } break;
            
            case Instr_Divide: {
                println_begin(output, "div ");
                print_operand(output, instr->dest);
                print(output, ", ");
                print_operand(output, instr->src1);
                print(output, ", ");
                print_operand(output, instr->src2);
                println_end(output);
            } break;
            
            case Instr_Jump: {
                println_begin(output, "jmp ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            case Instr_Return: {
                println_begin(output, "ret ");
                print_operand(output, instr->src1);
                println_end(output);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}

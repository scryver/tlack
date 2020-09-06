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

#define emit_r_frame_offset(op, offset, dst) \
if (((offset) >= S8_MIN) && ((offset) <= S8_MAX)) { \
emit_r_mb(assembler, op, dst, Reg_RBP, (offset)); \
} else { \
i_expect(((offset) >= S32_MIN) && ((offset) <= S32_MAX)); \
emit_r_md(assembler, op, dst, Reg_RBP, (offset)); \
}

#define emit_x_frame_offset(op, offset) \
if (((offset) >= S8_MIN) && ((offset) <= S8_MAX)) { \
emit_x_mb(assembler, op, Reg_RBP, (offset)); \
} else { \
i_expect(((offset) >= S32_MIN) && ((offset) <= S32_MAX)); \
emit_x_md(assembler, op, Reg_RBP, (offset)); \
}

internal void
initialize_free_registers(Assembler *assembler)
{
    Register available_registers[] = {Reg_RCX, Reg_RBX, Reg_RSI, Reg_RDI, Reg_R8, Reg_R9, Reg_R10, Reg_R11, Reg_R12, Reg_R13, Reg_R14, Reg_R15};
    for (size_t i = 0; i < sizeof(available_registers) / sizeof(*available_registers); i++) {
        assembler->freeRegisterMask |= 1 << (available_registers[i] & 0xF);
    }
}

internal Register
allocate_register(Assembler *assembler)
{
    i_expect(assembler->freeRegisterMask);
    BitScanResult firstFree = find_least_significant_set_bit(assembler->freeRegisterMask);
    i_expect(firstFree.found);
    assembler->freeRegisterMask &= ~(1 << firstFree.index);
    Register result = (Register)(Reg_RAX | firstFree.index);
    return result;
}

internal void
deallocate_register(Assembler *assembler, Register oldReg) {
    i_expect((assembler->freeRegisterMask & (1 << (oldReg & 0xF))) == 0);
    assembler->freeRegisterMask |= (1 << (oldReg & 0xF));
}

internal void
allocate_operand_register(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind != AsmOperand_Register)
    {
        operand->kind = AsmOperand_Register;
        operand->oRegister = allocate_register(assembler);
    }
}

internal void
steal_operand_register(Assembler *assembler, AsmOperand *source, AsmOperand *dest)
{
    i_expect(source->kind == AsmOperand_Register);
    i_expect(dest->kind != AsmOperand_Register);
    dest->kind = AsmOperand_Register;
    dest->oRegister = source->oRegister;
    source->kind = AsmOperand_None;
}

internal void
deallocate_operand(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind == AsmOperand_Register)
    {
        deallocate_register(assembler, operand->oRegister);
    }
    operand->kind = AsmOperand_None;
}

internal void
patch_with_operand_address(Assembler *assembler, u32 offset, AsmOperand *operand)
{
    i_expect(operand->kind == AsmOperand_Address);
    i_expect((operand->oAddress >= 0) && (operand->oAddress <= S32_MAX));
    u8 *nextCode = assembler->codeData.data + assembler->codeAt;
    s32 *patch = (s32 *)(nextCode - 4 * offset);
    *patch = (s32)(operand->oAddress - assembler->codeAt);
    //*patch = (s32)(assembler->codeData.data - nextCode - 8) - (u32)operand->oAddress;
    //*(s32 *)(nextCode - 4 * offset) = (s32)((assembler->codeData.data - nextCode - 8) - (s32)operand->oAddress);
}

internal void
emit_operand_to_register(Assembler *assembler, AsmOperand *operand, Register targetReg)
{
    switch (operand->kind)
    {
        case AsmOperand_Immediate: { 
            if ((operand->oImmediate >= S32_MIN) && (operand->oImmediate <= S32_MAX))
            {
                emit_r_i(assembler, movsx, targetReg, operand->oImmediate);
            }
            else
            {
                emit_mov64_r_i(assembler, targetReg, operand->oImmediate);
            }
        } break;
        
        case AsmOperand_FrameOffset: {
            emit_r_frame_offset(mov, operand->oFrameOffset, targetReg);
        } break;
        
        case AsmOperand_Address: {
            emit_r_ripd(assembler, mov, targetReg, 0);
            patch_with_operand_address(assembler, 1, operand);
        } break;
        
        case AsmOperand_Register: {
            if (operand->oRegister != targetReg)
            {
                emit_r_r(assembler, mov, targetReg, operand->oRegister);
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
ensure_operand_has_register(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind != AsmOperand_Register)
    {
        Register operandReg = allocate_register(assembler);
        emit_operand_to_register(assembler, operand, operandReg);
        operand->kind = AsmOperand_Register;
        operand->oRegister = operandReg;
    }
}

internal void
emit_neg(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind == AsmOperand_Immediate)
    {
        operand->oImmediate = -operand->oImmediate;
    }
    else
    {
        ensure_operand_has_register(assembler, operand);
        emit_x_r(assembler, neg, operand->oRegister);
    }
}

internal void
emit_not(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind == AsmOperand_Immediate)
    {
        operand->oImmediate = ~operand->oImmediate;
    }
    else
    {
        ensure_operand_has_register(assembler, operand);
        emit_x_r(assembler, not, operand->oRegister);
    }
}

internal void
emit_logical_not(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind == AsmOperand_Immediate)
    {
        operand->oImmediate = !operand->oImmediate;
    }
    else
    {
        ensure_operand_has_register(assembler, operand);
        emit_r_i(assembler, cmp, operand->oRegister, 0);
        emit_r_r(assembler, xor, operand->oRegister, operand->oRegister);
        emit_c_r(assembler, set, CC_Equal, operand->oRegister);
    }
}

internal void
emit_mov(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    i_expect(dst->kind != AsmOperand_Immediate);
    if (dst->kind == AsmOperand_Register)
    {
        if (src->kind == AsmOperand_Immediate)
        {
            if ((src->oImmediate >= S32_MIN) && (src->oImmediate <= S32_MAX))
            {
                emit_r_i(assembler, movsx, dst->oRegister, src->oImmediate);
            }
            else
            {
                emit_mov64_r_i(assembler, dst->oRegister, src->oImmediate);
            }
        }
        else if (src->kind == AsmOperand_FrameOffset)
        {
            emit_r_frame_offset(mov, src->oFrameOffset, dst->oRegister);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_r_ripd(assembler, mov, dst->oRegister, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            i_expect(src->kind == AsmOperand_Register);
            emit_r_r(assembler, mov, dst->oRegister, src->oRegister);
        }
    }
    else if (dst->kind == AsmOperand_FrameOffset)
    {
        if ((dst->oFrameOffset >= S8_MIN) && (dst->oFrameOffset <= S8_MAX))
        {
            if (src->kind == AsmOperand_Immediate)
            {
                emit_mb_i(assembler, movsx, Reg_RBP, dst->oFrameOffset, src->oImmediate);
            }
            else
            {
                ensure_operand_has_register(assembler, src);
                emit_mb_r(assembler, mov, Reg_RBP, dst->oFrameOffset, src->oRegister);
            }
        }
        else
        {
            i_expect((dst->oFrameOffset >= S32_MIN) && (dst->oFrameOffset <= S32_MAX));
            if (src->kind == AsmOperand_Immediate)
            {
                emit_md_i(assembler, movsx, Reg_RBP, dst->oFrameOffset, src->oImmediate);
            }
            else
            {
                ensure_operand_has_register(assembler, src);
                emit_md_r(assembler, mov, Reg_RBP, dst->oFrameOffset, src->oRegister);
            }
        }
    }
    else
    {
        i_expect(dst->kind == AsmOperand_Address);
        if (src->kind == AsmOperand_Immediate)
        {
            emit_ripd_i(assembler, movsx, 0, src->oImmediate);
            patch_with_operand_address(assembler, 2, dst); // TODO(michiel): Check
        }
        else
        {
            ensure_operand_has_register(assembler, src);
            emit_ripd_r(assembler, mov, 0, src->oRegister);
            patch_with_operand_address(assembler, 1, dst);
        }
    }
}

internal void
emit_add(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        dst->oImmediate += src->oImmediate;
    }
    else if ((dst->kind == AsmOperand_Immediate) && (dst->oImmediate >= S32_MIN) && (dst->oImmediate <= S32_MAX))
    {
        ensure_operand_has_register(assembler, src);
        emit_r_i(assembler, add, src->oRegister, dst->oImmediate);
        steal_operand_register(assembler, src, dst);
    }
    else
    {
        ensure_operand_has_register(assembler, dst);
        
        switch (src->kind)
        {
            case AsmOperand_Immediate: {
                if ((src->oImmediate >= S32_MIN) && (src->oImmediate <= S32_MAX))
                {
                    emit_r_i(assembler, add, dst->oRegister, src->oImmediate);
                }
                else
                {
                    ensure_operand_has_register(assembler, src);
                    emit_r_r(assembler, add, dst->oRegister, src->oRegister);
                }
            } break;
            
            case AsmOperand_FrameOffset: {
                emit_r_frame_offset(add, src->oFrameOffset, dst->oRegister);
            } break;
            
            case AsmOperand_Address: {
                emit_r_ripd(assembler, add, dst->oRegister, 0);
                patch_with_operand_address(assembler, 1, src);
            } break;
            
            case AsmOperand_Register: {
                emit_r_r(assembler, add, dst->oRegister, src->oRegister);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}

internal void
emit_sub(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        dst->oImmediate -= src->oImmediate;
    }
    //else if (dst->kind == AsmOperand_Immediate)
    //{
    //ensure_operand_has_register(assembler, src);
    //emit_r_i(assembler, add, src->oRegister, dst->oImmediate);
    //steal_operand_register(assembler, src, dst);
    //}
    else
    {
        ensure_operand_has_register(assembler, dst);
        
        switch (src->kind)
        {
            case AsmOperand_Immediate: {
                if ((src->oImmediate >= S32_MIN) && (src->oImmediate <= S32_MAX))
                {
                    emit_r_i(assembler, sub, dst->oRegister, src->oImmediate);
                }
                else
                {
                    ensure_operand_has_register(assembler, src);
                    emit_r_r(assembler, sub, dst->oRegister, src->oRegister);
                }
            } break;
            
            case AsmOperand_FrameOffset: {
                emit_r_frame_offset(sub, src->oFrameOffset, dst->oRegister);
            } break;
            
            case AsmOperand_Address: {
                emit_r_ripd(assembler, sub, dst->oRegister, 0);
                patch_with_operand_address(assembler, 1, src);
            } break;
            
            case AsmOperand_Register: {
                emit_r_r(assembler, sub, dst->oRegister, src->oRegister);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}

internal void
emit_mul(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        dst->oImmediate *= src->oImmediate;
    }
    else if ((src->kind == AsmOperand_Immediate) && is_pow2(src->oImmediate))
    {
        ensure_operand_has_register(assembler, dst);
        emit_r_ib(assembler, shl, dst->oRegister, log2(src->oImmediate));
    }
    else if ((dst->kind == AsmOperand_Immediate) && is_pow2(dst->oImmediate))
    {
        ensure_operand_has_register(assembler, src);
        emit_r_ib(assembler, shl, src->oRegister, log2(dst->oImmediate));
        steal_operand_register(assembler, src, dst);
    }
    else
    {
        emit_operand_to_register(assembler, dst, Reg_RAX);
        
        if (src->kind == AsmOperand_FrameOffset)
        {
            emit_x_frame_offset(mul, src->oFrameOffset);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_x_ripd(assembler, mul, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            ensure_operand_has_register(assembler, src);
            emit_x_r(assembler, mul, src->oRegister);
        }
        
        allocate_operand_register(assembler, dst);
        emit_r_r(assembler, mov, dst->oRegister, Reg_RAX);
    }
}

internal void
emit_div(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        dst->oImmediate /= src->oImmediate;
    }
    else if ((src->kind == AsmOperand_Immediate) && is_pow2(src->oImmediate))
    {
        ensure_operand_has_register(assembler, dst);
        emit_r_ib(assembler, shr, dst->oRegister, log2(src->oImmediate));
    }
    else
    {
        b32 signedDiv = false;
        emit_rax_extend(assembler, Reg_RAX, signedDiv); // NOTE(michiel): Extend rax to rdx
        emit_operand_to_register(assembler, dst, Reg_RAX);
        
        if (src->kind == AsmOperand_FrameOffset)
        {
            emit_x_frame_offset(div, src->oFrameOffset);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_x_ripd(assembler, div, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            ensure_operand_has_register(assembler, src);
            emit_x_r(assembler, div, src->oRegister);
        }
        
        allocate_operand_register(assembler, dst);
        emit_r_r(assembler, mov, dst->oRegister, Reg_RAX);
    }
}

internal void
emit_expression(Assembler *assembler, Expression *expression, AsmOperand *destination)
{
    i_expect(destination->kind == AsmOperand_None);
    switch (expression->kind)
    {
        case Expr_Int: {
            destination->kind = AsmOperand_Immediate;
            destination->oImmediate = expression->intConst;
        } break;
        
        case Expr_Identifier: {
            fprintf(stderr, "\nERRROROORROROROORRRR:\nMichiel moet dit nog doen...\n\n");
            FileStream errors = {};
            errors.file = gFileApi->open_file(string("stderr"), FileOpen_Write);
            print_expression(&errors, expression);
            fprintf(stderr, "\n");
            destination->kind = AsmOperand_Address;
            destination->oAddress = 0x11223344;
        } break;
        
        case Expr_Unary: {
            emit_expression(assembler, expression->unary.operand, destination);
            switch (expression->unary.op)
            {
                case Unary_Minus: {
                    emit_neg(assembler, destination);
                } break;
                
                case Unary_Flip: {
                    emit_not(assembler, destination);
                } break;
                
                case Unary_Not: {
                    emit_logical_not(assembler, destination);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        } break;
        
        case Expr_Binary: {
            emit_expression(assembler, expression->binary.left, destination);
            AsmOperand rightOp = {};
            emit_expression(assembler, expression->binary.right, &rightOp);
            switch (expression->binary.op)
            {
                case Binary_Add: {
                    emit_add(assembler, destination, &rightOp);
                } break;
                
                case Binary_Sub: {
                    emit_sub(assembler, destination, &rightOp);
                } break;
                
                case Binary_Mul: {
                    emit_mul(assembler, destination, &rightOp);
                } break;
                
                case Binary_Div: {
                    emit_div(assembler, destination, &rightOp);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
            deallocate_operand(assembler, &rightOp);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
emit_statement(Assembler *assembler, Statement *statement)
{
    switch (statement->kind)
    {
        case Stmt_Assign: {
            fprintf(stderr, "\nERRROROORROROROORRRR:\nMichiel moet dit nog doen...\n\n");
            FileStream errors = {};
            errors.file = gFileApi->open_file(string("stderr"), FileOpen_Write);
            print_statement(&errors, statement);
            fprintf(stderr, "\n");
            AsmOperand destination = {};
            AsmOperand source = {};
            emit_expression(assembler, statement->assign.left, &destination);
            emit_expression(assembler, statement->assign.right, &source);
            switch (statement->assign.op)
            {
                case Assign_Set: { emit_mov(assembler, &destination, &source); } break;
                case Assign_Add: { emit_add(assembler, &destination, &source); } break;
                case Assign_Sub: { emit_sub(assembler, &destination, &source); } break;
                case Assign_Mul: { emit_mul(assembler, &destination, &source); } break;
                case Assign_Div: { emit_div(assembler, &destination, &source); } break;
                INVALID_DEFAULT_CASE;
            }
            deallocate_operand(assembler, &destination);
            deallocate_operand(assembler, &source);
        } break;
        
        case Stmt_Return: {
            AsmOperand destination = {};
            if (statement->expression) {
                emit_expression(assembler, statement->expression, &destination);
                emit_operand_to_register(assembler, &destination, Reg_RAX);
            }
            emit_pop(assembler, Reg_EBP);
            emit_ret(assembler);
            deallocate_operand(assembler, &destination);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
emit_stmt_block(Assembler *assembler, StmtBlock *block)
{
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        emit_statement(assembler, block->statements[stmtIdx]);
    }
}

internal umm
emit_function(Assembler *assembler, Function *function)
{
    umm result = assembler->codeAt;
    
    // TODO(michiel): This preamble should be applied for bigger functions only, when Reg_RSP will be used...,
    // so to properly deal with this, this should be known by the AST itself.
    emit_push(assembler, Reg_EBP);
    emit_r_r(assembler, mov, Reg_RBP, Reg_RSP);
    emit_stmt_block(assembler, function->body);
    
    return result;
}

internal umm
emit_program(Assembler *assembler, Program *program)
{
    return emit_function(assembler, program->main);
}

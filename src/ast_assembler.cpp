// TODO(michiel): Add Token to error
internal void
ast_assemble_error(SourcePos origin, char *fmt, va_list args)
{
    if (origin.filename.size)
    {
        fprintf(stderr, "AST->ASM::ERROR::%.*s:%d:%d\n\t", STR_FMT(origin.filename), origin.line, origin.column);
    }
    else
    {
        fprintf(stderr, "AST->ASM::ERROR::%d:%d\n\t", origin.line, origin.column);
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

internal void
ast_assemble_error(SourcePos origin, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ast_assemble_error(origin, fmt, args);
    va_end(args);
}

#define emit_r_frame_offset(a, op, offset, dst) \
if (is_8bit(offset)) { \
emit_r_mb(a, op, dst, Reg_RBP, (offset)); \
} else { \
i_expect(is_32bit(offset)); \
emit_r_md(a, op, dst, Reg_RBP, (offset)); \
}

#define emit_x_frame_offset(a, op, offset) \
if (is_8bit(offset)) { \
emit_x_mb(a, op, Reg_RBP, (offset)); \
} else { \
i_expect(is_32bit(offset)); \
emit_x_md(a, op, Reg_RBP, (offset)); \
}

// TODO(michiel): Add RAX and RDX and check usage in div/mul, maybe use a OC_XchgRMtoR to swap them if needed
// NOTE(michiel): RAX/RDX are very volatile
//                RBX, R12-R15 are callee-saved, so try not to use them
global Register gUsableRegisters[] = {Reg_RCX, Reg_RSI, Reg_RDI, Reg_R8, Reg_R9, Reg_R10, Reg_R11, Reg_RAX, Reg_RDX};

internal void emit_mov(Assembler *assembler, AsmOperand *dst, AsmOperand *src);

internal void
initialize_free_registers(Assembler *assembler)
{
    for (size_t i = 0; i < array_count(gUsableRegisters); i++) {
        assembler->freeRegisterMask |= 1 << (gUsableRegisters[i] & 0xF);
    }
}

internal b32
is_register_used(Assembler *assembler, Register reg)
{
    return !(assembler->freeRegisterMask & (1 << (reg & 0xF)));
}

internal void
deallocate_register(Assembler *assembler, Register oldReg)
{
    //fprintf(stderr, "Free 0x%02X\n", oldReg & 0xF);
    i_expect((assembler->freeRegisterMask & (1 << (oldReg & 0xF))) == 0);
    assembler->freeRegisterMask |= (1 << (oldReg & 0xF));
    AsmRegUser *user = assembler->registerUsers + (oldReg & 0xF);
    if (user->kind == AsmReg_Symbol)
    {
        AsmSymbol *symbol = user->symbol;
        i_expect(symbol);
        symbol->loadedRegister = Reg_None;
    }
    user->kind = AsmReg_None;
    user->hold = false;
    user->symbol = 0;
}

internal void
save_symbol(Assembler *assembler, AsmSymbol *symbol)
{
    i_expect(symbol->loadedRegister != Reg_None);
    if (symbol->unsaved)
    {
        AsmRegUser *usage = assembler->registerUsers + (symbol->loadedRegister & 0xF);
        i_expect(symbol->loadedRegister == usage->reg);
        AsmOperand source = {};
        source.kind = AsmOperand_Register;
        source.oRegister = usage->reg;
        emit_mov(assembler, &symbol->operand, &source);
        symbol->unsaved = false;
    }
}

internal void
save_all_registers(Assembler *assembler)
{
    // NOTE(michiel): Should only be called after a statement evaluation. It saves all remaining symbols
    // to their respective address
    for (u32 index = 0; index < array_count(gUsableRegisters); ++index)
    {
        Register reg = gUsableRegisters[index];
        if (is_register_used(assembler, reg))
        {
            AsmRegUser *user = assembler->registerUsers + (reg & 0xF);
            i_expect(user->kind == AsmReg_Symbol);
            save_symbol(assembler, user->symbol);
            deallocate_register(assembler, reg);
        }
    }
}

internal void
drop_all_local_registers(Assembler *assembler)
{
    // NOTE(michiel): Should only be called after a statement evaluation. It just frees registers used by local symbols.
    for (u32 index = 0; index < array_count(gUsableRegisters); ++index)
    {
        Register reg = gUsableRegisters[index];
        if (is_register_used(assembler, reg))
        {
            AsmRegUser *user = assembler->registerUsers + (reg & 0xF);
            i_expect(user->kind == AsmReg_Symbol);
            if (user->symbol->operand.kind == AsmOperand_FrameOffset)
            {
                deallocate_register(assembler, reg);
            }
        }
    }
}

internal AsmRegUser *
allocate_register(Assembler *assembler, Register reg = Reg_None)
{
    // TODO(michiel): Round robin allocation
    Register targetReg = reg;
    if ((reg == Reg_None) || is_register_used(assembler, reg))
    {
        //BitScanResult firstFree = find_least_significant_set_bit(assembler->freeRegisterMask);
        BitScanResult firstFree = find_most_significant_set_bit(assembler->freeRegisterMask);
        if (!firstFree.found)
        {
            b32 error = true;
            // TODO(michiel): Check first if we can free up the requested reg
            for (size_t i = 0; i < array_count(gUsableRegisters); i++) {
                AsmRegUser *user = assembler->registerUsers + (gUsableRegisters[i] & 0xF);
                i_expect(user->kind != AsmReg_None);
                if ((user->kind == AsmReg_Symbol) &&
                    !user->hold &&
                    !user->symbol->unsaved)
                {
                    i_expect(user->symbol->loadedRegister == gUsableRegisters[i]);
                    targetReg = user->symbol->loadedRegister;
                    user->symbol->loadedRegister = Reg_None;
                    user->kind = AsmReg_None;
                    assembler->freeRegisterMask |= 1 << (gUsableRegisters[i] & 0xF);
                    error = false;
                    break;
                }
            }
            
            if (error)
            {
                for (size_t i = 0; i < array_count(gUsableRegisters); i++) {
                    AsmRegUser *user = assembler->registerUsers + (gUsableRegisters[i] & 0xF);
                    i_expect(user->kind != AsmReg_None);
                    if (user->kind == AsmReg_Symbol && !user->hold)
                    {
                        i_expect(user->symbol->loadedRegister == gUsableRegisters[i]);
                        save_symbol(assembler, user->symbol);
                        targetReg = user->symbol->loadedRegister;
                        user->symbol->loadedRegister = Reg_None;
                        user->kind = AsmReg_None;
                        assembler->freeRegisterMask |= 1 << (gUsableRegisters[i] & 0xF);
                        error = false;
                        break;
                    }
                }
                
                if (error)
                {
                    fprintf(stderr, "No free registers!!\n");
                    return 0;
                }
            }
        }
        else
        {
            targetReg = (Register)(Reg_RAX | firstFree.index);
        }
    }
    
    if ((reg != Reg_None) && (targetReg != reg))
    {
        i_expect(!is_register_used(assembler, targetReg));
        emit_r_r(assembler, mov, targetReg, reg);
        AsmRegUser *oldUser = assembler->registerUsers + (reg & 0xF);
        
        assembler->freeRegisterMask &= ~(1 << (targetReg & 0xF));
        AsmRegUser *newUser = assembler->registerUsers + (targetReg & 0xF);
        *newUser = *oldUser;
        
        i_expect(newUser->reg == reg);
        newUser->reg = targetReg;
        
        oldUser->kind = AsmReg_None;
        
        if (newUser->kind == AsmReg_Symbol)
        {
            newUser->symbol->loadedRegister = targetReg;
        }
        
        oldUser->kind = AsmReg_None;
        
        targetReg = reg;
    }
    
    assembler->freeRegisterMask &= ~(1 << (targetReg & 0xF));
    AsmRegUser *result = assembler->registerUsers + (targetReg & 0xF);
    i_expect(result->kind == AsmReg_None);
    result->kind = AsmReg_AllocatedReg;
    result->reg  = targetReg;
    
    return result;
}

internal void
steal_operand_register(Assembler *assembler, AsmOperand *source, AsmOperand *dest)
{
    i_expect((source->kind == AsmOperand_Register) || (source->kind == AsmOperand_LockedRegister));
    i_expect((dest->kind != AsmOperand_Register) && (dest->kind != AsmOperand_LockedRegister));
    
    AsmRegUser *user = assembler->registerUsers + (source->oRegister & 0xF);
    i_expect((user->kind == AsmReg_TempOperand) &&
             (user->operand == source));
    
    dest->kind = source->kind;
    dest->oRegister = source->oRegister;
    source->kind = AsmOperand_None;
    
    user->operand = dest;
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
            if (is_32bit(operand->oImmediate)) {
                emit_r_i(assembler, movsx, targetReg, operand->oImmediate);
            } else {
                emit_mov64_r_i(assembler, targetReg, operand->oImmediate);
            }
        } break;
        
        case AsmOperand_FrameOffset: {
            emit_r_frame_offset(assembler, mov, operand->oFrameOffset, targetReg);
        } break;
        
        case AsmOperand_Address: {
            emit_r_ripd(assembler, mov, targetReg, 0);
            patch_with_operand_address(assembler, 1, operand);
        } break;
        
        case AsmOperand_LockedRegister:
        case AsmOperand_Register: {
            if (operand->oRegister != targetReg) {
                emit_r_r(assembler, mov, targetReg, operand->oRegister);
            }
        } break;
        
        case AsmOperand_None: {
            // NOTE(michiel): Just reserve the register
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

// NOTE(michiel): This will copy the register if it is a locked one
internal void
ensure_operand_has_register_for_dst(Assembler *assembler, AsmOperand *operand, Register reg = Reg_None)
{
    if ((reg != Reg_None) &&
        (operand->kind == AsmOperand_LockedRegister) &&
        (operand->oRegister == reg))
    {
        AsmRegUser *swapReg = allocate_register(assembler);
        AsmRegUser *reqReg  = assembler->registerUsers + (reg & 0xF);
        i_expect(swapReg);
        i_expect(swapReg->reg != Reg_None);
        Register tempReg = swapReg->reg;
        *swapReg = *reqReg;
        swapReg->reg = tempReg;
        
        if (swapReg->kind == AsmReg_Symbol)
        {
            i_expect(swapReg->symbol->loadedRegister == reg);
            swapReg->symbol->loadedRegister = tempReg;
        }
        
        reqReg->kind = AsmReg_TempOperand;
        reqReg->operand = operand;
        emit_operand_to_register(assembler, operand, reg);
        operand->kind = AsmOperand_Register;
        operand->oRegister = reg;
    }
    else if ((operand->kind != AsmOperand_Register) ||
             ((reg != Reg_None) && (reg != operand->oRegister)))
    {
        AsmRegUser *operandUsage = allocate_register(assembler, reg);
        i_expect(operandUsage);
        i_expect(operandUsage->reg != Reg_None);
        operandUsage->kind = AsmReg_TempOperand;
        operandUsage->operand = operand;
        emit_operand_to_register(assembler, operand, operandUsage->reg);
        operand->kind = AsmOperand_Register;
        operand->oRegister = operandUsage->reg;
    }
}

// NOTE(michiel): This will not copy the register if it is a locked one
internal void
ensure_operand_has_register_for_src(Assembler *assembler, AsmOperand *operand)
{
    if ((operand->kind != AsmOperand_Register) &&
        (operand->kind != AsmOperand_LockedRegister))
    {
        AsmRegUser *operandUsage = allocate_register(assembler);
        i_expect(operandUsage);
        i_expect(operandUsage->reg != Reg_None);
        operandUsage->kind = AsmReg_TempOperand;
        operandUsage->operand = operand;
        emit_operand_to_register(assembler, operand, operandUsage->reg);
        operand->kind = AsmOperand_Register;
        operand->oRegister = operandUsage->reg;
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
        ensure_operand_has_register_for_dst(assembler, operand);
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
        ensure_operand_has_register_for_dst(assembler, operand);
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
        ensure_operand_has_register_for_dst(assembler, operand);
        emit_r_i(assembler, cmp, operand->oRegister, 0);
        emit_r_r(assembler, xor, operand->oRegister, operand->oRegister);
        emit_c_r(assembler, set, CC_Equal, operand->oRegister);
    }
}

internal void
emit_mov(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    i_expect(dst->kind != AsmOperand_Immediate);
    if ((dst->kind == AsmOperand_Register) ||
        (dst->kind == AsmOperand_LockedRegister))
    {
        // TODO(michiel): What if they are the same already?
        ensure_operand_has_register_for_dst(assembler, dst);
        if (src->kind == AsmOperand_Immediate)
        {
            if (is_32bit(src->oImmediate))
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
            emit_r_frame_offset(assembler, mov, src->oFrameOffset, dst->oRegister);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_r_ripd(assembler, mov, dst->oRegister, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            i_expect((src->kind == AsmOperand_Register) || (src->kind == AsmOperand_LockedRegister));
            if (dst->oRegister != src->oRegister) {
                emit_r_r(assembler, mov, dst->oRegister, src->oRegister);
            }
        }
    }
    else if (dst->kind == AsmOperand_FrameOffset)
    {
        if (is_8bit(dst->oFrameOffset))
        {
            if (src->kind == AsmOperand_Immediate)
            {
                i_expect(is_32bit(src->oImmediate));
                emit_mb_i(assembler, movsx, Reg_RBP, dst->oFrameOffset, src->oImmediate);
            }
            else
            {
                ensure_operand_has_register_for_src(assembler, src);
                emit_mb_r(assembler, mov, Reg_RBP, dst->oFrameOffset, src->oRegister);
            }
        }
        else
        {
            i_expect(is_32bit(dst->oFrameOffset));
            if (src->kind == AsmOperand_Immediate)
            {
                i_expect(is_32bit(src->oImmediate));
                emit_md_i(assembler, movsx, Reg_RBP, dst->oFrameOffset, src->oImmediate);
            }
            else
            {
                ensure_operand_has_register_for_src(assembler, src);
                emit_md_r(assembler, mov, Reg_RBP, dst->oFrameOffset, src->oRegister);
            }
        }
    }
    else
    {
        i_expect(dst->kind == AsmOperand_Address);
        if (src->kind == AsmOperand_Immediate)
        {
            i_expect(is_32bit(src->oImmediate));
            emit_ripd_i(assembler, movsx, 0, src->oImmediate);
            patch_with_operand_address(assembler, 2, dst); // TODO(michiel): Check
        }
        else
        {
            ensure_operand_has_register_for_src(assembler, src);
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
    else if ((dst->kind == AsmOperand_Immediate) && is_32bit(dst->oImmediate))
    {
        ensure_operand_has_register_for_dst(assembler, src);
        emit_r_i(assembler, add, src->oRegister, dst->oImmediate);
        steal_operand_register(assembler, src, dst);
    }
    else
    {
        ensure_operand_has_register_for_dst(assembler, dst);
        
        switch (src->kind)
        {
            case AsmOperand_Immediate: {
                if (is_8bit(src->oImmediate))
                {
                    emit_r_ib(assembler, add, dst->oRegister, src->oImmediate);
                }
                else if (is_32bit(src->oImmediate))
                {
                    emit_r_i(assembler, add, dst->oRegister, src->oImmediate);
                }
                else
                {
                    ensure_operand_has_register_for_src(assembler, src);
                    emit_r_r(assembler, add, dst->oRegister, src->oRegister);
                }
            } break;
            
            case AsmOperand_FrameOffset: {
                emit_r_frame_offset(assembler, add, src->oFrameOffset, dst->oRegister);
            } break;
            
            case AsmOperand_Address: {
                emit_r_ripd(assembler, add, dst->oRegister, 0);
                patch_with_operand_address(assembler, 1, src);
            } break;
            
            case AsmOperand_LockedRegister:
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
        ensure_operand_has_register_for_dst(assembler, dst);
        
        switch (src->kind)
        {
            case AsmOperand_Immediate: {
                if (is_8bit(src->oImmediate))
                {
                    emit_r_ib(assembler, sub, dst->oRegister, src->oImmediate);
                }
                else if (is_32bit(src->oImmediate))
                {
                    emit_r_i(assembler, sub, dst->oRegister, src->oImmediate);
                }
                else
                {
                    ensure_operand_has_register_for_src(assembler, src);
                    emit_r_r(assembler, sub, dst->oRegister, src->oRegister);
                }
            } break;
            
            case AsmOperand_FrameOffset: {
                emit_r_frame_offset(assembler, sub, src->oFrameOffset, dst->oRegister);
            } break;
            
            case AsmOperand_Address: {
                emit_r_ripd(assembler, sub, dst->oRegister, 0);
                patch_with_operand_address(assembler, 1, src);
            } break;
            
            case AsmOperand_LockedRegister:
            case AsmOperand_Register: {
                emit_r_r(assembler, sub, dst->oRegister, src->oRegister);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}

internal void
emit_mul_kernel(Assembler *assembler, AsmOperand *src)
{
    if (src->kind == AsmOperand_FrameOffset)
    {
        emit_x_frame_offset(assembler, imul, src->oFrameOffset);
    }
    else if (src->kind == AsmOperand_Address)
    {
        emit_x_ripd(assembler, imul, 0);
        patch_with_operand_address(assembler, 1, src);
    }
    else
    {
        ensure_operand_has_register_for_src(assembler, src);
        emit_x_r(assembler, imul, src->oRegister);
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
        ensure_operand_has_register_for_dst(assembler, dst);
        emit_r_ib(assembler, shl, dst->oRegister, log2(src->oImmediate));
    }
    else if ((dst->kind == AsmOperand_Immediate) && is_pow2(dst->oImmediate))
    {
        ensure_operand_has_register_for_dst(assembler, src);
        emit_r_ib(assembler, shl, src->oRegister, log2(dst->oImmediate));
        steal_operand_register(assembler, src, dst);
    }
    else
    {
        // 1. RAX/RDX are not used, dst == X    => alloc RAX, temp use RDX, mul, ret RAX
        // 2. RAX is used, RDX not, dst == X    => alloc REG0, swap REG RAX, temp use RDX, mul, swap REG0 RAX, ret REG0
        // 3. RDX is used, RAX not, dst == X    => alloc RAX, alloc REG0, swap REG0 RDX, mul, swap REG0 RDX, dealloc REG0, ret RAX
        // 4. RAX/RDX are used    , dst == X    => alloc REG0, swap REG0 RAX, alloc REG1, swap REG1 RDX, mul, swap REG1 RDX, dealloc REG1, swap REG0 RAX, ret REG0
        // 5. RAX is used, RDX not, dst == RAX  => temp use RDX, mul, ret RAX
        // 6. RDX is used, RAX not, dst == RDX  => alloc RAX, mov RAX RDX, mul, dealloc RDX, ret RAX
        // 7. RAX/RDX are used    , dst == RAX  => alloc REG, swap REG RDX, mul, swap REG RDX, dealloc REG, ret RAX
        // 8. RAX/RDX are used    , dst == RDX  => alloc REG, swap REG RAX, mov RAX RDX, mul, swap REG RAX, dealloc RDX, ret REG
        ensure_operand_has_register_for_dst(assembler, dst, Reg_RAX);
        AsmOperand temp = {};
        ensure_operand_has_register_for_dst(assembler, &temp, Reg_RDX);
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_mul_kernel(assembler, src);
        
        deallocate_operand(assembler, &temp);
    }
}

internal void
emit_div_kernel(Assembler *assembler, AsmOperand *src)
{
    b32 signedDiv = true;
    emit_rax_extend(assembler, Reg_RAX, signedDiv); // NOTE(michiel): Extend rax to rdx
    
    if (src->kind == AsmOperand_FrameOffset)
    {
        emit_x_frame_offset(assembler, idiv, src->oFrameOffset);
    }
    else if (src->kind == AsmOperand_Address)
    {
        emit_x_ripd(assembler, idiv, 0);
        patch_with_operand_address(assembler, 1, src);
    }
    else
    {
        ensure_operand_has_register_for_src(assembler, src);
        i_expect(src->oRegister != Reg_RDX);
        emit_x_r(assembler, idiv, src->oRegister);
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
        ensure_operand_has_register_for_dst(assembler, dst);
        emit_r_ib(assembler, shr, dst->oRegister, log2(src->oImmediate));
    }
    else
    {
        ensure_operand_has_register_for_dst(assembler, dst, Reg_RAX);
        AsmOperand temp = {};
        ensure_operand_has_register_for_dst(assembler, &temp, Reg_RDX);
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_div_kernel(assembler, src);
        
        deallocate_operand(assembler, &temp);
    }
}

internal void
emit_mod(Assembler *assembler, AsmOperand *dst, AsmOperand *src)
{
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        dst->oImmediate %= src->oImmediate;
    }
    else if ((src->kind == AsmOperand_Immediate) && is_pow2(src->oImmediate))
    {
        // TODO(michiel): emit_ripd_ib / emit_mb_ib / emit_md_id
        ensure_operand_has_register_for_dst(assembler, dst);
        emit_r_ib(assembler, and, dst->oRegister, src->oImmediate - 1);
    }
    else
    {
        ensure_operand_has_register_for_dst(assembler, dst, Reg_RAX);
        AsmOperand temp = {};
        ensure_operand_has_register_for_dst(assembler, &temp, Reg_RDX);
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(temp.oRegister == Reg_RDX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_div_kernel(assembler, src);
        
        deallocate_operand(assembler, dst);
        dst->kind = AsmOperand_Register;
        dst->oRegister = temp.oRegister;
        AsmRegUser *user = assembler->registerUsers + (temp.oRegister & 0xF);
        i_expect(user->reg == temp.oRegister);
        user->kind = AsmReg_TempOperand;
        user->operand = dst;
    }
}

internal ConditionCode
emit_cmp(Assembler *assembler, BinaryOpType op, AsmOperand *dst, AsmOperand *src)
{
    // NOTE(michiel): Only updates the flag register, doesn't move out the value
    ConditionCode result = CC_Invalid;
    if ((dst->kind == AsmOperand_Immediate) && (src->kind == AsmOperand_Immediate))
    {
        switch (op)
        {
            case Binary_Eq: { dst->oImmediate = dst->oImmediate == src->oImmediate; } break;
            case Binary_Ne: { dst->oImmediate = dst->oImmediate != src->oImmediate; } break;
            case Binary_Lt: { dst->oImmediate = dst->oImmediate <  src->oImmediate; } break;
            case Binary_Gt: { dst->oImmediate = dst->oImmediate >  src->oImmediate; } break;
            case Binary_LE: { dst->oImmediate = dst->oImmediate <= src->oImmediate; } break;
            case Binary_GE: { dst->oImmediate = dst->oImmediate >= src->oImmediate; } break;
            INVALID_DEFAULT_CASE;
        }
    }
    else if ((dst->kind == AsmOperand_Immediate) && is_32bit(dst->oImmediate))
    {
        // TODO(michiel): Could use a 'test reg, reg' for the eq 0 case
        ensure_operand_has_register_for_src(assembler, src);
        emit_r_i(assembler, cmp, src->oRegister, dst->oImmediate);
        steal_operand_register(assembler, src, dst);
        
        switch (op)
        {
            case Binary_Eq: { result = CC_Equal; } break;
            case Binary_Ne: { result = CC_NotEqual; } break;
            case Binary_Lt: { result = CC_GreaterOrEqual; } break;
            case Binary_Gt: { result = CC_LessOrEqual; } break;
            case Binary_LE: { result = CC_Greater; } break;
            case Binary_GE: { result = CC_Less; } break;
            INVALID_DEFAULT_CASE;
        }
    }
    else
    {
        ensure_operand_has_register_for_src(assembler, dst);
        
        switch (src->kind)
        {
            case AsmOperand_Immediate: {
                if (is_8bit(src->oImmediate))
                {
                    emit_r_ib(assembler, cmp, dst->oRegister, src->oImmediate);
                }
                else if (is_32bit(src->oImmediate))
                {
                    emit_r_i(assembler, cmp, dst->oRegister, src->oImmediate);
                }
                else
                {
                    ensure_operand_has_register_for_src(assembler, src);
                    emit_r_r(assembler, cmp, dst->oRegister, src->oRegister);
                }
            } break;
            
            case AsmOperand_FrameOffset: {
                emit_r_frame_offset(assembler, cmp, src->oFrameOffset, dst->oRegister);
            } break;
            
            case AsmOperand_Address: {
                emit_r_ripd(assembler, cmp, dst->oRegister, 0);
                patch_with_operand_address(assembler, 1, src);
            } break;
            
            case AsmOperand_LockedRegister:
            case AsmOperand_Register: {
                emit_r_r(assembler, cmp, dst->oRegister, src->oRegister);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        switch (op)
        {
            case Binary_Eq: { result = CC_Equal; } break;
            case Binary_Ne: { result = CC_NotEqual; } break;
            case Binary_Lt: { result = CC_Less; } break;
            case Binary_Gt: { result = CC_Greater; } break;
            case Binary_LE: { result = CC_LessOrEqual; } break;
            case Binary_GE: { result = CC_GreaterOrEqual; } break;
            INVALID_DEFAULT_CASE;
        }
    }
    return result;
}

internal ConditionCode
emit_expression(Assembler *assembler, Expression *expression, AsmOperand *destination, AsmExprContext context)
{
    ConditionCode result = CC_Invalid;
    // NOTE(michiel): context is only used for binary compare expressions
    i_expect(destination->kind == AsmOperand_None);
    switch (expression->kind)
    {
        case Expr_Int: {
            destination->kind = AsmOperand_Immediate;
            destination->oImmediate = expression->eIntConst;
        } break;
        
        case Expr_Identifier: {
            AsmSymbol *symbol = find_symbol(assembler, expression->eName);
            if (symbol)
            {
                *destination = symbol->operand;
                i_expect(destination->symbol);
                i_expect((destination->kind != AsmOperand_Register) &&
                         (destination->kind != AsmOperand_LockedRegister));
                
                if (symbol->loadedRegister != Reg_None)
                {
                    destination->kind = AsmOperand_LockedRegister;
                    destination->oRegister = symbol->loadedRegister;
                }
            }
            else
            {
                ast_assemble_error(expression->origin, "Identifier '%.*s' was not declared.\n", STR_FMT(expression->eName));
                //FileStream errors = {};
                //errors.file = gFileApi->open_file(string("stderr"), FileOpen_Write);
                //print_expression(&errors, expression);
                //fprintf(stderr, "\n");
            }
        } break;
        
        case Expr_Unary: {
            emit_expression(assembler, expression->eUnary.operand, destination, context);
            switch (expression->eUnary.op)
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
            emit_expression(assembler, expression->eBinary.left, destination, context);
            AsmOperand rightOp = {};
            emit_expression(assembler, expression->eBinary.right, &rightOp, context);
            switch (expression->eBinary.op)
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
                
                case Binary_Mod: {
                    emit_mod(assembler, destination, &rightOp);
                } break;
                
                case Binary_Eq:
                case Binary_Ne:
                case Binary_Lt:
                case Binary_Gt:
                case Binary_LE:
                case Binary_GE: {
                    BinaryOpType op = expression->eBinary.op;
                    if (context == AsmExpr_ToValue)
                    {
                        AsmRegUser *destReg = allocate_register(assembler);
                        emit_r_r(assembler, xor, destReg->reg, destReg->reg);
                        result = emit_cmp(assembler, op, destination, &rightOp);
                        emit_c_r(assembler, set, result, (Register)((destReg->reg & 0xF) | Reg_AL));
                        deallocate_operand(assembler, destination);
                        destination->kind = AsmOperand_Register;
                        destination->oRegister = destReg->reg;
                        destReg->kind = AsmReg_TempOperand;
                        destReg->operand = destination;
                    }
                    else
                    {
                        i_expect(context == AsmExpr_ToCompare);
                        result = emit_cmp(assembler, op, destination, &rightOp);
                    }
                } break;
                
                INVALID_DEFAULT_CASE;
            }
            deallocate_operand(assembler, &rightOp);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

internal void emit_stmt_block(Assembler *assembler, StmtBlock *block);

internal void
emit_statement(Assembler *assembler, Statement *statement)
{
    switch (statement->kind)
    {
        case Stmt_Assign: {
            if (statement->sAssign.left->kind == Expr_Identifier)
            {
                AsmOperand source = {};
                emit_expression(assembler, statement->sAssign.right, &source, AsmExpr_ToValue);
                
                AsmSymbol *leftSym = find_symbol(assembler, statement->sAssign.left->eName);
                if (!leftSym)
                {
                    if (statement->sAssign.op == Assign_Set) {
                        leftSym = create_local_sym(assembler, AsmSymbol_Var, statement->sAssign.left->eName);
                    } else {
                        ast_assemble_error(statement->origin, "'%.*s' not previously declared.\n", STR_FMT(statement->sAssign.left->eName));
                    }
                }
                i_expect(leftSym);
                
                AsmOperand destination = {};
                if (leftSym->loadedRegister)
                {
                    i_expect(is_register_used(assembler, leftSym->loadedRegister));
                    destination.kind = AsmOperand_Register;
                    destination.oRegister = leftSym->loadedRegister;
                    destination.symbol = leftSym;
                }
                else
                {
                    destination = leftSym->operand;
                }
                AsmOperand origDest = destination;
                
                switch (statement->sAssign.op)
                {
                    case Assign_Set: {
                        if ((destination.kind == AsmOperand_Register) &&
                            (source.kind == AsmOperand_Register))
                        {
                            // NOTE(michiel): Just steal the register where the result of the right-hand expression
                            // is located.
                            deallocate_register(assembler, destination.oRegister);
                            destination.kind = AsmOperand_None;
                            steal_operand_register(assembler, &source, &destination);
                        }
                        else
                        {
                            emit_mov(assembler, &destination, &source);
                            if (source.kind == AsmOperand_Register) {
                                AsmRegUser *user = assembler->registerUsers + (source.oRegister & 0xF);
                                i_expect(user->kind == AsmReg_TempOperand);
                                user->kind = AsmReg_Symbol;
                                user->symbol = leftSym;
                                leftSym->loadedRegister = source.oRegister;
                                i_expect(user->reg == source.oRegister);
                                source.kind = AsmOperand_None;
                            }
                        }
                    } break;
                    case Assign_Add: { emit_add(assembler, &destination, &source); } break;
                    case Assign_Sub: { emit_sub(assembler, &destination, &source); } break;
                    case Assign_Mul: { emit_mul(assembler, &destination, &source); } break;
                    case Assign_Div: { emit_div(assembler, &destination, &source); } break;
                    case Assign_Mod: { emit_mod(assembler, &destination, &source); } break;
                    INVALID_DEFAULT_CASE;
                }
                
                leftSym->unsaved |= origDest != destination;
                if (destination == leftSym->operand)
                {
                    leftSym->unsaved = false;
                }
                
                if ((destination.kind == AsmOperand_Register) || (destination.kind == AsmOperand_LockedRegister))
                {
                    if ((leftSym->loadedRegister != Reg_None) &&
                        (leftSym->loadedRegister != destination.oRegister))
                    {
                        deallocate_register(assembler, leftSym->loadedRegister);
                    }
                    leftSym->loadedRegister = destination.oRegister;
                    AsmRegUser *regUsage = assembler->registerUsers + (destination.oRegister & 0xF);
                    if (regUsage->kind == AsmReg_Symbol)
                    {
                        i_expect(regUsage->symbol == leftSym);
                    }
                    else
                    {
                        i_expect(regUsage->kind == AsmReg_TempOperand);
                        i_expect(regUsage->operand == &destination);
                    }
                    regUsage->kind = AsmReg_Symbol;
                    regUsage->symbol = leftSym;
                }
                
                deallocate_operand(assembler, &source);
            }
            else
            {
                ast_assemble_error(statement->origin, "Left side of assignment must be an identifier.\n");
            }
        } break;
        
        case Stmt_Return: {
            AsmOperand destination = {};
            
            Expression *expr = statement->sExpression;
            if (expr)
            {
                emit_expression(assembler, expr, &destination, AsmExpr_ToValue);
                
                if (is_register_used(assembler, Reg_RAX) &&
                    ((destination.kind == AsmOperand_Register) || (destination.kind == AsmOperand_LockedRegister)) &&
                    (destination.oRegister != Reg_RAX))
                {
                    AsmRegUser *offender = assembler->registerUsers + (Reg_RAX & 0xF);
                    i_expect(offender->kind == AsmReg_Symbol);
                    if (offender->symbol->operand.kind != AsmOperand_FrameOffset) {
                        i_expect(offender->symbol->operand.kind == AsmOperand_Address);
                        save_symbol(assembler, offender->symbol);
                    }
                    deallocate_register(assembler, Reg_RAX);
                }
                emit_operand_to_register(assembler, &destination, Reg_RAX);
            }
            
            drop_all_local_registers(assembler);
            save_all_registers(assembler);
            
            //emit_pop(assembler, Reg_EBP);
            emit_ret(assembler);
            deallocate_operand(assembler, &destination);
        } break;
        
        case Stmt_IfElse: {
            // TODO(michiel): Cache last compare
            AsmOperand destination = {};
            ConditionCode cc = emit_expression(assembler, statement->sIf.ifBlock.condition, &destination, AsmExpr_ToCompare);
            deallocate_operand(assembler, &destination);
            i_expect(cc != CC_Invalid);
            emit_c_i(assembler, j, invert_condition(cc), 0);
            u32 ifStart = assembler->codeAt;
            emit_stmt_block(assembler, statement->sIf.ifBlock.block);
            
            u32 *endJumps = 0;
            u32 ifEnd = assembler->codeAt;
            if (statement->sIf.elseBlock || statement->sIf.elIfCount)
            {
                emit_i(assembler, jmp, 0);
                ifEnd = assembler->codeAt;
                mbuf_push(endJumps, ifEnd);
            }
            s32 diff = ifEnd - ifStart;
            *(s32 *)(assembler->codeData.data + ifStart - 4) = diff;
            
            for (u32 elifIdx = 0; elifIdx < statement->sIf.elIfCount; ++elifIdx)
            {
                AsmOperand ccResult = {};
                ConditionBlock *elif = statement->sIf.elIfBlocks + elifIdx;
                cc = emit_expression(assembler, elif->condition, &ccResult, AsmExpr_ToCompare);
                deallocate_operand(assembler, &ccResult);
                i_expect(cc != CC_Invalid);
                emit_c_i(assembler, j, invert_condition(cc), 0);
                u32 elifStart = assembler->codeAt;
                emit_stmt_block(assembler, elif->block);
                
                u32 elifEnd = assembler->codeAt;
                if (statement->sIf.elseBlock || (elifIdx < (statement->sIf.elIfCount - 1)))
                {
                    emit_i(assembler, jmp, 0);
                    elifEnd = assembler->codeAt;
                    mbuf_push(endJumps, elifEnd);
                }
                s32 elifDiff = elifEnd - elifStart;
                *(s32 *)(assembler->codeData.data + elifStart - 4) = elifDiff;
            }
            
            if (statement->sIf.elseBlock)
            {
                emit_stmt_block(assembler, statement->sIf.elseBlock);
            }
            
            u32 codeAt = assembler->codeAt;
            for (u32 backpatchIdx = 0; backpatchIdx < mbuf_count(endJumps); ++backpatchIdx)
            {
                u32 address = endJumps[backpatchIdx];
                s32 diff = codeAt - address;
                *(s32 *)(assembler->codeData.data + address - 4) = diff;
            }
            
            mbuf_deallocate(endJumps);
        } break;
        
        case Stmt_DoWhile: {
            AsmOperand doResult = {};
            u32 doStart = assembler->codeAt;
            emit_stmt_block(assembler, statement->sDo.block);
            ConditionCode cc = emit_expression(assembler, statement->sDo.condition, &doResult, AsmExpr_ToCompare);
            deallocate_operand(assembler, &doResult);
            i_expect(cc != CC_Invalid);
            emit_c_i(assembler, j, cc, 0);
            s32 diff = doStart - assembler->codeAt;
            *(s32 *)(assembler->codeData.data + assembler->codeAt - 4) = diff;
        } break;
        
        case Stmt_While: {
            AsmOperand whileResult = {};
            emit_i(assembler, jmp, 0);
            u32 doStart = assembler->codeAt;
            emit_stmt_block(assembler, statement->sWhile.block);
            
            s32 jmpTo = assembler->codeAt - doStart;
            *(s32 *)(assembler->codeData.data + doStart - 4) = jmpTo;
            
            ConditionCode cc = emit_expression(assembler, statement->sWhile.condition, &whileResult, AsmExpr_ToCompare);
            deallocate_operand(assembler, &whileResult);
            i_expect(cc != CC_Invalid);
            emit_c_i(assembler, j, cc, 0);
            s32 diff = doStart - assembler->codeAt;
            *(s32 *)(assembler->codeData.data + assembler->codeAt - 4) = diff;
        } break;
        
        case Stmt_For: {
            // TODO(michiel): Special case this and keep the loop counter in a reg??
            i_expect(statement->sFor.init->kind == Stmt_Assign);
            emit_statement(assembler, statement->sFor.init);
            
            AsmSymbol *counterSym = find_symbol(assembler, statement->sFor.init->sAssign.left->eName);
            AsmOperand counter = {};
            i_expect(counterSym);
            if (counterSym->loadedRegister == Reg_None)
            {
                counter = counterSym->operand;
                ensure_operand_has_register_for_dst(assembler, &counter);
                counterSym->loadedRegister = counter.oRegister;
                counterSym->unsaved = true;
            }
            else
            {
                counter.kind = AsmOperand_Register;
                counter.oRegister = counterSym->loadedRegister;
            }
            
            AsmRegUser *counterUse = assembler->registerUsers + (counter.oRegister & 0xF);
            i_expect(counterUse->kind == AsmReg_TempOperand);
            i_expect(counterUse->reg == counterSym->loadedRegister);
            counterUse->kind = AsmReg_Symbol;
            counterUse->hold = true;
            counterUse->symbol = counterSym;
            
            emit_i(assembler, jmp, 0);
            u32 forStart = assembler->codeAt;
            emit_stmt_block(assembler, statement->sFor.body);
            
            i_expect(statement->sFor.next->kind == Stmt_Assign);
            emit_statement(assembler, statement->sFor.next);
            AsmSymbol *nextSym = find_symbol(assembler, statement->sFor.next->sAssign.left->eName);
            i_expect(nextSym == counterSym);
            i_expect(nextSym->loadedRegister != Reg_None);
            
            s32 jmpTo = assembler->codeAt - forStart;
            *(s32 *)(assembler->codeData.data + forStart - 4) = jmpTo;
            
            AsmOperand condResult = {};
            ConditionCode cc = emit_expression(assembler, statement->sFor.condition, &condResult, AsmExpr_ToCompare);
            deallocate_operand(assembler, &condResult);
            i_expect(cc != CC_Invalid);
            emit_c_i(assembler, j, cc, 0);
            s32 diff = forStart - assembler->codeAt;
            *(s32 *)(assembler->codeData.data + assembler->codeAt - 4) = diff;
            
            // TODO(michiel): Do we need to save the counter?
            // If we should implement different type of for loops than we could do without, it would be
            // a register value only.
            save_symbol(assembler, counterSym);
            deallocate_register(assembler, counterSym->loadedRegister);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
emit_stmt_block(Assembler *assembler, StmtBlock *block)
{
    u32 localScopeAt = assembler->localCount;
    u32 freeRegisters = assembler->freeRegisterMask;
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        emit_statement(assembler, block->statements[stmtIdx]);
    }
    u32 usedRegisters = ~assembler->freeRegisterMask & freeRegisters;
    for (u32 reg = Reg_RAX; reg <= Reg_R15; ++reg)
    {
        if (usedRegisters & (1 << (reg & 0xF)))
        {
            AsmRegUser *user = assembler->registerUsers + (reg & 0xF);
            i_expect(user->kind == AsmReg_Symbol);
            save_symbol(assembler, user->symbol);
            deallocate_register(assembler, user->reg);
        }
    }
    assembler->localCount = localScopeAt;
}

internal umm
emit_function(Assembler *assembler, Function *function)
{
    umm result = assembler->codeAt;
    
    AsmSymbol *funcSymbol = create_global_sym(assembler, AsmSymbol_Func, function->name, result);
    unused(funcSymbol);
    
    // TODO(michiel): This preamble should be applied for bigger functions only, when Reg_RSP will be used...,
    // so to properly deal with this, this should be known by the AST itself.
    //emit_push(assembler, Reg_EBP);
    emit_stmt_block(assembler, function->body);
    
    return result;
}

internal umm
emit_program(Assembler *assembler, Program *program)
{
    return emit_function(assembler, program->main);
}

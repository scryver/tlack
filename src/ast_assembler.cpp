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

#define emit_r_frame_offset(op, offset, dst) \
if (is_8bit(offset)) { \
emit_r_mb(assembler, op, dst, Reg_RBP, (offset)); \
} else { \
i_expect(is_32bit(offset)); \
emit_r_md(assembler, op, dst, Reg_RBP, (offset)); \
}

#define emit_x_frame_offset(op, offset) \
if (is_8bit(offset)) { \
emit_x_mb(assembler, op, Reg_RBP, (offset)); \
} else { \
i_expect(is_32bit(offset)); \
emit_x_md(assembler, op, Reg_RBP, (offset)); \
}

internal void
initialize_free_registers(Assembler *assembler)
{
    // TODO(michiel): Add RAX and RDX and check usage in div/mul, maybe use a OC_XchgRMtoR to swap them if needed
    // NOTE(michiel): RAX/RDX are very volatile
    //                RBX, R12-R15 are callee-saved, so try not to use them
    Register available_registers[] = {Reg_RCX, Reg_RSI, Reg_RDI, Reg_R8, Reg_R9, Reg_R10, Reg_R11, Reg_RAX, Reg_RDX};
    for (size_t i = 0; i < sizeof(available_registers) / sizeof(*available_registers); i++) {
        assembler->freeRegisterMask |= 1 << (available_registers[i] & 0xF);
    }
}

internal b32
is_register_used(Assembler *assembler, Register reg)
{
    return !(assembler->freeRegisterMask & (1 << (reg & 0xF)));
}

internal Register
allocate_register(Assembler *assembler)
{
    i_expect(assembler->freeRegisterMask);
    //BitScanResult firstFree = find_least_significant_set_bit(assembler->freeRegisterMask);
    BitScanResult firstFree = find_most_significant_set_bit(assembler->freeRegisterMask);
    i_expect(firstFree.found);
    assembler->freeRegisterMask &= ~(1 << firstFree.index);
    Register result = (Register)(Reg_RAX | firstFree.index);
    fprintf(stderr, "Alloc 0x%02X\n", result & 0xF);
    return result;
}

internal Register
allocate_register(Assembler *assembler, Register reg)
{
    fprintf(stderr, "Alloc R 0x%02X\n", reg & 0xF);
    i_expect(!is_register_used(assembler, reg));
    assembler->freeRegisterMask &= ~(1 << (reg & 0xF));
    Register result = reg;
    return result;
}

internal void
deallocate_register(Assembler *assembler, Register oldReg)
{
    fprintf(stderr, "Free 0x%02X\n", oldReg & 0xF);
    i_expect((assembler->freeRegisterMask & (1 << (oldReg & 0xF))) == 0);
    assembler->freeRegisterMask |= (1 << (oldReg & 0xF));
    assembler->registerUsage[oldReg & 0xF] = 0;
}

internal void
steal_operand_register(Assembler *assembler, AsmOperand *source, AsmOperand *dest)
{
    i_expect((source->kind == AsmOperand_Register) || (source->kind == AsmOperand_LockedRegister));
    i_expect((dest->kind != AsmOperand_Register) && (dest->kind != AsmOperand_LockedRegister));
    i_expect(assembler->registerUsage[source->oRegister & 0xF] == source);
    dest->kind = source->kind;
    dest->oRegister = source->oRegister;
    source->kind = AsmOperand_None;
    assembler->registerUsage[dest->oRegister & 0xF] = dest;
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
            emit_r_frame_offset(mov, operand->oFrameOffset, targetReg);
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
        
        INVALID_DEFAULT_CASE;
    }
    assembler->registerUsage[targetReg & 0xF] = operand;
}

// NOTE(michiel): This will copy the register if it is a locked one
internal void
ensure_operand_has_register_for_dst(Assembler *assembler, AsmOperand *operand)
{
    if (operand->kind != AsmOperand_Register)
    {
        Register operandReg = allocate_register(assembler);
        emit_operand_to_register(assembler, operand, operandReg);
        operand->kind = AsmOperand_Register;
        operand->oRegister = operandReg;
    }
}

// NOTE(michiel): This will not copy the register if it is a locked one
internal void
ensure_operand_has_register_for_src(Assembler *assembler, AsmOperand *operand)
{
    if ((operand->kind != AsmOperand_Register) &&
        (operand->kind != AsmOperand_LockedRegister))
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
            emit_r_frame_offset(mov, src->oFrameOffset, dst->oRegister);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_r_ripd(assembler, mov, dst->oRegister, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            i_expect((src->kind == AsmOperand_Register) || (src->kind == AsmOperand_LockedRegister));
            emit_r_r(assembler, mov, dst->oRegister, src->oRegister);
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
                emit_r_frame_offset(add, src->oFrameOffset, dst->oRegister);
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
                emit_r_frame_offset(sub, src->oFrameOffset, dst->oRegister);
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
        emit_x_frame_offset(imul, src->oFrameOffset);
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
        if (!is_register_used(assembler, Reg_RAX))
        {
            Register rax = allocate_register(assembler, Reg_RAX);
            emit_operand_to_register(assembler, dst, rax);
            deallocate_operand(assembler, dst);
            dst->kind = AsmOperand_Register;
            dst->oRegister = rax;
        }
        else
        {
            ensure_operand_has_register_for_dst(assembler, dst);
            if (dst->oRegister != Reg_RAX)
            {
                AsmOperand *offender = assembler->registerUsage[Reg_RAX & 0xF];
                i_expect(offender);
                i_expect((offender->kind == AsmOperand_Register) ||
                         (offender->kind == AsmOperand_LockedRegister));
                i_expect(offender->oRegister == Reg_RAX);
                
                if (offender->symbol && (offender->symbol->loadedRegister == Reg_RAX))
                {
                    offender->symbol->loadedRegister = dst->oRegister;
                }
                emit_r_r(assembler, xchg, dst->oRegister, offender->oRegister);
                offender->oRegister = dst->oRegister;
                dst->oRegister = Reg_RAX;
                assembler->registerUsage[offender->oRegister & 0xF] = offender;
            }
        }
        
        if (is_register_used(assembler, Reg_RDX))
        {
            Register temp = allocate_register(assembler);
            AsmOperand *offender = assembler->registerUsage[Reg_RDX & 0xF];
            i_expect(offender);
            i_expect((offender->kind == AsmOperand_Register) ||
                     (offender->kind == AsmOperand_LockedRegister));
            i_expect(offender->oRegister == Reg_RDX);
            
            if (offender->symbol && (offender->symbol->loadedRegister == Reg_RDX))
            {
                offender->symbol->loadedRegister = temp;
            }
            emit_r_r(assembler, xchg, temp, offender->oRegister);
            offender->oRegister = temp;
            assembler->registerUsage[offender->oRegister & 0xF] = offender;
        }
        else
        {
            allocate_register(assembler, Reg_RDX);
        }
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_mul_kernel(assembler, src);
        
        deallocate_register(assembler, Reg_RDX);
        assembler->registerUsage[Reg_RAX & 0xF] = dst;
        assembler->registerUsage[Reg_RDX & 0xF] = 0;
    }
}

internal void
emit_div_kernel(Assembler *assembler, AsmOperand *src)
{
    b32 signedDiv = true;
    emit_rax_extend(assembler, Reg_RAX, signedDiv); // NOTE(michiel): Extend rax to rdx
    
    if (src->kind == AsmOperand_FrameOffset)
    {
        emit_x_frame_offset(idiv, src->oFrameOffset);
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
        if (!is_register_used(assembler, Reg_RAX))
        {
            Register rax = allocate_register(assembler, Reg_RAX);
            emit_operand_to_register(assembler, dst, rax);
            deallocate_operand(assembler, dst);
            dst->kind = AsmOperand_Register;
            dst->oRegister = rax;
        }
        else
        {
            ensure_operand_has_register_for_dst(assembler, dst);
            if (dst->oRegister != Reg_RAX)
            {
                AsmOperand *offender = assembler->registerUsage[Reg_RAX & 0xF];
                i_expect(offender);
                i_expect(offender != dst);
                i_expect((offender->kind == AsmOperand_Register) ||
                         (offender->kind == AsmOperand_LockedRegister));
                i_expect(offender->oRegister == Reg_RAX);
                
                if (offender->symbol && (offender->symbol->loadedRegister == Reg_RAX))
                {
                    offender->symbol->loadedRegister = dst->oRegister;
                }
                emit_r_r(assembler, xchg, dst->oRegister, offender->oRegister);
                offender->oRegister = dst->oRegister;
                dst->oRegister = Reg_RAX;
                assembler->registerUsage[offender->oRegister & 0xF] = offender;
            }
        }
        
        if (is_register_used(assembler, Reg_RDX))
        {
            Register temp = allocate_register(assembler);
            AsmOperand *offender = assembler->registerUsage[Reg_RDX & 0xF];
            i_expect(offender);
            i_expect((offender->kind == AsmOperand_Register) ||
                     (offender->kind == AsmOperand_LockedRegister));
            i_expect(offender->oRegister == Reg_RDX);
            
            if (offender->symbol && (offender->symbol->loadedRegister == Reg_RDX))
            {
                offender->symbol->loadedRegister = temp;
            }
            emit_r_r(assembler, mov, temp, offender->oRegister);
            offender->oRegister = temp;
            assembler->registerUsage[temp & 0xF] = offender;
        }
        else
        {
            allocate_register(assembler, Reg_RDX);
        }
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_div_kernel(assembler, src);
        
        deallocate_register(assembler, Reg_RDX);
        assembler->registerUsage[Reg_RAX & 0xF] = dst;
        assembler->registerUsage[Reg_RDX & 0xF] = 0;
#if 0        
        if (swapA)
        {
            // TODO(michiel): Find the operand that uses this and change it's register
            ensure_operand_has_register_for_dst(assembler, dst);
            emit_r_r(assembler, xchg, Reg_RAX, dst->oRegister);
        }
        else
        {
            Register rax = allocate_register(assembler, Reg_RAX);
            emit_operand_to_register(assembler, dst, rax);
            dst->kind = AsmOperand_Register;
            dst->oRegister = rax;
        }
        
        if (swapD)
        {
            // TODO(michiel): Find the operand that uses this and change it's register
            tempD = allocate_register(assembler);
            emit_r_r(assembler, xchg, Reg_RDX, tempD);
        }
        
        b32 signedDiv = true;
        emit_rax_extend(assembler, Reg_RAX, signedDiv); // NOTE(michiel): Extend rax to rdx
        
        if (src->kind == AsmOperand_FrameOffset)
        {
            emit_x_frame_offset(idiv, src->oFrameOffset);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_x_ripd(assembler, idiv, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            ensure_operand_has_register_for_src(assembler, src);
            emit_x_r(assembler, idiv, src->oRegister);
        }
        
        if (swapD)
        {
            emit_r_r(assembler, xchg, Reg_RDX, tempD);
            deallocate_register(assembler, tempD);
        }
        
        if (swapA)
        {
            emit_r_r(assembler, xchg, Reg_RAX, dst->oRegister);
        }
#endif
        
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
        if (!is_register_used(assembler, Reg_RAX))
        {
            Register rax = allocate_register(assembler, Reg_RAX);
            emit_operand_to_register(assembler, dst, rax);
            deallocate_operand(assembler, dst);
            dst->kind = AsmOperand_Register;
            dst->oRegister = rax;
        }
        else
        {
            ensure_operand_has_register_for_dst(assembler, dst);
            if (dst->oRegister != Reg_RAX)
            {
                AsmOperand *offender = assembler->registerUsage[Reg_RAX & 0xF];
                i_expect(offender);
                i_expect((offender->kind == AsmOperand_Register) ||
                         (offender->kind == AsmOperand_LockedRegister));
                i_expect(offender->oRegister == Reg_RAX);
                
                if (offender->symbol && (offender->symbol->loadedRegister == Reg_RAX))
                {
                    offender->symbol->loadedRegister = dst->oRegister;
                }
                emit_r_r(assembler, xchg, dst->oRegister, offender->oRegister);
                offender->oRegister = dst->oRegister;
                dst->oRegister = Reg_RAX;
                assembler->registerUsage[offender->oRegister & 0xF] = offender;
            }
        }
        
        if (is_register_used(assembler, Reg_RDX))
        {
            Register temp = allocate_register(assembler);
            AsmOperand *offender = assembler->registerUsage[Reg_RDX & 0xF];
            i_expect(offender);
            i_expect((offender->kind == AsmOperand_Register) ||
                     (offender->kind == AsmOperand_LockedRegister));
            i_expect(offender->oRegister == Reg_RDX);
            
            if (offender->symbol && (offender->symbol->loadedRegister == Reg_RDX))
            {
                offender->symbol->loadedRegister = temp;
            }
            emit_r_r(assembler, xchg, temp, offender->oRegister);
            offender->oRegister = temp;
            assembler->registerUsage[offender->oRegister & 0xF] = offender;
        }
        else
        {
            allocate_register(assembler, Reg_RDX);
        }
        
        i_expect(dst->kind == AsmOperand_Register);
        i_expect(dst->oRegister == Reg_RAX);
        i_expect(is_register_used(assembler, Reg_RAX));
        i_expect(is_register_used(assembler, Reg_RDX));
        
        emit_div_kernel(assembler, src);
        
        deallocate_register(assembler, Reg_RAX);
        dst->oRegister = Reg_RDX;
        assembler->registerUsage[Reg_RAX & 0xF] = 0;
        assembler->registerUsage[Reg_RDX & 0xF] = dst;
        
#if 0
        b32 swapA = is_register_used(assembler, Reg_RAX);
        b32 swapD = is_register_used(assembler, Reg_RDX);
        Register tempD = Reg_None;
        
        if (swapA)
        {
            // TODO(michiel): Find the operand that uses this and change it's register
            ensure_operand_has_register_for_dst(assembler, dst);
            emit_r_r(assembler, xchg, Reg_RAX, dst->oRegister);
        }
        else
        {
            Register rax = allocate_register(assembler, Reg_RAX);
            emit_operand_to_register(assembler, dst, rax);
            dst->kind = AsmOperand_Register;
            dst->oRegister = rax;
        }
        
        if (swapD)
        {
            // TODO(michiel): Find the operand that uses this and change it's register
            tempD = allocate_register(assembler);
            emit_r_r(assembler, xchg, Reg_RDX, tempD);
        }
        else
        {
            tempD = allocate_register(assembler, Reg_RDX);
        }
        
        b32 signedDiv = true;
        emit_rax_extend(assembler, Reg_RAX, signedDiv); // NOTE(michiel): Extend rax to rdx
        
        if (src->kind == AsmOperand_FrameOffset)
        {
            emit_x_frame_offset(idiv, src->oFrameOffset);
        }
        else if (src->kind == AsmOperand_Address)
        {
            emit_x_ripd(assembler, idiv, 0);
            patch_with_operand_address(assembler, 1, src);
        }
        else
        {
            ensure_operand_has_register_for_src(assembler, src);
            emit_x_r(assembler, idiv, src->oRegister);
        }
        
        if (swapD)
        {
            emit_r_r(assembler, xchg, Reg_RDX, tempD);
        }
        
        if (swapA)
        {
            emit_r_r(assembler, xchg, Reg_RAX, dst->oRegister);
        }
        
        deallocate_operand(assembler, dst);
        dst->kind = AsmOperand_Register;
        dst->oRegister = tempD;
#endif
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
        ensure_operand_has_register_for_dst(assembler, src);
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
        ensure_operand_has_register_for_dst(assembler, dst);
        
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
                emit_r_frame_offset(cmp, src->oFrameOffset, dst->oRegister);
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
            destination->oImmediate = expression->intConst;
        } break;
        
        case Expr_Identifier: {
            AsmSymbol *symbol = find_symbol(assembler, expression->name);
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
                ast_assemble_error(expression->origin, "Identifier '%.*s' was not declared.\n", STR_FMT(expression->name));
                //FileStream errors = {};
                //errors.file = gFileApi->open_file(string("stderr"), FileOpen_Write);
                //print_expression(&errors, expression);
                //fprintf(stderr, "\n");
            }
        } break;
        
        case Expr_Unary: {
            emit_expression(assembler, expression->unary.operand, destination, context);
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
            emit_expression(assembler, expression->binary.left, destination, context);
            AsmOperand rightOp = {};
            emit_expression(assembler, expression->binary.right, &rightOp, context);
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
                
                case Binary_Mod: {
                    emit_mod(assembler, destination, &rightOp);
                } break;
                
                case Binary_Eq:
                case Binary_Ne:
                case Binary_Lt:
                case Binary_Gt:
                case Binary_LE:
                case Binary_GE: {
                    BinaryOpType op = expression->binary.op;
                    if (context == AsmExpr_ToValue)
                    {
                        Register destReg = allocate_register(assembler);
                        emit_r_r(assembler, xor, destReg, destReg);
                        result = emit_cmp(assembler, op, destination, &rightOp);
                        emit_c_r(assembler, set, result, (Register)((destReg & 0xF) | Reg_AL));
                        deallocate_operand(assembler, destination);
                        destination->kind = AsmOperand_Register;
                        destination->oRegister = destReg;
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

internal AsmStatementResult
emit_statement(Assembler *assembler, Statement *statement, AsmStatementResult prevResult)
{
    AsmStatementResult result = {};
    switch (statement->kind)
    {
        case Stmt_Assign: {
            if (statement->assign.left->kind == Expr_Identifier)
            {
                AsmOperand destination = {};
                AsmOperand source = {};
                AsmSymbol *leftSym = find_symbol(assembler, statement->assign.left->name);
                if (!leftSym)
                {
                    leftSym = create_local_sym(assembler, AsmSymbol_Var, statement->assign.left->name);
                }
                i_expect(leftSym);
                
                AsmOperand origDest = leftSym->operand;
                if (prevResult.symbol)
                {
                    // NOTE(michiel): We had a previous result, if this is not to the same variable,
                    // than write out the previous result. Otherwise reuse the result register.
                    if (prevResult.symbol == leftSym)
                    {
                        destination = prevResult.result;
                    }
                    else
                    {
                        emit_mov(assembler, &prevResult.original, &prevResult.result);
                        deallocate_operand(assembler, &prevResult.result);
                        prevResult.symbol->loadedRegister = Reg_None;
                        destination = leftSym->operand;
                    }
                }
                else
                {
                    destination = leftSym->operand;
                }
                
                //emit_expression(assembler, statement->assign.left, &destination, AsmExpr_ToValue);
                emit_expression(assembler, statement->assign.right, &source, AsmExpr_ToValue);
                switch (statement->assign.op)
                {
                    //case Assign_Set: { emit_mov(assembler, &destination, &source); } break;
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
                        }
                    } break;
                    case Assign_Add: { emit_add(assembler, &destination, &source); } break;
                    case Assign_Sub: { emit_sub(assembler, &destination, &source); } break;
                    case Assign_Mul: { emit_mul(assembler, &destination, &source); } break;
                    case Assign_Div: { emit_div(assembler, &destination, &source); } break;
                    case Assign_Mod: { emit_mod(assembler, &destination, &source); } break;
                    INVALID_DEFAULT_CASE;
                }
                
                if (origDest != destination)
                {
                    result.symbol = leftSym;
                    result.original = origDest;
                    result.result = destination;
                    if (destination.kind == AsmOperand_Register)
                    {
                        result.symbol->loadedRegister = destination.oRegister;
                    }
                    else
                    {
                        i_expect(destination.kind != AsmOperand_LockedRegister);
                        result.symbol->loadedRegister = Reg_None;
                    }
                }
                
                deallocate_operand(assembler, &source);
            }
            else
            {
                ast_assemble_error(statement->origin, "Left side of assignment must be an identifier.\n");
#if 0                
                fprintf(stderr, "\nERRROROORROROROORRRR:\nMichiel moet dit nog doen...\n\n");
                FileStream errors = {};
                errors.file = gFileApi->open_file(string("stderr"), FileOpen_Write);
                print_statement(&errors, statement);
                fprintf(stderr, "\n");
#endif
            }
        } break;
        
        case Stmt_Return: {
            AsmOperand destination = {};
            
            Expression *expr = statement->expression;
            if (expr)
            {
                emit_expression(assembler, expr, &destination, AsmExpr_ToValue);
                emit_operand_to_register(assembler, &destination, Reg_RAX);
            }
            
            // NOTE(michiel): Most of the time we return something that has been calculated just before, so
            // this mov after the return expression will make sure the return expression can use the cached
            // register value of the previous line.
            if (prevResult.symbol)
            {
                emit_mov(assembler, &prevResult.original, &prevResult.result);
                deallocate_operand(assembler, &prevResult.result);
                prevResult.symbol->loadedRegister = Reg_None;
            }
            
            //emit_pop(assembler, Reg_EBP);
            emit_ret(assembler);
            deallocate_operand(assembler, &destination);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

internal void
emit_stmt_block(Assembler *assembler, StmtBlock *block)
{
    u32 localScopeAt = assembler->localCount;
    AsmStatementResult prevResult = {};
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        prevResult = emit_statement(assembler, block->statements[stmtIdx], prevResult);
    }
    if (prevResult.original.kind != AsmOperand_None)
    {
        emit_mov(assembler, &prevResult.original, &prevResult.result);
        deallocate_operand(assembler, &prevResult.result);
        prevResult.symbol->loadedRegister = Reg_None;
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

internal void
emit_rex_indexed(Assembler *assembler, u8 rx, u8 base, u8 index)
{
    // TODO(michiel): Clean up switch on upper 4 bits or something, and handle 16bit
    i_expect(rx < RegCount);
    i_expect(base < RegCount);
    i_expect(index < RegCount);
    b32 new8bits = (((rx >= Reg_SPL) && (rx <= Reg_R15B)) ||
                    ((base >= Reg_SPL) && (base <= Reg_R15B)) ||
                    ((index >= Reg_SPL) && (index <= Reg_R15B)));
    b32 extended64 = (rx >= Reg_RAX) || (base >= Reg_RAX) || (index >= Reg_RAX);
    rx    = (rx    & 0xF) >> 3;
    base  = (base  & 0xF) >> 3;
    index = (index & 0xF) >> 3;
    
    u32 byte = (extended64 ? OC_Extend64Mask : 0) | (rx << 2) | (index << 1) | base;
    if (new8bits || byte)
    {
        emit(assembler, OC_New8BitRegs | byte);
    }
}

internal void
emit_rex(Assembler *assembler, u8 rx, u8 base)
{
    emit_rex_indexed(assembler, rx, base, 0);
}

internal void
emit_mod_rx_rm(Assembler *assembler, u8 mod, u8 rx, u8 rm)
{
    i_expect(mod < 4);
    i_expect(rx < RegCount);
    i_expect(rm < RegCount);
    emit(assembler, (mod << 6) | ((rx & 0x7) << 3) | (rm & 0x7));
}

// NOTE(michiel): op rx, reg
internal void
emit_direct(Assembler *assembler, u8 rx, Register reg)
{
    emit_mod_rx_rm(assembler, RegMode_Direct, rx, reg);
}

// NOTE(michiel): op rx, [0x12345678]
internal void
emit_displaced(Assembler *assembler, u8 rx, s32 displacement)
{
    emit_mod_rx_rm(assembler, RegMode_Indirect, rx, Reg_RSP);
    emit_mod_rx_rm(assembler, Scale_X1, Reg_RSP, Reg_RBP);
    emit_u32(assembler, displacement);
}

// NOTE(michiel): op rx, [reg]
internal void
emit_indirect(Assembler *assembler, u8 rx, Register reg)
{
    i_expect((reg & 0x7) != (Reg_RSP & 0x7));
    i_expect((reg & 0x7) != (Reg_RBP & 0x7));
    emit_mod_rx_rm(assembler, RegMode_Indirect, rx, reg);
}

// NOTE(michiel): op rx, [reg + 0x12]
internal void
emit_indirect_byte_displaced(Assembler *assembler, u8 rx, Register reg, s8 displacement)
{
    i_expect((reg & 0x7) != (Reg_RSP & 0x7));
    emit_mod_rx_rm(assembler, RegMode_ByteDisplaced, rx, reg);
    emit(assembler, displacement);
}

// NOTE(michiel): op rx, [reg + 0x12345678]
internal void
emit_indirect_displaced(Assembler *assembler, u8 rx, Register reg, s32 displacement)
{
    i_expect((reg & 0x7) != (Reg_RSP & 0x7));
    emit_mod_rx_rm(assembler, RegMode_Displaced, rx, reg);
    emit_u32(assembler, displacement);
}

// NOTE(michiel): op rx, [rip + 0x12345678]
internal void
emit_indirect_displaced_rip(Assembler *assembler, u8 rx, s32 displacement)
{
    emit_mod_rx_rm(assembler, RegMode_Indirect, rx, Reg_RBP);
    emit_u32(assembler, displacement);
}

// NOTE(michiel): op rx, [base + scale*index]
internal void
emit_indirect_indexed(Assembler *assembler, u8 rx, Register base, Register index, Scale scale)
{
    i_expect((base & 0x7) != (Reg_RBP & 0x7));
    emit_mod_rx_rm(assembler, RegMode_Indirect, rx, Reg_RSP);
    emit_mod_rx_rm(assembler, scale, index, base);
}

// NOTE(michiel): op rx, [base + scale*index + 0x12]
internal void
emit_indirect_indexed_byte_displaced(Assembler *assembler, u8 rx, Register base, Register index, Scale scale, s8 displacement)
{
    emit_mod_rx_rm(assembler, RegMode_ByteDisplaced, rx, Reg_RSP);
    emit_mod_rx_rm(assembler, scale, index, base);
    emit(assembler, displacement);
}

// NOTE(michiel): op rx, [base + scale*index + 0x12345678]
internal void
emit_indirect_indexed_displaced(Assembler *assembler, u8 rx, Register base, Register index, Scale scale, s32 displacement)
{
    emit_mod_rx_rm(assembler, RegMode_Displaced, rx, Reg_RSP);
    emit_mod_rx_rm(assembler, scale, index, base);
    emit_u32(assembler, displacement);
}

// NOTE(michiel): Register dest, variant source
#define emit_r_r(a, op, dst, src) \
emit_rex(a, dst, src); \
emit_##op##_r(a); \
emit_direct(a, dst, src)

#define emit_r_d(a, op, dst, src_disp) \
emit_rex(a, dst, 0); \
emit_##op##_r(a); \
emit_displaced(a, dst, src_disp)

#define emit_r_ripd(a, op, dst, src_disp) \
emit_rex(a, dst, 0); \
emit_##op##_r(a); \
emit_indirect_displaced_rip(a, dst, src_disp)

#define emit_r_m(a, op, dst, src) \
emit_rex(a, dst, src); \
emit_##op##_r(a); \
emit_indirect(a, dst, src)

#define emit_r_mb(a, op, dst, src, src_disp) \
emit_rex(a, dst, src); \
emit_##op##_r(a); \
emit_indirect_byte_displaced(a, dst, src, src_disp)

#define emit_r_md(a, op, dst, src, src_disp) \
emit_rex(a, dst, src); \
emit_##op##_r(a); \
emit_indirect_displaced(a, dst, src, src_disp)

#define emit_r_sib(a, op, dst, src_base, src_index, src_scale) \
emit_rex_indexed(a, dst, src_base, src_index); \
emit_##op##_r(a); \
emit_indirect_indexed(a, dst, src_base, src_index, src_scale)

#define emit_r_sibb(a, op, dst, src_base, src_index, src_scale, src_disp) \
emit_rex_indexed(a, dst, src_base, src_index); \
emit_##op##_r(a); \
emit_indirect_indexed_byte_displaced(a, dst, src_base, src_index, src_scale, src_disp)

#define emit_r_sibd(a, op, dst, src_base, src_index, src_scale, src_disp) \
emit_rex_indexed(a, dst, src_base, src_index); \
emit_##op##_r(a); \
emit_indirect_indexed_displaced(a, dst, src_base, src_index, src_scale, src_disp)

// NOTE(michiel): Variant dest, register source
#define emit_d_r(a, op, dst_disp, src) \
emit_rex(a, src, 0); \
emit_##op##_m(a); \
emit_displaced(a, src, dst_disp)

#define emit_ripd_r(a, op, dst_disp, src) \
emit_rex(a, src, 0); \
emit_##op##_m(a); \
emit_indirect_displaced_rip(a, src, dst_disp)

#define emit_m_r(a, op, dst, src) \
emit_rex(a, src, dst); \
emit_##op##_m(a); \
emit_indirect(a, src, dst)

#define emit_mb_r(a, op, dst, dst_disp, src) \
emit_rex(a, src, dst); \
emit_##op##_m(a); \
emit_indirect_byte_displaced(a, src, dst, dst_disp)

#define emit_md_r(a, op, dst, dst_disp, src) \
emit_rex(a, src, dst); \
emit_##op##_m(a); \
emit_indirect_displaced(a, src, dst, dst_disp)

#define emit_sib_r(a, op, dst_base, dst_index, dst_scale, src) \
emit_rex_indexed(a, src, dst_base, dst_index); \
emit_##op##_m(a); \
emit_indirect_indexed(a, src, dst_base, dst_index, dst_scale)

#define emit_sibb_r(a, op, dst_base, dst_index, dst_scale, dst_disp, src) \
emit_rex_indexed(a, src, dst_base, dst_index); \
emit_##op##_m(a); \
emit_indirect_indexed_byte_displaced(a, src, dst_base, dst_index, dst_scale, dst_disp)

#define emit_sibd_r(a, op, dst_base, dst_index, dst_scale, dst_disp, src) \
emit_rex_indexed(a, src, dst_base, dst_index); \
emit_##op##_m(a); \
emit_indirect_indexed_displaced(a, src, dst_base, dst_index, dst_scale, dst_disp)

// NOTE(michiel): Variant dest, 32b immediate source
#define emit_r_i(a, op, dst, src_imm) \
emit_rex(a, 0, dst); \
emit_##op##_i(a); \
emit_direct(a, Extension_##op##_Imm, dst); \
emit_u32(a, src_imm)

#define emit_d_i(a, op, dst_disp, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_i(a); \
emit_displaced(a, Extension_##op##_Imm, dst_disp); \
emit_u32(a, src_imm)

#define emit_ripd_i(a, op, dst_disp, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_i(a); \
emit_indirect_displaced_rip(a, Extension_##op##_Imm, dst_disp); \
emit_u32(a, src_imm)

#define emit_m_i(a, op, dst, src_imm) \
emit_rex(a, 0, dst); \
emit_##op##_i(a); \
emit_indirect(a, Extension_##op##_Imm, dst); \
emit_u32(a, src_imm)

#define emit_mb_i(a, op, dst, dst_disp, src_imm) \
emit_rex(a, 0, dst); \
emit_##op##_i(a); \
emit_indirect_byte_displaced(a, Extension_##op##_Imm, dst, dst_disp); \
emit_u32(a, src_imm)

#define emit_md_i(a, op, dst, dst_disp, src_imm) \
emit_rex(a, 0, dst); \
emit_##op##_i(a); \
emit_indirect_displaced(a, Extension_##op##_Imm, dst, dst_disp); \
emit_u32(a, src_imm)

#define emit_sib_i(a, op, dst_base, dst_index, dst_scale, src_imm) \
emit_rex_indexed(a, 0, dst_base, dst_index); \
emit_##op##_i(a); \
emit_indirect_indexed(a, Extension_##op##_Imm, dst_base, dst_index, dst_scale); \
emit_u32(a, src_imm)

#define emit_sibb_i(a, op, dst_base, dst_index, dst_scale, dst_disp, src_imm) \
emit_rex_indexed(a, 0, dst_base, dst_index); \
emit_##op##_i(a); \
emit_indirect_indexed_byte_displaced(a, Extension_##op##_Imm, dst_base, dst_index, dst_scale, dst_disp); \
emit_u32(a, src_imm)

#define emit_sibd_i(a, op, dst_base, dst_index, dst_scale, dst_disp, src_imm) \
emit_rex_indexed(a, 0, dst_base, dst_index); \
emit_##op##_i(a); \
emit_indirect_indexed_displaced(a, Extension_##op##_Imm, dst_base, dst_index, dst_scale, dst_disp); \
emit_u32(a, src_imm)

// NOTE(michiel): Variant dest, 8b immediate source
#define emit_r_ib(a, op, dst, src_imm) \
emit_rex(a, 0, dst); \
emit_##op##_ib(a); \
emit_direct(a, Extension_##op##_ImmB, dst); \
emit(a, src_imm)

// NOTE(michiel): Implicit dest, variant source
#define emit_x_r(a, op, src) \
emit_rex(a, 0, src); \
emit_##op##_x(a); \
emit_direct(a, Extension_##op##_X, src)

#define emit_x_d(a, op, src_disp) \
emit_rex(a, 0, 0); \
emit_##op##_x(a); \
emit_displaced(a, Extension_##op##_X, src_disp)

#define emit_x_ripd(a, op, src_disp) \
emit_rex(a, 0, 0); \
emit_##op##_x(a); \
emit_indirect_displaced_rip(a, Extension_##op##_X, src_disp)

#define emit_x_m(a, op, src) \
emit_rex(a, 0, src); \
emit_##op##_x(a); \
emit_indirect(a, Extension_##op##_X, src)

#define emit_x_mb(a, op, src, src_disp) \
emit_rex(a, 0, src); \
emit_##op##_x(a); \
emit_indirect_byte_displaced(a, Extension_##op##_X, src, src_disp)

#define emit_x_md(a, op, src, src_disp) \
emit_rex(a, 0, src); \
emit_##op##_x(a); \
emit_indirect_displaced(a, Extension_##op##_X, src, src_disp)

#define emit_x_sib(a, op, src_base, src_index, src_scale) \
emit_rex_indexed(a, 0, src_base, src_index); \
emit_##op##_x(a); \
emit_indirect_indexed(a, Extension_##op##_X, src_base, src_index, src_scale)

#define emit_x_sibb(a, op, src_base, src_index, src_scale, src_disp) \
emit_rex_indexed(a, 0, src_base, src_index); \
emit_##op##_x(a); \
emit_indirect_indexed_byte_displaced(a, Extension_##op##_X, src_base, src_index, src_scale, src_disp)

#define emit_x_sibd(a, op, src_base, src_index, src_scale, src_disp) \
emit_rex_indexed(a, 0, src_base, src_index); \
emit_##op##_x(a); \
emit_indirect_indexed_displaced(a, Extension_##op##_X, src_base, src_index, src_scale, src_disp)


#define emit_mov64_r_i(a, dst, src_imm) \
emit_rex(a, 0, dst); \
emit(a, OC_MovItoRBase | (dst & 0x7)); \
emit_u64(a, src_imm)

#define emit_mov64_rax_off(a, src_offset) \
emit_rex(a, 0, 0); \
emit(a, OC_MovMOffsToXAX); \
emit_u64(a, src_offset)

#define emit_mov64_off_rax(a, dst_offset) \
emit_rex(a, 0, 0); \
emit(a, OC_MovXAXtoMOffs); \
emit_u64(a, dst_offset)


#define emit_ib(a, op, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_ib(a); \
emit(a, src_imm)

#define emit_i(a, op, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_i(a); \
emit_u32(a, src_imm)

#define emit_c_i(a, op, cond, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_c_i(a, cond); \
emit_u32(a, src_imm)

#define emit_c_ib(a, op, cond, src_imm) \
emit_rex(a, 0, 0); \
emit_##op##_c_ib(a, cond); \
emit(a, src_imm)

#define emit_c_r(a, op, cond, src) \
emit_rex(a, 0, src); \
emit_##op##_c_r(a, cond); \
emit_direct(a, Extension_##op##_C, src)


#define OPERATION_1_R(op, code) \
void emit_##op##_r(Assembler *assembler) { \
emit(assembler, code); \
}

#define OPERATION_2_R(op, code) \
void emit_##op##_r(Assembler *assembler) { \
emit(assembler, 0x0F); \
emit(assembler, code); \
}

#define OPERATION_1_M(op, code) \
void emit_##op##_m(Assembler *assembler) { \
emit(assembler, code); \
}

#define OPERATION_2_M(op, code) \
void emit_##op##_m(Assembler *assembler) { \
emit(assembler, 0x0F); \
emit(assembler, code); \
}

#define OPERATION_1_I(op, code, extension) \
void emit_##op##_i(Assembler *assembler) { \
emit(assembler, code); \
} \
enum { Extension_##op##_Imm = extension };

#define OPERATION_1_IB(op, code, extension) \
void emit_##op##_ib(Assembler *assembler) { \
emit(assembler, code); \
} \
enum { Extension_##op##_ImmB = extension };

#define OPERATION_1_X(op, code, extension) \
void emit_##op##_x(Assembler *assembler) { \
emit(assembler, code); \
} \
enum { Extension_##op##_X = extension };

#define OPERATION_1_C_IB(op, code) \
void emit_##op##_c_ib(Assembler *assembler, ConditionCode cond) { \
emit(assembler, code | cond); \
}

#define OPERATION_2_C_I(op, code) \
void emit_##op##_c_i(Assembler *assembler, ConditionCode cond) { \
emit(assembler, 0x0F); \
emit(assembler, code | cond); \
}

#define OPERATION_2_C_R(op, code, extension) \
void emit_##op##_c_r(Assembler *assembler, ConditionCode cond) { \
emit(assembler, 0x0F); \
emit(assembler, code | cond); \
} \
enum { Extension_##op##_C = extension };



OPERATION_1_R(mov, OC_MovRMtoR)
OPERATION_1_M(mov, OC_MovRtoRM)
OPERATION_1_I(movsx, OC_MovItoRM, 0x00)   // NOTE(michiel): Only sign extended from 32b imm -> 64b reg
OPERATION_1_IB(mov, OC_MovI8toRM8, 0x00)
OPERATION_2_R(movsxb, 0xBE)

// NOTE(michiel): Variable shifts only with CL
OPERATION_1_IB(shl, OC_RotShftRMbyI, 0x04)
OPERATION_1_IB(shr, OC_RotShftRMbyI, 0x05)
OPERATION_1_IB(sar, OC_RotShftRMbyI, 0x07)

OPERATION_1_R(add, OC_AddRMtoR)
OPERATION_1_M(add, OC_AddRtoRM)
OPERATION_1_I(add, OC_OpItoRM, 0x00)
OPERATION_1_IB(add, OC_OpI8toRM, 0x00)

OPERATION_1_R(or, OC_OrRMtoR)
OPERATION_1_M(or, OC_OrRtoRM)
OPERATION_1_I(or, OC_OpItoRM, 0x01)
OPERATION_1_IB(or, OC_OpI8toRM, 0x01)

OPERATION_1_R(adc, OC_AdcRMtoR)
OPERATION_1_M(adc, OC_AdcRtoRM)
OPERATION_1_I(adc, OC_OpItoRM, 0x02)
OPERATION_1_IB(adc, OC_OpI8toRM, 0x02)

OPERATION_1_R(sbb, OC_SbbRMtoR)
OPERATION_1_M(sbb, OC_SbbRtoRM)
OPERATION_1_I(sbb, OC_OpItoRM, 0x03)
OPERATION_1_IB(sbb, OC_OpI8toRM, 0x03)

OPERATION_1_R(and, OC_AndRMtoR)
OPERATION_1_M(and, OC_AndRtoRM)
OPERATION_1_I(and, OC_OpItoRM, 0x04)
OPERATION_1_IB(and, OC_OpI8toRM, 0x04)

OPERATION_1_R(sub, OC_SubRMtoR)
OPERATION_1_M(sub, OC_SubRtoRM)
OPERATION_1_I(sub, OC_OpItoRM, 0x05)
OPERATION_1_IB(sub, OC_OpI8toRM, 0x05)

OPERATION_1_R(xor, OC_XorRMtoR)
OPERATION_1_M(xor, OC_XorRtoRM)
OPERATION_1_I(xor, OC_OpItoRM, 0x06)
OPERATION_1_IB(xor, OC_OpI8toRM, 0x06)

OPERATION_1_R(cmp, OC_CmpRMtoR)
OPERATION_1_M(cmp, OC_CmpRtoRM)
OPERATION_1_I(cmp, OC_OpItoRM, 0x07)
OPERATION_1_IB(cmp, OC_OpI8toRM, 0x07)

OPERATION_1_X(mul, OC_IntArith, 0x04)
OPERATION_1_X(imul, OC_IntArith, 0x05)
OPERATION_1_X(div, OC_IntArith, 0x06)
OPERATION_1_X(idiv, OC_IntArith, 0x07)

OPERATION_1_X(not, OC_IntArith, 0x02)
OPERATION_1_X(neg, OC_IntArith, 0x03)

OPERATION_1_R(xchg, OC_XchgRMtoR)

OPERATION_1_I(jmp, OC_Jump, 0x00)
OPERATION_1_IB(jmp, OC_Jump8, 0x00)

OPERATION_2_C_R(set, 0x90, 0x00)

OPERATION_1_C_IB(j, 0x70)
OPERATION_2_C_I(j, 0x80)

#if 0
// NOTE(michiel): This produces 7-bytes of instructions, a 32 bit immediate with a simple movsx
// will also produces 7-bytes of instructions. The main difference is that this emits 2 opcodes,
// and the 32 bit variant a single opcode.
internal void
emit_mov64_r_ib(Assembler *assembler, Register reg, s64 value)
{
    i_expect(is_8bit(value));
    emit_r_ib(assembler, mov, (Register)((reg & 0xF) | Reg_AL), value);
    emit_r_r(assembler, movsxb, reg, reg);
}
#endif

internal void
emit_mov32_r_i(Assembler *assembler, Register reg, s64 value)
{
    i_expect(is_32bit_unsigned(value));
    if (reg >= Reg_RAX) {
        reg = (Register)((reg & 0xF) | Reg_EAX);
    }
    emit_r_i(assembler, movsx, reg, value);
}

internal void
emit_push(Assembler *assembler, Register reg)
{
    emit_rex(assembler, 0, reg);
    emit(assembler, OC_PushRBase | (reg & 0x7));
}

internal void
emit_pop(Assembler *assembler, Register reg)
{
    emit_rex(assembler, reg, 0);
    emit(assembler, OC_PopRBase | (reg & 0x7));
}

internal void
emit_ret(Assembler *assembler)
{
    emit(assembler, 0xC3);
}

internal void
emit_rax_extend(Assembler *assembler, Register typeReg, b32 signedReg = false)
{
    if (signedReg)
    {
        // NOTE(michiel): Sign extend EDX or RDX
        if (typeReg >= Reg_RAX)
        {
            emit(assembler, 0x48);
        }
        emit(assembler, 0x99);
    }
    else
    {
        // NOTE(michiel): Clear EDX or RDX
        Register rdx = (Register)((typeReg & ~0x15) | 0x2);
        emit_r_r(assembler, xor, rdx, rdx);
    }
}

#if 0
internal b32
emit_reg_prefix(Assembler *assembler, Register dstReg, Register srcReg = Reg_None)
{
    b32 result = false;
    
    b32 is64bit = (dstReg >= Reg_RAX) || (srcReg >= Reg_RAX);
    b32 isExtReg = (dstReg >= Reg_R8D) || (srcReg >= Reg_R8D);
    
    u8 byte = 0x00;
    byte |= isExtReg ? 0x40 : 0x00;
    byte |= is64bit  ? 0x08 : 0x00;
    byte |= ((dstReg >= Reg_R8) || ((dstReg >= Reg_R8D) && (dstReg <= Reg_R15D))) ? 0x1 : 0x0;
    byte |= ((srcReg >= Reg_R8) || ((srcReg >= Reg_R8D) && (srcReg <= Reg_R15D))) ? 0x4 : 0x0;
    
    if (byte)
    {
        emit(assembler, byte);
        result = true;
    }
    
    return result;
}

internal void
emit_ret(Assembler *assembler)
{
    emit(assembler, 0xC3);
}

internal void
emit_mov_imm(Assembler *assembler, Register reg, u64 imm)
{
    // TODO(michiel): Sign extended: mov rax, -1 => 0x48 0xC7 0xC0 0xFF 0xFF 0xFF 0xFF
    // a sign extended 32 bit value
    if (imm > U32_MAX)
    {
        i_expect(reg >= Reg_RAX);
        i_expect(reg <= Reg_R15);
    }
    else if (reg > Reg_R15D)
    {
        reg = (Register)(reg - 16);
    }
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0xB8 | (reg & 0x7));
    
    if (imm > U32_MAX)
    {
        emit_u64(assembler, imm);
    }
    else
    {
        emit_u32(assembler, imm);
    }
}

#if 0
internal void
emit_mov_if_equal(Assembler *assembler, Register srcReg, Register dstReg)
{
    // TODO(michiel): assert(bits(srcReg) == bits(dstReg))
    emit_reg_prefix(assembler, dstReg);
    emit(assembler, 0x0F);
    emit(assembler, 0x44);
    emit(assembler, 0xC0 | ((dstReg & 0x7) << 3) | (srcReg & 0x7));
    
}
#endif

internal void
emit_mov_reg(Assembler *assembler, Register dstReg, Register srcReg)
{
    emit_reg_prefix(assembler, dstReg, srcReg);
    emit(assembler, 0x89);
    emit(assembler, 0xC0 | ((srcReg & 0x7) << 3) | ((dstReg & 0x7) & 0x7));
}

internal void
emit_cmp_imm(Assembler *assembler, Register reg, s64 imm)
{
    i_expect(imm >= S32_MIN);
    i_expect(imm <= S32_MAX);
    emit_reg_prefix(assembler, reg);
    if ((imm >= S8_MIN) && (imm <= S8_MAX))
    {
        emit(assembler, 0x83);
        emit(assembler, 0xF8 | (reg & 0x7));
        emit(assembler, imm & 0xFF);
    }
    else
    {
        emit(assembler, 0x81);
        emit(assembler, 0xF8 | (reg & 0x7));
        emit_u32(assembler, imm & 0xFFFFFFFF);
    }
}

internal void
emit_add_reg(Assembler *assembler, Register dstReg, Register srcReg)
{
    emit_reg_prefix(assembler, dstReg, srcReg);
    emit(assembler, 0x01);
    emit(assembler, 0xC0 | ((srcReg & 0x7) << 3) | (dstReg & 0x7));
}

internal void
emit_sub_reg(Assembler *assembler, Register dstReg, Register srcReg)
{
    emit_reg_prefix(assembler, dstReg, srcReg);
    emit(assembler, 0x29);
    emit(assembler, 0xC0 | ((srcReg & 0x7) << 3) | (dstReg & 0x7));
}

internal void
emit_mul_reg(Assembler *assembler, Register reg, b32 signedMult = false)
{
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0xF7);
    emit(assembler, 0xE0 | (signedMult ? 0x08 : 0x00) | (reg & 0x7));
}

internal void
emit_rax_extend(Assembler *assembler, Register typeReg, b32 signedReg = false)
{
    if (signedReg)
    {
        // NOTE(michiel): Sign extend EDX or RDX
        if (typeReg >= Reg_RAX)
        {
            emit(assembler, 0x48);
        }
        emit(assembler, 0x99);
    }
    else
    {
        // NOTE(michiel): Clear EDX or RDX
        Register rdx = (Register)((typeReg & ~0x15) | 0x2);
        emit_mov_imm(assembler, rdx, 0);
    }
}

internal void
emit_div_reg(Assembler *assembler, Register reg, b32 signedDiv = false)
{
    // NOTE(michiel): RDX is used as well!
    emit_rax_extend(assembler, reg, signedDiv);
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0xF7);
    emit(assembler, 0xF0 | (signedDiv ? 0x08 : 0x00) | (reg & 0x7));
}

internal void
emit_not_reg(Assembler *assembler, Register reg)
{
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0xF7);
    emit(assembler, 0xD0 | (reg & 0x7));
}

internal void
emit_neg_reg(Assembler *assembler, Register reg)
{
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0xF7);
    emit(assembler, 0xD8 | (reg & 0x7));
}

internal void
emit_set_equal(Assembler *assembler, Register reg)
{
    emit_reg_prefix(assembler, reg);
    emit(assembler, 0x0F);
    emit(assembler, 0x94);
    emit(assembler, 0xC0 | (reg & 0x7));
}

internal void
emit_push(Assembler *assembler, Register reg)
{
    i_expect(reg >= Reg_RAX);
    i_expect(reg <= Reg_R15);
    if (reg >= Reg_R8)
    {
        emit(assembler, 0x41);
    }
    emit(assembler, 0x50 | (reg & 0x7));
}

internal void
emit_pop(Assembler *assembler, Register reg)
{
    i_expect(reg >= Reg_RAX);
    i_expect(reg <= Reg_R15);
    if (reg >= Reg_R8)
    {
        emit(assembler, 0x41);
    }
    emit(assembler, 0x58 | (reg & 0x7));
}

//
// NOTE(michiel): Compounds
//

internal void
emit_logical_not(Assembler *assembler, Register reg)
{
    emit_cmp_imm(assembler, reg, 0);
    // TODO(michiel): emit_xor_reg(assembler, reg, reg);
    emit_mov_imm(assembler, reg, 0);
    emit_set_equal(assembler, reg);
}
#endif

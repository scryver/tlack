enum Register
{
    Reg_None,
    
    Reg_AH = 0x04,
    Reg_CH,
    Reg_DH,
    Reg_BH,
    
    Reg_AL = 0x10,
    Reg_CL,
    Reg_DL,
    Reg_BL,
    Reg_SPL,
    Reg_BPL,
    Reg_SIL,
    Reg_DIL,
    Reg_R8B,
    Reg_R9B,
    Reg_R10B,
    Reg_R11B,
    Reg_R12B,
    Reg_R13B,
    Reg_R14B,
    Reg_R15B,
    Reg_AX = 0x20,
    Reg_CX,
    Reg_DX,
    Reg_BX,
    Reg_SP,
    Reg_BP,
    Reg_SI,
    Reg_DI,
    Reg_R8W,
    Reg_R9W,
    Reg_R10W,
    Reg_R11W,
    Reg_R12W,
    Reg_R13W,
    Reg_R14W,
    Reg_R15W,
    Reg_EAX = 0x30,
    Reg_ECX,
    Reg_EDX,
    Reg_EBX,
    Reg_ESP,
    Reg_EBP,
    Reg_ESI,
    Reg_EDI,
    Reg_R8D,
    Reg_R9D,
    Reg_R10D,
    Reg_R11D,
    Reg_R12D,
    Reg_R13D,
    Reg_R14D,
    Reg_R15D,
    Reg_RAX = 0x40,
    Reg_RCX,
    Reg_RDX,
    Reg_RBX,
    Reg_RSP,
    Reg_RBP,
    Reg_RSI,
    Reg_RDI,
    Reg_R8,
    Reg_R9,
    Reg_R10,
    Reg_R11,
    Reg_R12,
    Reg_R13,
    Reg_R14,
    Reg_R15,
    
    RegCount
};

enum RegModeEnum
{
    RegMode_Indirect = 0,
    RegMode_ByteDisplaced = 1,
    RegMode_Displaced = 2,
    RegMode_Direct = 3,
};
typedef u8 RegMode;

enum ScaleEnum
{
    Scale_X1 = 0,
    Scale_X2 = 1,
    Scale_X4 = 2,
    Scale_X8 = 3,
};
typedef u8 Scale;

enum ConditionCodesEnum
{
    CC_Overflow   = 0,
    CC_NoOverflow = 1,
    CC_Below      = 2,   // NOTE(michiel): Below unsigned
    CC_NotBelow   = 3,   // NOTE(michiel): Above or equal unsigned
    CC_Equal      = 4,
    CC_NotEqual   = 5,
    CC_NotAbove   = 6,   // NOTE(michiel): Below or equal unsigned
    CC_Above      = 7,   // NOTE(michiel): Above unsigned
    CC_Sign       = 8,
    CC_NoSign     = 9,
    CC_Parity     = 10,
    CC_NoParity   = 11,
    CC_Less       = 12,   // NOTE(michiel): Less signed
    CC_NotLess    = 13,   // NOTE(michiel): Greater or equal signed
    CC_NotGreater = 14,   // NOTE(michiel): Less or equal signed
    CC_Greater    = 15,   // NOTE(michiel): Greater signed
    CC_NotAboveOrEqual = CC_Below,
    CC_Carry           = CC_Below,
    CC_AboveOrEqual    = CC_NotBelow,
    CC_NoCarry         = CC_NotBelow,
    CC_Zero            = CC_Equal,
    CC_NotZero         = CC_NotEqual,
    CC_BelowOrEqual    = CC_NotAbove,
    CC_NotBelowOrEqual = CC_Above,
    CC_ParityEven      = CC_Parity,
    CC_ParityOdd       = CC_NoParity,
    CC_NotGreaterEqual = CC_Less,
    CC_GreaterOrEqual  = CC_NotLess,
    CC_LessOrEqual     = CC_NotGreater,
    CC_NotLessOrEqual  = CC_Greater,
};
typedef u8 ConditionCode;

// NOTE(michiel): Immediates are 32b max
enum OpCodes
{
    OC_AddR8toRM8     = 0x00,
    OC_AddRtoRM       = 0x01,
    OC_AddRM8toR8     = 0x02,
    OC_AddRMtoR       = 0x03,
    OC_AddI8toAL      = 0x04,
    OC_AddItoXAX      = 0x05,
    // 0x06/0x07 invalid
    OC_OrR8toRM8      = 0x08,
    OC_OrRtoRM        = 0x09,
    OC_OrRM8toR8      = 0x0A,
    OC_OrRMtoR        = 0x0B,
    OC_OrI8toAL       = 0x0C,
    OC_OrItoXAX       = 0x0D,
    // 0x0E invalid
    OC_TwoByteOC      = 0x0F,
    OC_AdcR8toRM8     = 0x10,
    OC_AdcRtoRM       = 0x11,
    OC_AdcRM8toR8     = 0x12,
    OC_AdcRMtoR       = 0x13,
    OC_AdcI8toAL      = 0x14,
    OC_AdcItoXAX      = 0x15,
    // 0x16/0x17 invalid
    OC_SbbR8toRM8     = 0x18,
    OC_SbbRtoRM       = 0x19,
    OC_SbbRM8toR8     = 0x1A,
    OC_SbbRMtoR       = 0x1B,
    OC_SbbI8toAL      = 0x1C,
    OC_SbbItoXAX      = 0x1D,
    // 0x1E, 0x1F invalid
    OC_AndR8toRM8     = 0x20,
    OC_AndRtoRM       = 0x21,
    OC_AndRM8toR8     = 0x22,
    OC_AndRMtoR       = 0x23,
    OC_AndI8toAL      = 0x24,
    OC_AndItoXAX      = 0x25,
    OC_NullPrefix26   = 0x26,
    // 0x27 invalid
    OC_SubR8toRM8     = 0x28,
    OC_SubRtoRM       = 0x29,
    OC_SubRM8toR8     = 0x2A,
    OC_SubRMtoR       = 0x2B,
    OC_SubI8toAL      = 0x2C,
    OC_SubItoXAX      = 0x2D,
    OC_NullPrefix2E   = 0x2E,
    // 0x2F invalid
    OC_XorR8toRM8     = 0x30,
    OC_XorRtoRM       = 0x31,
    OC_XorRM8toR8     = 0x32,
    OC_XorRMtoR       = 0x33,
    OC_XorI8toAL      = 0x34,
    OC_XorItoXAX      = 0x35,
    OC_NullPrefix36   = 0x36,
    // 0x37 invalid
    OC_CmpR8toRM8     = 0x38,
    OC_CmpRtoRM       = 0x39,
    OC_CmpRM8toR8     = 0x3A,
    OC_CmpRMtoR       = 0x3B,
    OC_CmpI8toAL      = 0x3C,
    OC_CmpItoXAX      = 0x3D,
    OC_NullPrefix3E   = 0x3E,
    // 0x3F invalid
    
    OC_ExtendBMask    = 0x01,
    OC_ExtendXMask    = 0x02,
    OC_ExtendRMask    = 0x04,
    OC_Extend64Mask   = 0x08,
    OC_New8BitRegs    = 0x40,
    OC_ExtendB        = 0x41, // Extension of r/m field, base field, or opcode reg field
    OC_ExtendX        = 0x42, // Extension of SIB index field
    OC_ExtendXB       = 0x43,
    OC_ExtendR        = 0x44, // Extension of ModR/M reg field
    OC_ExtendRB       = 0x45,
    OC_ExtendRX       = 0x46,
    OC_ExtendRXB      = 0x47,
    OC_Extend64       = 0x48, // 64 Bit Operand Size
    OC_Extend64B      = 0x49,
    OC_Extend64X      = 0x4A,
    OC_Extend64XB     = 0x4B,
    OC_Extend64R      = 0x4C,
    OC_Extend64RB     = 0x4D,
    OC_Extend64RX     = 0x4E,
    OC_Extend64RXB    = 0x4F,
    
    OC_PushRBase      = 0x50,
    OC_PopRBase       = 0x58,
    
    // 0x60, 0x61, 0x62 invalid
    OC_RegRMtoR_Sext  = 0x63,
    OC_FS_Prefix      = 0x64, // FS segment override prefix
    OC_GS_Prefix      = 0x65, // GS segment override prefix
    OC_OperandSize    = 0x66, // Operand-size override prefix
    OC_PrecisionSize  = 0x66, // Precision-size override prefix
    OC_AddressSize    = 0x67, // Address-size override prefix
    OC_PushI          = 0x68,
    OC_MulRMxItoR     = 0x69,
    OC_PushI8         = 0x6A,
    OC_MulRMxI8toR    = 0x6B,
    
    OC_IOtoString8    = 0x6C, // Copy 8b from io port specified in DX to DS:[RDI++]
    OC_IOtoString     = 0x6D,
    OC_StringToIO8    = 0x6E, // Copy 8b from DS:[RSI++] to io port specified in DX
    OC_StringToIO     = 0x6F,
    
    OC_JmpOverflow    = 0x70, // OF = 1
    OC_JmpNoOverflow  = 0x71, // OF = 0
    OC_JmpCarry       = 0x72, // CF = 1, also JmpBelow, JmpNotAboveOrEq
    OC_JmpNoCarry     = 0x73, // CF = 0, also JmpNotBelow, JmpAboveOrEq
    OC_JmpZero        = 0x74, // ZF = 1, also JmpEqual
    OC_JmpNotZero     = 0x75, // ZF = 0, also JmpNotEqual
    OC_JmpBelowOrEq   = 0x76, // CF = 1 || ZF = 1, also JmpNotAbove
    OC_JmpAbove       = 0x77, // CF = 0 && ZF = 0, also JmpNotBelowOrEq
    OC_JmpSign        = 0x78, // SF = 1
    OC_JmpNoSign      = 0x79, // SF = 0
    OC_JmpParityEven  = 0x7A, // PF = 1, also JmpParity
    OC_JmpParityOdd   = 0x7B, // PF = 0, also JmpNotParity
    OC_JmpLess        = 0x7C, // SF != OF, also JmpNotGreaterOrEq
    OC_JmpGreaterOrEq = 0x7D, // SF = OF, also JmpNotLess
    OC_JmpLessOrEq    = 0x7E, // ZF = 1 || SF != OF, also JmpNotGreater
    OC_JmpGreater     = 0x7F, // ZF = 0 && SF = OF, also JmpNotLessOrEq
    
    OC_OpI8toRM8      = 0x80, // add, or, adc, sbb, and, sub, xor, cmp
    OC_OpItoRM        = 0x81,
    // 0x82 invalid
    OC_OpI8toRM       = 0x83,
    OC_TestR8toRM8    = 0x84,
    OC_TestRtoRM      = 0x85,
    OC_XchgRM8toR8    = 0x86,
    OC_XchgRMtoR      = 0x87,
    OC_MovR8toRM8     = 0x88,
    OC_MovRtoRM       = 0x89,
    OC_MovRM8toR8     = 0x8A,
    OC_MovRMtoR       = 0x8B,
    OC_MovSregToRM    = 0x8C, // M can only be 16bit
    OC_LeaMtoR        = 0x8D,
    OC_MovRM16toSreg  = 0x8E,
    OC_PopRM          = 0x8F,
    OC_XchgXAXtoRBase = 0x90, // also used in pause/spin loop hint (0xF3 0x90)
    OC_SignExtendAtoA = 0x98,
    OC_SignExtendAtoD = 0x99,
    // 0x9A invalid
    OC_FWait          = 0x9B, // float exception check
    OC_PushFlags      = 0x9C, // push rFlags onto stack
    OC_PopFlags       = 0x9D, // pop stack into rFlags
    OC_StoreAHFlags   = 0x9E,
    OC_LoadAHFlags    = 0x9F, 
    
    OC_MovMOffs8ToAL  = 0xA0,
    OC_MovMOffsToXAX  = 0xA1,
    OC_MovALtoMOffs8  = 0xA2,
    OC_MovXAXtoMOffs  = 0xA3,
    OC_MovString8     = 0xA4, // Copy DS:[RSI++] to DS:[RDI++]
    OC_MovString      = 0xA5,
    OC_CmpString8     = 0xA6,
    OC_CmpString      = 0xA7,
    OC_TestI8toAL     = 0xA8,
    OC_TestItoXAX     = 0xA9,
    OC_MovALToString8 = 0xAA, // DS:[RDI++] = AL
    OC_MovXAXToString = 0XAB,
    OC_MovString8ToAL = 0xAC, // AL = DS:[RSI++]
    OC_MovStringToXAX = 0xAD,
    OC_CmpString8ToAL = 0xAE, // AL == DS:[RDI++]
    OC_CmpStringToXAX = 0xAF,
    
    OC_MovI8toR8Base  = 0xB0,
    OC_MovItoRBase    = 0xB8,
    
    OC_RotShftRM8byI8 = 0xC0, // rol, ror, rcl, rcr, shl, shr, sal, sar
    OC_RotShftRMbyI   = 0xC1,
    OC_RetNearI16     = 0xC2,
    OC_RetNear        = 0xC3,
    // 0xC4, 0xC5 invalid
    OC_MovI8toRM8     = 0xC6,
    OC_MovItoRM       = 0xC7,
    OC_Enter          = 0xC8, // setup stack frame for procedure
    OC_Leave          = 0xC9, // exit procedure
    OC_RetFarI16      = 0xCA,
    OC_RetFar         = 0xCB,
    OC_BreakpointTrap = 0xCC, // int3
    OC_SWIntI8        = 0xCD,
    OC_OverflowTrap   = 0xCE, // int0, overflow trap if OF = 1
    OC_IntReturn      = 0xCF,
    
    OC_RotShftRM8by1  = 0xD0,
    OC_RotShftRMby1   = 0xD1,
    OC_RotShftRM8byCL = 0xD2,
    OC_RotShftRMbyCL  = 0xD3,
    // 0xD4, 0xD5, 0xD6 invalid
    OC_TableLookup    = 0xD7, // AL = DS:[RBX + (unsigned)AL]
    
    OC_FloatArith     = 0xD8, // add, mul, cmp, cmp+pop, sub, reverse sub, div, reverser div
    OC_FloatStuff     = 0xD9, // TODO(michiel): Dingen
    OC_FloatMoreStff0 = 0xDA, // TODO(michiel): More dinguh
    OC_FloatMoreStff1 = 0xDB,
    OC_FloatMoreStff2 = 0xDC,
    OC_FloatMoreStff3 = 0xDD,
    OC_FloatMoreStff4 = 0xDE,
    OC_FloatMoreStff5 = 0xDF,
    
    OC_LoopNotZero    = 0xE0, // dec count, jmp if ZF = 0 and count
    OC_LoopZero       = 0xE1, // dec count, jmp if ZF = 1 and count
    OC_Loop           = 0xE2, // dec count, jmp if count
    OC_JmpXCXisZero   = 0xE3, // 
    
    OC_In8I8toAL      = 0xE4,
    OC_InI8toXAX      = 0xE5,
    OC_ALtoOut8I8     = 0xE6,
    OC_XAXtoOutI8     = 0xE7,
    
    OC_Call           = 0xE8,
    OC_Jump           = 0xE9,
    // 0xEA invalid
    OC_Jump8          = 0xEB,
    
    OC_In8DXtoAL      = 0xEC,
    OC_InDXtoXAX      = 0xED,
    OC_ALtoOut8DX     = 0xEE,
    OC_XAXtoOutDX     = 0xEF,
    
    OC_Lock           = 0xF0,
    OC_DebugTrap      = 0xF1, // int1
    
    OC_RepAndNonZero  = 0xF2, // Also SSE2
    OC_RepAndZero     = 0xF3, // Also SSE1
    
    OC_Halt           = 0xF4,
    OC_ComplementCF   = 0xF5, // CF = !CF
    
    OC_IntArith8      = 0xF6, // test rm8 i8, test x, not rm8, neg rm8, mul ax = al*rm8, imul, div al,ah = ax / rm8, idiv
    OC_IntArith       = 0xF7,
    
    OC_ClearCarry     = 0xF8,
    OC_SetCarry       = 0xF9,
    OC_ClearInterrupt = 0xFA,
    OC_SetInterrupt   = 0xFB,
    OC_ClearDirection = 0xFC,
    OC_SetDirection   = 0xFD,
    
    OC_IncDecRM8      = 0xFE,
    OC_IncDecRM       = 0xFF,
    
#if 0    
    OpCode_CompImm    = 0x81,
    OpCode_CmpImmSign = 0x83,
    OpCode_MoveImm    = 0xB8,
    OpCode_MovePre    = 0x89,
    OpCode_SignExtend = 0x99,
    OpCode_Return     = 0xC3,
    OpCode_MulPre     = 0xF7,
    // NOTE(michiel): Register set basis
    OpCode_PushReg    = 0x50,
    OpCode_PopReg     = 0x58,
    OpCode_MoveReg    = 0xC0,
    OpCode_NotReg     = 0xD0,
    OpCode_NegReg     = 0xD8,
    OpCode_MulReg     = 0xE0,
    OpCode_MulRegSign = 0xE8,
    OpCode_DivReg     = 0xF0,
    OpCode_DivRegSign = 0xF8,
    OpCode_CmpReg     = 0xF8,
#endif
};

internal u8
create_modrm_byte(Register r, Register addr)
{
    // NOTE(michiel):
    // result = 0x00 for  [reg]
    // result = 0x40 for  [reg]+disp8
    // result = 0x80 for  [reg]+disp32
    // result = 0xC0 for  reg
    u8 result = 0xC0;
    
    result |= ((r & 0x7) << 3);
    result |= (addr & 0x7);
    
    return result;
    
    // ModRM
    // ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | ((rm & 0x7) << 0)
    // MOD R/M Addressing Mode
    // === === ================================
    // 00 000 [ eax ]
    // 01 000 [ eax + disp8 ]               (1)
    // 10 000 [ eax + disp32 ]
    // 11 000 register  ( al / ax / eax )   (2)
    // 00 001 [ ecx ]
    // 01 001 [ ecx + disp8 ]
    // 10 001 [ ecx + disp32 ]
    // 11 001 register  ( cl / cx / ecx )
    // 00 010 [ edx ]
    // 01 010 [ edx + disp8 ]
    // 10 010 [ edx + disp32 ]
    // 11 010 register  ( dl / dx / edx )
    // 00 011 [ ebx ]
    // 01 011 [ ebx + disp8 ]
    // 10 011 [ ebx + disp32 ]
    // 11 011 register  ( bl / bx / ebx )
    // 00 100 SIB  Mode                     (3)
    // 01 100 SIB  +  disp8  Mode
    // 10 100 SIB  +  disp32  Mode
    // 11 100 register  ( ah / sp / esp )
    // 00 101 32-bit Displacement-Only Mode (4)
    // 01 101 [ ebp + disp8 ]
    // 10 101 [ ebp + disp32 ]
    // 11 101 register  ( ch / bp / ebp )
    // 00 110 [ esi ]
    // 01 110 [ esi + disp8 ]
    // 10 110 [ esi + disp32 ]
    // 11 110 register  ( dh / si / esi )
    // 00 111 [ edi ]
    // 01 111 [ edi + disp8 ]
    // 10 111 [ edi + disp32 ]
    // 11 111 register  ( bh / di / edi )
    
    // r when OC_ExtendRMask is not set
    // r =                       :    0 |    1 |    2 |    3 |    4 |    5 |    6 |    7 |
    // R8   without 0x40 prefix  : AL   | CL   | DL   | BL   | AH   | CH   | DH   | BH   |
    // R8   with any 0x40 prefix : AL   | CL   | DL   | BL   | SPL  | BPL  | SIL  | DIL  |
    // R16                       : AX   | CX   | DX   | BX   | SP   | BP   | SI   | DI   |
    // R32                       : EAX  | ECX  | EDX  | EBX  | ESP  | EBP  | ESI  | EDI  |
    // R64                       : RAX  | RCX  | RDX  | RBX  | RSP  | RBP  | RSI  | RDI  |
    // MM                        : MM0  | MM1  | MM2  | MM0  | MM0  | MM0  | MM0  | MM0  |
    // XMM                       : XMM0 | XMM1 | XMM2 | XMM3 | XMM4 | XMM5 | XMM6 | XMM7 |
    // SREG                      : ES   | CS   | SS   | DS   | FS   | GS   | res  | res  |
    
    // r when OC_ExtendRMask is set
    // r =                       :    0 |    1 |    2 |    3 |    4 |    5 |    6 |    7 |
    // R8   with any 0x40 prefix : R8B  | R9B  | R10B | R11B | R12B | R13B | R14B | R15B |
    // R16                       : R8W  | R9W  | R10W | R11W | R12W | R13W | R14W | R15W |
    // R32                       : R8D  | R9D  | R10D | R11D | R12D | R13D | R14D | R15D |
    // R64                       : R8   | R9   | R10  | R11  | R12  | R13  | R14  | R15  |
    // MM                        : MM0  | MM1  | MM2  | MM0  | MM0  | MM0  | MM0  | MM0  |
    // XMM                       : XMM8 | XMM9 | XMM10| XMM11| XMM12| XMM13| XMM14| XMM15|
    // SREG                      : ES   | CS   | SS   | DS   | FS   | GS   | res  | res  |
    
    // addr when OC_ExtendBMask is not set
    // mod  = 0b00
    // addr = 0  when [RAX/EAX]
    //      = 1  when [RCX/ECX]
    //      = 2  when [RDX/EDX]
    //      = 3  when [RBX/EBX]
    //      = 4  when [SIB following]
    //      = 5  when [RIP/EIP]+disp32
    //      = 6  when [RSI/ESI]
    //      = 7  when [RDI/EDI]
    
    // addr when OC_ExtendBMask is set
    // mod  = 0b00
    // addr = 0  when [R8 /R8D ]
    //      = 1  when [R9 /R9D ]
    //      = 2  when [R10/R10D]
    //      = 3  when [R11/R11D]
    //      = 4  when [SIB following]
    //      = 5  when [RIP/EIP]+disp32
    //      = 6  when [R14/R14D]
    //      = 7  when [R15/R15D]
    
    // addr when OC_ExtendBMask is not set
    // mod  = 0b01
    // addr = 0  when [RAX/EAX]+disp8
    //      = 1  when [RCX/ECX]+disp8
    //      = 2  when [RDX/EDX]+disp8
    //      = 3  when [RBX/EBX]+disp8
    //      = 4  when [SIB following]+disp8
    //      = 5  when [RBP/EBP]+disp8
    //      = 6  when [RSI/ESI]+disp8
    //      = 7  when [RDI/EDI]+disp8
    
    // addr when OC_ExtendBMask is set
    // mod  = 0b01
    // addr = 0  when [R8 /R8D ]+disp8
    //      = 1  when [R9 /R9D ]+disp8
    //      = 2  when [R10/R10D]+disp8
    //      = 3  when [R11/R11D]+disp8
    //      = 4  when [SIB following]+disp8
    //      = 5  when [R13/R13D]+disp8
    //      = 6  when [R14/R14D]+disp8
    //      = 7  when [R15/R15D]+disp8
    
    // addr when OC_ExtendBMask is not set
    // mod  = 0b10
    // addr = 0  when [RAX/EAX]+disp32
    //      = 1  when [RCX/ECX]+disp32
    //      = 2  when [RDX/EDX]+disp32
    //      = 3  when [RBX/EBX]+disp32
    //      = 4  when [SIB following]+disp32
    //      = 5  when [RBP/EBP]+disp32
    //      = 6  when [RSI/ESI]+disp32
    //      = 7  when [RDI/EDI]+disp32
    
    // addr when OC_ExtendBMask is set
    // mod  = 0b10
    // addr = 0  when [R8 /R8D ]+disp32
    //      = 1  when [R9 /R9D ]+disp32
    //      = 2  when [R10/R10D]+disp32
    //      = 3  when [R11/R11D]+disp32
    //      = 4  when [SIB following]+disp32
    //      = 5  when [R13/R13D]+disp32
    //      = 6  when [R14/R14D]+disp32
    //      = 7  when [R15/R15D]+disp32
    
    // addr when OC_ExtendBMask is not set
    // mod  = 0b11
    // addr = 0  when AL/AX/EAX/RAX/ST0/MM0/XMM0
    //      = 1  when CL/CX/ECX/RCX/ST1/MM1/XMM1
    //      = 2  when DL/DX/EDX/RDX/ST2/MM2/XMM2
    //      = 3  when BL/BX/EBX/RBX/ST3/MM3/XMM3
    //      = 4  when AH/SP/ESP/RSP/ST4/MM4/XMM4
    //      = 5  when CH/BP/EBP/RBP/ST5/MM5/XMM5
    //      = 6  when DH/SI/ESI/RSI/ST6/MM6/XMM6
    //      = 7  when BH/DI/EDI/RDI/ST7/MM7/XMM7
    
    // addr when OC_ExtendBMask is set
    // mod  = 0b11
    // addr = 0  when R8B /R8W /R8D /R8 /ST0/MM0/XMM8
    //      = 1  when R9B /R9W /R9D /R9 /ST1/MM1/XMM9
    //      = 2  when R10B/R10W/R10D/R10/ST2/MM2/XMM10
    //      = 3  when R11B/R11W/R11D/R11/ST3/MM3/XMM11
    //      = 4  when R12B/R12W/R12D/R12/ST4/MM4/XMM12
    //      = 5  when R13B/R13W/R13D/R13/ST5/MM5/XMM13
    //      = 6  when R14B/R14W/R14D/R14/ST6/MM6/XMM14
    //      = 7  when R15B/R15W/R15D/R15/ST7/MM7/XMM15
    
    // SIB (Scaled Index Byte)
    // ((scale & 0x3) << 6) | ((index & 0x7) << 3) | ((base & 0x7) << 0)
    // scale 1, 2, 4 or 8
    // index RAX, RCX, RDX, RBX, illegal, RBP, RSI, RDI
    // base  RAX, RCX, RDX, RBX, RSP, disp/RBP, RSI, RDI
    // disp/RBP dependent on mod value
}

struct ModRMByte
{
    u8 mod;
    u8 r;
    u8 rm;
};

internal ModRMByte
decode_modrm_byte(u8 byte)
{
    ModRMByte result = {};
    result.mod = (byte >> 6) & 0x3;
    result.r   = (byte >> 3) & 0x7;
    result.rm  = (byte >> 0) & 0x7;
    return result;
}
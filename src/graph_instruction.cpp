internal void
graph_instruction(FileStream *output, Instruction *instruction)
{
    u8 src1Buf[128];
    u8 src2Buf[128];
    u8 dstBuf[128];
    String src1 = {};
    String src2 = {};
    String dst  = {};
    String label = {};
    
    switch (instruction->kind)
    {
        case Instr_Zero: {
            label = static_string("zero");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
        } break;
        
        case Instr_Move: {
            label = static_string("mov");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        case Instr_Negate: {
            label = static_string("neg");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        case Instr_Invert: {
            label = static_string("inv");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        case Instr_Not: {
            label = static_string("not");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        case Instr_Add: {
            label = static_string("add");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
            src2 = string_from_operand(instruction->src2, sizeof(src2Buf), src2Buf);
        } break;
        
        case Instr_Subtract: {
            label = static_string("sub");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
            src2 = string_from_operand(instruction->src2, sizeof(src2Buf), src2Buf);
        } break;
        
        case Instr_Multiply: {
            label = static_string("mul");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
            src2 = string_from_operand(instruction->src2, sizeof(src2Buf), src2Buf);
        } break;
        
        case Instr_Divide: {
            label = static_string("div");
            dst = string_from_operand(instruction->dest, sizeof(dstBuf), dstBuf);
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
            src2 = string_from_operand(instruction->src2, sizeof(src2Buf), src2Buf);
        } break;
        
        case Instr_Jump: {
            label = static_string("jmp");
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        case Instr_Return: {
            label = static_string("ret");
            src1 = string_from_operand(instruction->src1, sizeof(src1Buf), src1Buf);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    i_expect(label.size);
    u8 instrNameBuf[128];
    String instrName = string_fmt(sizeof(instrNameBuf), instrNameBuf, "instr%p", instruction);
    graph_label(output, instrName, label);
    
    if (dst.size)
    {
        graph_connect(output, instrName, dst);
    }
    if (src1.size)
    {
        graph_connect(output, src1, instrName);
    }
    if (src2.size)
    {
        graph_connect(output, src2, instrName);
    }
}

internal void
graph_instructions(OpBuilder *builder, char *fileName)
{
    FileStream output = {0};
    output.file = open_file(string(fileName), FileOpen_Write);
    
    println(&output, "digraph instructions {");
    ++output.indent;
    
    umm index = 0;
    while (index < builder->instrAt)
    {
        Instruction *instr = (Instruction *)(builder->instrStorage.data + index);
        index += sizeof(Instruction);
        
        graph_instruction(&output, instr);
    }
    
    --output.indent;
    println(&output, "}\n");
    
    close_file(&output.file);
}

#define USE_LINUX_PLATFORM    1
#define USE_STD_PLATFORM      0

#include "../libberdip/src/platform.h"

#if USE_LINUX_PLATFORM
#include "../libberdip/src/linux_memory.h"
#elif USE_STD_PLATFORM
#include "../libberdip/src/std_memory.h"
#else
#error NO PLATFORM DEFINED
#endif

#include "ast.h"

//#include "instruction.h"
#include "assembler.h"
#include "assembler_amd64.h"

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>

global MemoryAPI gMemoryApi_;
global MemoryAPI *gMemoryApi = &gMemoryApi_;

global FileAPI gFileApi_;
global FileAPI *gFileApi = &gFileApi_;

internal Buffer
allocate_buffer(MemoryAllocator *allocator, umm size)
{
    Buffer result = {};
    result.data = (u8 *)allocate_size(allocator, size, default_memory_alloc());
    if (result.data)
    {
        result.size = size;
    }
    return result;
}

internal Buffer
reallocate_buffer(MemoryAllocator *allocator, Buffer oldBuffer, umm newSize)
{
    Buffer result = {};
    result.data = (u8 *)reallocate_size(allocator, newSize, oldBuffer.data, default_memory_alloc());
    if (result.data)
    {
        result.size = newSize;
    }
    return result;
}

internal b32
make_executable(Buffer buffer)
{
    b32 result = false;
    if (buffer.data)
    {
        PlatformMemoryBlock *block = (PlatformMemoryBlock *)((u8 *)buffer.data - sizeof(PlatformMemoryBlock));
        result = gMemoryApi->executable_memory(block);
    }
    return result;
}

#include "../libberdip/src/memory.cpp"
#include "../libberdip/src/files.cpp"

#if USE_LINUX_PLATFORM
#include "../libberdip/src/linux_memory.cpp"
#include "../libberdip/src/linux_file.c"
#elif USE_STD_PLATFORM
#include "../libberdip/src/std_memory.cpp"
#include "../libberdip/src/std_file.c"
#endif

//#include "instruction.cpp"
#include "ast.cpp"
#include "assembler.cpp"
#include "assembler_amd64.cpp"
#include "ast_assembler.cpp"

#include "graphviz.cpp"
#include "graph_ast.cpp"
//#include "graph_instruction.cpp"

#define FUNC_CALL(name)   u64 name(void)
typedef FUNC_CALL(FuncCall);

internal void
dump_code(Assembler *assembler, String filename)
{
    Buffer totalData = {assembler->codeAt, assembler->codeData.data};
    gFileApi->write_entire_file(filename, totalData);
}

internal void
emit_jump_i(Assembler *assembler)
{
    emit(assembler, OC_Jump);
}

s32 main(s32 argc, char **argv)
{
#if USE_LINUX_PLATFORM
    linux_memory_api(gMemoryApi);
    linux_file_api(gFileApi);
#elif USE_STD_PLATFORM
    std_memory_api(gMemoryApi);
    std_file_api(gFileApi);
#endif
    
    MemoryAllocator defaultAlloc = {};
    initialize_platform_allocator(0, &defaultAlloc);
    
    Buffer codeBuild = allocate_buffer(&defaultAlloc, megabytes(1ULL));
    
    Assembler assembler_ = {};
    Assembler *assembler = &assembler_;
    assembler->allocator = &defaultAlloc;
    assembler->codeData = codeBuild;
    assembler->codeAt = kilobytes(4);
    initialize_free_registers(assembler);
    
    Ast mainAst = {};
    
#if 1
    
    // NOTE(michiel): 2 * (3 + 4) / (6 * 7 + 1) + (0 - 4) * 5
    // (2 * (3 + 4)) / ((6 * 7) + 1) + ((0 - 4) * 5)
    
    Expression const2 = create_expression(2);
    Expression const3 = create_expression(3);
    Expression const4a = create_expression(4);
    Expression const6 = create_expression(6);
    Expression const7 = create_expression(7);
    Expression const1 = create_expression(1);
    Expression const0 = create_expression(0);
    Expression const4b = create_expression(4);
    Expression const5 = create_expression(5);
    
    Expression const1234 = create_expression(0x1234);
    Expression named;
    named.kind = Expr_Identifier;
    named.name = static_string("test");
    Statement set1  = create_assign_stmt(Assign_Set, &named, &const1234);
    
    Expression add1 = create_expression(Binary_Add, &const3, &const4a);
    Expression mul1 = create_expression(Binary_Mul, &const2, &add1);
    Expression mul2 = create_expression(Binary_Mul, &const6, &const7);
    Expression add2 = create_expression(Binary_Add, &mul2, &const1);
    Expression div1 = create_expression(Binary_Div, &mul1, &add2);
    Expression sub1 = create_expression(Binary_Sub, &const0, &const4b);
    Expression mul3 = create_expression(Binary_Mul, &sub1, &const5);
    Expression add3 = create_expression(Binary_Add, &div1, &mul3);
    Expression sub2 = create_expression(Binary_Sub, &add3, &named);
    Statement ret1  = create_return_stmt(&sub2);
    
    Statement *list[] = {
        &set1,
        &ret1,
    };
    StmtBlock block = create_statement_block(array_count(list), list);
    Function mainFunc = create_function(minterned_string(&mainAst.interns, "main"), &block);
#else
    Expression constExpr = {};
    constExpr.kind = Expr_Int;
    constExpr.intConst = -9223372036854775807 - 1; // NOTE(michiel): Wauw...
    
    Expression unaryExpr = {};
    unaryExpr.kind = Expr_Unary;
    unaryExpr.unary.op = Unary_Flip;
    unaryExpr.unary.operand = &constExpr;
    
    Expression constExpr2 = {};
    constExpr2.kind = Expr_Int;
    constExpr2.intConst = 7;
    
    Expression binaryAdd = {};
    binaryAdd.kind = Expr_Binary;
    binaryAdd.binary.op = Binary_Mul;
    binaryAdd.binary.left = &unaryExpr;
    binaryAdd.binary.right = &constExpr2;
    
    Expression returnExpr = {};
    returnExpr.kind = Expr_Return;
    returnExpr.returnExpr = &binaryAdd;
    
    mainFunc.body = &returnExpr;
#endif
    
    mainAst.program.main = &mainFunc;
    
    FileStream printStream = {};
    printStream.file = gFileApi->open_file(string("stdout"), FileOpen_Write);
    print_ast(&printStream, &mainAst);
    
    //OpBuilder opBuilder = {};
    //opBuilder.instrStorage = allocate_buffer(4096);
    //instr_from_ast(&opBuilder, &mainAst);
    //print_instructions(&printStream, &opBuilder);
    
    graph_ast(&mainAst, "mainast.dot");
    //graph_instructions(&opBuilder, "maininstr.dot");
    
    // NOTE(michiel): RM -> R
    emit_r_r(assembler, add, Reg_RDX, Reg_RCX);
    emit_r_d(assembler, add, Reg_RDX, 0x12345678);
    emit_r_ripd(assembler, add, Reg_RDX, 0x12345678);
    emit_r_m(assembler, add, Reg_RDX, Reg_RCX);
    emit_r_mb(assembler, add, Reg_RDX, Reg_RCX, 0x12);
    emit_r_md(assembler, add, Reg_RDX, Reg_RCX, 0x12345678);
    emit_r_sib(assembler, add, Reg_RDX, Reg_RCX, Reg_RBX, Scale_X8);
    emit_r_sibb(assembler, add, Reg_RDX, Reg_RCX, Reg_RBX, Scale_X8, 0x12);
    emit_r_sibd(assembler, add, Reg_RDX, Reg_RCX, Reg_RBX, Scale_X8, 0x12345678);
    
    // NOTE(michiel): R -> RM
    emit_d_r(assembler, add, 0x12345678, Reg_RCX);
    emit_ripd_r(assembler, add, 0x12345678, Reg_RCX);
    emit_m_r(assembler, add, Reg_RDX, Reg_RCX);
    emit_mb_r(assembler, add, Reg_RDX, 0x12, Reg_RCX);
    emit_md_r(assembler, add, Reg_RDX, 0x12345678, Reg_RCX);
    emit_sib_r(assembler, add, Reg_RDX, Reg_RBX, Scale_X8, Reg_RCX);
    emit_sibb_r(assembler, add, Reg_RDX, Reg_RBX, Scale_X8, 0x12, Reg_RCX);
    emit_sibd_r(assembler, add, Reg_RDX, Reg_RBX, Scale_X8, 0x12345678, Reg_RCX);
    
    // NOTE(michiel): I -> RM
    emit_r_i(assembler, add, Reg_RDX, 0x12093487);
    emit_d_i(assembler, add, 0x12345678, 0x12093487);
    emit_ripd_i(assembler, add, 0x12345678, 0x12093487);
    emit_m_i(assembler, add, Reg_RDX, 0x12093487);
    emit_mb_i(assembler, add, Reg_RDX, 0x12, 0x12093487);
    emit_md_i(assembler, add, Reg_RDX, 0x12345678, 0x12093487);
    emit_sib_i(assembler, add, Reg_RDX, Reg_RCX, Scale_X8, 0x12093487);
    emit_sibb_i(assembler, add, Reg_RDX, Reg_RCX, Scale_X8, 0x12, 0x12093487);
    emit_sibd_i(assembler, add, Reg_RDX, Reg_RCX, Scale_X8, 0x12345678, 0x12093487);
    
    // NOTE(michiel): RM -> X
    emit_x_r(assembler, mul, Reg_RCX);
    emit_x_d(assembler, mul, 0x12345678);
    emit_x_ripd(assembler, mul, 0x12345678);
    emit_x_m(assembler, mul, Reg_RCX);
    emit_x_mb(assembler, mul, Reg_RCX, 0x12);
    emit_x_md(assembler, mul, Reg_RCX, 0x12345678);
    emit_x_sib(assembler, mul, Reg_RCX, Reg_RBX, Scale_X8);
    emit_x_sibb(assembler, mul, Reg_RCX, Reg_RBX, Scale_X8, 0x12);
    emit_x_sibd(assembler, mul, Reg_RCX, Reg_RBX, Scale_X8, 0x12345678);
    
    // NOTE(michiel): Move 64
    emit_mov64_r_i(assembler, Reg_RDX, 0x1234567890ABCDEF);
    emit_mov64_rax_off(assembler, 0x1234567890ABCDEF);
    emit_mov64_off_rax(assembler, 0x1234567890ABCDEF);
    
    // NOTE(michiel): Jumps and set based on condition codes
    emit_ib(assembler, jmp, 0x12);
    emit_i(assembler, jmp, 0x12345678);
    emit_c_ib(assembler, j, CC_NotEqual, 0x12);
    emit_c_ib(assembler, j, CC_NotEqual, 0x12);
    emit_c_i(assembler, j, CC_NotEqual, 0x1234);
    emit_c_i(assembler, j, CC_NotEqual, 0x1234);
    
    emit_c_r(assembler, set, CC_Overflow, Reg_AL);
    emit_c_r(assembler, set, CC_NoOverflow, Reg_AH);
    emit_c_r(assembler, set, CC_Below, Reg_CL);
    emit_c_r(assembler, set, CC_NotBelow, Reg_CH);
    emit_c_r(assembler, set, CC_Equal, Reg_SPL);    // TODO(michiel): This is AH now
    emit_c_r(assembler, set, CC_NotEqual, Reg_BPL); // TODO(michiel): This is CH now
    emit_c_r(assembler, set, CC_NotAbove, Reg_SIL); // TODO(michiel): This is DH now
    emit_c_r(assembler, set, CC_Above, Reg_DIL);    // TODO(michiel): This is BH now
    emit_c_r(assembler, set, CC_Sign, Reg_R8D);
    emit_c_r(assembler, set, CC_NoSign, Reg_R9D);
    emit_c_r(assembler, set, CC_Parity, Reg_R10D);
    emit_c_r(assembler, set, CC_NoParity, Reg_R11D);
    emit_c_r(assembler, set, CC_Less, Reg_R12D);
    emit_c_r(assembler, set, CC_NotLess, Reg_R13D);
    emit_c_r(assembler, set, CC_NotGreater, Reg_R14D);
    emit_c_r(assembler, set, CC_Greater, Reg_R15D);
    
    emit_program(assembler, &mainAst.program);
    dump_code(assembler, string("test.bin"));
#if 0    
    
    if (make_executable(codeBuild))
    {
        FuncCall *func = (FuncCall *)codeBuild.data;
        u64 x = func();
        fprintf(stdout, "Got 0x%016lX | %ld\n", x, x);
    }
#endif
    
    return 0;
}

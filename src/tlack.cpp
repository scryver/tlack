#define USE_LINUX_PLATFORM    1
#define USE_STD_PLATFORM      0

#include "../libberdip/src/platform.h"
#include "../libberdip/src/tokenizer.h"
#if USE_LINUX_PLATFORM
#include "../libberdip/src/linux_memory.h"
#elif USE_STD_PLATFORM
#include "../libberdip/src/std_memory.h"
#else
#error NO PLATFORM DEFINED
#endif

#include "ast.h"

#include "assembler_amd64.h"
#include "assembler.h"

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
make_executable(Buffer buffer, umm offset)
{
    b32 result = false;
    if (buffer.data)
    {
        PlatformMemoryBlock *block = (PlatformMemoryBlock *)((u8 *)buffer.data - sizeof(LinuxMemoryBlock));
        result = gMemoryApi->executable_memory(block, offset);
    }
    return result;
}

#include "../libberdip/src/memory.cpp"
#include "../libberdip/src/tokenizer.cpp"
#include "../libberdip/src/files.cpp"

#if USE_LINUX_PLATFORM
#include "../libberdip/src/linux_memory.cpp"
#include "../libberdip/src/linux_file.c"
#elif USE_STD_PLATFORM
#include "../libberdip/src/std_memory.cpp"
#include "../libberdip/src/std_file.c"
#endif

#include "ast.cpp"
#include "assembler.cpp"
#include "assembler_amd64.cpp"
#include "ast_assembler.cpp"
#include "ast_tlk.cpp"

#include "graphviz.cpp"
#include "graph_ast.cpp"

#define FUNC_CALL(name)   u64 name(void)
typedef FUNC_CALL(AsmFuncCall);

internal void
dump_code(Assembler *assembler, String filename)
{
    Buffer totalData = {assembler->codeAt, assembler->codeData.data};
    gFileApi->write_entire_file(filename, totalData);
}

internal void
test_emitter(Assembler *assembler)
{
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
    emit_d_r(assembler, sub, 0x12345678, Reg_RCX);
    emit_ripd_r(assembler, sub, 0x12345678, Reg_RCX);
    emit_m_r(assembler, sub, Reg_RDX, Reg_RCX);
    emit_mb_r(assembler, sub, Reg_RDX, 0x12, Reg_RCX);
    emit_md_r(assembler, sub, Reg_RDX, 0x12345678, Reg_RCX);
    emit_sib_r(assembler, sub, Reg_RDX, Reg_RBX, Scale_X8, Reg_RCX);
    emit_sibb_r(assembler, sub, Reg_RDX, Reg_RBX, Scale_X8, 0x12, Reg_RCX);
    emit_sibd_r(assembler, sub, Reg_RDX, Reg_RBX, Scale_X8, 0x12345678, Reg_RCX);
    
    // NOTE(michiel): I -> RM
    emit_r_ib(assembler, xor, Reg_RDX, 0x12);
    emit_r_i(assembler, xor, Reg_RDX, 0x12093487);
    emit_d_i(assembler, xor, 0x12345678, 0x12093487);
    emit_ripd_i(assembler, xor, 0x12345678, 0x12093487);
    emit_m_i(assembler, xor, Reg_RDX, 0x12093487);
    emit_mb_i(assembler, xor, Reg_RDX, 0x12, 0x12093487);
    emit_md_i(assembler, xor, Reg_RDX, 0x12345678, 0x12093487);
    emit_sib_i(assembler, xor, Reg_RDX, Reg_RCX, Scale_X8, 0x12093487);
    emit_sibb_i(assembler, xor, Reg_RDX, Reg_RCX, Scale_X8, 0x12, 0x12093487);
    emit_sibd_i(assembler, xor, Reg_RDX, Reg_RCX, Scale_X8, 0x12345678, 0x12093487);
    
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
    emit_mov32_r_i(assembler, Reg_RDX, 0x12);
    emit_mov32_r_i(assembler, Reg_RDX, 0xFFFFFFFF);
    emit_r_i(assembler, movsx, Reg_RDX, 0x12);
    emit_r_i(assembler, movsx, Reg_RDX, -0x12);
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
    emit_c_r(assembler, set, CC_Equal, Reg_SPL);
    emit_c_r(assembler, set, CC_NotEqual, Reg_BPL);
    emit_c_r(assembler, set, CC_NotAbove, Reg_SIL);
    emit_c_r(assembler, set, CC_Above, Reg_DIL);
    emit_c_r(assembler, set, CC_Sign, Reg_R8D);
    emit_c_r(assembler, set, CC_NoSign, Reg_R9D);
    emit_c_r(assembler, set, CC_Parity, Reg_R10D);
    emit_c_r(assembler, set, CC_NoParity, Reg_R11D);
    emit_c_r(assembler, set, CC_Less, Reg_R12D);
    emit_c_r(assembler, set, CC_NotLess, Reg_R13D);
    emit_c_r(assembler, set, CC_NotGreater, Reg_R14D);
    emit_c_r(assembler, set, CC_Greater, Reg_R15D);
}

internal void
test_simple_bytes(MemoryAllocator *allocator)
{
    Assembler *assembler = create_assembler(allocator, kilobytes(4));
    //emit_mov32_r_i(assembler, Reg_RAX, 0xFFFFFFFF);
    //emit_mov64_r_i(assembler, Reg_RAX, 0xFFFFFFFF);
    emit_r_i(assembler, movsx, Reg_RAX, 0xFFFFFFFF);
    emit_ret(assembler);
    
    if (make_executable(assembler->codeData, 0))
    {
        AsmFuncCall *func = (AsmFuncCall *)assembler->codeData.data;
        u64 x = func();
        fprintf(stdout, "Got 0x%016lX | %ld\n", x, x);
    }
    else
    {
        fprintf(stderr, "Failed making it execute\n");
    }
    
    deallocate(allocator, assembler->codeData.data);
    deallocate(allocator, assembler);
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
    
    Assembler *assembler = create_assembler(&defaultAlloc, megabytes(1ULL), kilobytes(4));
    
#if 0    
    test_emitter(assembler);
    dump_code(assembler, string("test.bin"));
    return 0;
#endif
    
    i_expect(assembler->codeAt < (assembler->codeData.size - 0x1000));
    assembler->codeAt = (assembler->codeAt + 0xFFF) & ~0xFFF;
    
    Ast mainAst = {};
    
    String langName = static_string("test/simple.tlk");
    if (argc > 1)
    {
        langName = string(argv[1]);
    }
    Buffer langTest = gFileApi->read_entire_file(&defaultAlloc, langName);
    ast_from_tlk(&mainAst, langName, langTest);
    
    FileStream printStream = {};
    printStream.file = gFileApi->open_file(string("stdout"), FileOpen_Write);
    print_ast(&printStream, &mainAst);
    
    graph_ast(&mainAst, "mainast.dot");
    
    u32 freeRegs = assembler->freeRegisterMask;
    //u64 callAddress = assembler->codeAt;
    umm callAddress = emit_program(assembler, &mainAst.program);
    fprintf(stderr, "Code at: 0x%016lX | %lu\n", callAddress, callAddress);
    dump_code(assembler, string("test.bin"));
    
    fprintf(stderr, "0x%08X == 0x%08X\n", freeRegs, assembler->freeRegisterMask);
    i_expect(freeRegs == assembler->freeRegisterMask);
    
    // NOTE(michiel): Offset at least 64 bytes less than where the code starts. (sizeof(LinuxMemoryBlock))
    if (1 && make_executable(assembler->codeData, 4000))
    {
        AsmFuncCall *func = (AsmFuncCall *)(assembler->codeData.data + callAddress);
        u64 x = func();
        gFileApi->write_fmt_to_file(&printStream.file, "Got 0x%016lX | %ld\n", x, x);
    }
    else
    {
        fprintf(stderr, "Failed to make it executable\n");
    }
    
    return 0;
}

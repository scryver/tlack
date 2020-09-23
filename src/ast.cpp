internal void *
ast_allocate(Ast *ast, umm size)
{
    void *result = arena_allocate_size(&ast->arena, size, default_memory_alloc());
    return result;
}

internal Expression *
allocate_expression(Ast *ast, ExprKind kind)
{
    // TODO(michiel): Allocate depending on the kind
    Expression *result = (Expression *)ast_allocate(ast, sizeof(Expression));
    i_expect(result);
    result->kind = kind;
    return result;
}

internal Statement *
allocate_statement(Ast *ast, StmtKind kind)
{
    // TODO(michiel): Allocate depending on the kind
    Statement *result = (Statement *)ast_allocate(ast, sizeof(Statement));
    i_expect(result);
    result->kind = kind;
    return result;
}

internal Expression *
create_signed_int(Ast *ast, s64 intConst)
{
    Expression *result = allocate_expression(ast, Expr_Int);
    result->eIntConst = intConst;
    return result;
}

internal Expression *
create_identifier(Ast *ast, String name)
{
    Expression *result = allocate_expression(ast, Expr_Identifier);
    result->eName = minterned_string(&ast->interns, name);
    return result;
}

internal Expression *
create_unary_expr(Ast *ast, UnaryOpType op, Expression *operand)
{
    Expression *result = allocate_expression(ast, Expr_Unary);
    result->eUnary.op = op;
    result->eUnary.operand = operand;
    return result;
}

internal Expression *
create_binary_expr(Ast *ast, BinaryOpType op, Expression *left, Expression *right)
{
    Expression *result = allocate_expression(ast, Expr_Binary);
    result->eBinary.op = op;
    result->eBinary.left = left;
    result->eBinary.right = right;
    return result;
}

internal Statement *
create_assign_stmt(Ast *ast, AssignOpType op, Expression *left, Expression *right)
{
    Statement *result = allocate_statement(ast, Stmt_Assign);
    result->sAssign.op = op;
    result->sAssign.left = left;
    result->sAssign.right = right;
    return result;
}

internal Statement *
create_return_stmt(Ast *ast, Expression *returned)
{
    Statement *result = allocate_statement(ast, Stmt_Return);
    result->sExpression = returned;
    return result;
}

internal Statement *
create_if_stmt(Ast *ast, Expression *ifCond, StmtBlock *ifBlock, u32 elifCount, ConditionBlock *elifBlocks, StmtBlock *elseBlock)
{
    Statement *result = allocate_statement(ast, Stmt_IfElse);
    result->sIf.ifBlock.condition = ifCond;
    result->sIf.ifBlock.block = ifBlock;
    result->sIf.elIfCount = elifCount;
    if (elifCount) {
        result->sIf.elIfBlocks = (ConditionBlock *)ast_allocate(ast, sizeof(ConditionBlock)*elifCount);
        copy(sizeof(ConditionBlock)*elifCount, elifBlocks, result->sIf.elIfBlocks);
    }
    result->sIf.elseBlock = elseBlock;
    return result;
}

internal Statement *
create_do_stmt(Ast *ast, Expression *cond, StmtBlock *block)
{
    Statement *result = allocate_statement(ast, Stmt_DoWhile);
    result->sDo.condition = cond;
    result->sDo.block = block;
    return result;
}

internal Statement *
create_while_stmt(Ast *ast, Expression *cond, StmtBlock *block)
{
    Statement *result = allocate_statement(ast, Stmt_While);
    result->sWhile.condition = cond;
    result->sWhile.block = block;
    return result;
}

internal Statement *
create_for_stmt(Ast *ast, Statement *init, Expression *cond, Statement *next, StmtBlock *block)
{
    Statement *result = allocate_statement(ast, Stmt_For);
    result->sFor.init = init;
    result->sFor.condition = cond;
    result->sFor.next = next;
    result->sFor.body = block;
    return result;
}

internal StmtBlock *
create_statement_block(Ast *ast, u32 statementCount, Statement **statements)
{
    StmtBlock *result = (StmtBlock *)ast_allocate(ast, sizeof(StmtBlock));
    result->stmtCount = statementCount;
    result->statements = (Statement **)ast_allocate(ast, sizeof(Statement *)*statementCount);
    copy(sizeof(Statement *)*statementCount, statements, result->statements);
    return result;
}

internal Function *
create_function(Ast *ast, String name, StmtBlock *body)
{
    Function *result = (Function *)ast_allocate(ast, sizeof(Function));
    result->name = minterned_string(&ast->interns, name);
    result->body = body;
    return result;
}

internal void
print_expression(FileStream *output, Expression *expression)
{
    switch (expression->kind)
    {
        case Expr_Int: {
            print(output, "%ld", expression->eIntConst);
        } break;
        
        case Expr_Identifier: {
            print(output, "%.*s", STR_FMT(expression->eName));
        } break;
        
        case Expr_Unary: {
            char *unaryName = "unknown";
            switch (expression->eUnary.op)
            {
                case Unary_Minus: unaryName = "-"; break;
                case Unary_Flip : unaryName = "~"; break;
                case Unary_Not  : unaryName = "!"; break;
                INVALID_DEFAULT_CASE;
            }
            print(output, "(%s ", unaryName);
            print_expression(output, expression->eUnary.operand);
            print(output, ")");
        } break;
        
        case Expr_Binary: {
            char *binaryName = "unknown";
            switch (expression->eBinary.op)
            {
                case Binary_Add : binaryName = "+"; break;
                case Binary_Sub : binaryName = "-"; break;
                case Binary_Mul : binaryName = "*"; break;
                case Binary_Div : binaryName = "/"; break;
                case Binary_Mod : binaryName = "%"; break;
                case Binary_Eq  : binaryName = "=="; break;
                case Binary_Ne  : binaryName = "!="; break;
                case Binary_Lt  : binaryName = "<"; break;
                case Binary_Gt  : binaryName = ">"; break;
                case Binary_LE  : binaryName = "<="; break;
                case Binary_GE  : binaryName = ">="; break;
                INVALID_DEFAULT_CASE;
            }
            print(output, "(%s ", binaryName);
            print_expression(output, expression->eBinary.left);
            print(output, " ");
            print_expression(output, expression->eBinary.right);
            print(output, ")");
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void print_stmt_block(FileStream *output, StmtBlock *block, b32 startNewline = true);

internal void
print_statement(FileStream *output, Statement *statement)
{
    switch (statement->kind)
    {
        case Stmt_Assign: {
            char *assignName = "unknown";
            switch (statement->sAssign.op)
            {
                case Assign_Set : assignName = "="; break;
                case Assign_Add : assignName = "+="; break;
                case Assign_Sub : assignName = "-="; break;
                case Assign_Mul : assignName = "*="; break;
                case Assign_Div : assignName = "/="; break;
                case Assign_Mod : assignName = "%="; break;
                INVALID_DEFAULT_CASE;
            }
            println_begin(output, "(%s ", assignName);
            print_expression(output, statement->sAssign.left);
            print(output, " ");
            print_expression(output, statement->sAssign.right);
            println_end(output, ")");
        } break;
        
        case Stmt_Return: {
            if (statement->sExpression) {
                println_begin(output, "(return ");
                print_expression(output, statement->sExpression);
                println_end(output, ")");
            }
            else
            {
                println(output, "(return)");
            }
        } break;
        
        case Stmt_IfElse: {
            println_begin(output, "(if ");
            print_expression(output, statement->sIf.ifBlock.condition);
            println_end(output, " then");
            ++output->indent;
            print_stmt_block(output, statement->sIf.ifBlock.block, false);
            for (u32 elifIdx = 0; elifIdx < statement->sIf.elIfCount; ++elifIdx)
            {
                ConditionBlock *elif = statement->sIf.elIfBlocks + elifIdx;
                --output->indent;
                println_begin(output, " elif ");
                print_expression(output, elif->condition);
                println_end(output, " then");
                ++output->indent;
                print_stmt_block(output, elif->block, false);
            }
            if (statement->sIf.elseBlock)
            {
                --output->indent;
                println(output, " else");
                ++output->indent;
                print_stmt_block(output, statement->sIf.elseBlock, false);
            }
            --output->indent;
            println(output, ")");
        } break;
        
        case Stmt_DoWhile: {
            println(output, "(do");
            ++output->indent;
            print_stmt_block(output, statement->sDo.block);
            --output->indent;
            println_begin(output, " while ");
            print_expression(output, statement->sDo.condition);
            println_end(output, ")");
        } break;
        
        case Stmt_While: {
            println_begin(output, "(while ");
            print_expression(output, statement->sWhile.condition);
            println_end(output, "");
            ++output->indent;
            print_stmt_block(output, statement->sWhile.block);
            --output->indent;
            println(output, ")");
        } break;
        
        case Stmt_For: {
            println(output, "(for");
            ++output->indent;
            print_statement(output, statement->sFor.init);
            println_begin(output, "");
            print_expression(output, statement->sFor.condition);
            println_end(output, "");
            print_statement(output, statement->sFor.next);
            print_stmt_block(output, statement->sFor.body);
            --output->indent;
            println(output, ")");
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
print_stmt_block(FileStream *output, StmtBlock *block, b32 startNewline)
{
    startNewline = true;
    if (startNewline) {
        println(output, "(");
    } else {
        println_end(output, "(");
    }
    ++output->indent;
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        print_statement(output, block->statements[stmtIdx]);
    }
    --output->indent;
    if (startNewline) {
        println(output, ")");
    } else {
        println_begin(output, ")");
    }
}

internal void
print_function(FileStream *output, Function *function)
{
    println(output, "( func %.*s", STR_FMT(function->name));
    ++output->indent;
    print_stmt_block(output, function->body);
    --output->indent;
    println(output, ")");
}

internal void
print_program(FileStream *output, Program *program)
{
    println(output, "( program");
    ++output->indent;
    print_function(output, program->main);
    --output->indent;
    println(output, ")");
}

internal void
print_ast(FileStream *output, Ast *ast)
{
    print_program(output, &ast->program);
}

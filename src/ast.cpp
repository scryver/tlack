internal Expression
create_expression(s64 intConst)
{
    Expression result = {};
    result.kind = Expr_Int;
    result.intConst = intConst;
    return result;
}

internal Expression
create_expression(UnaryOpType op, Expression *operand)
{
    Expression result = {};
    result.kind = Expr_Unary;
    result.unary.op = op;
    result.unary.operand = operand;
    return result;
}

internal Expression
create_expression(BinaryOpType op, Expression *left, Expression *right)
{
    Expression result = {};
    result.kind = Expr_Binary;
    result.binary.op = op;
    result.binary.left = left;
    result.binary.right = right;
    return result;
}

internal Statement
create_assign_stmt(AssignOpType op, Expression *left, Expression *right)
{
    Statement result = {};
    result.kind = Stmt_Assign;
    result.assign.op = op;
    result.assign.left = left;
    result.assign.right = right;
    return result;
}

internal Statement
create_return_stmt(Expression *returned)
{
    Statement result = {};
    result.kind = Stmt_Return;
    result.expression = returned;
    return result;
}

internal void
print_expression(FileStream *output, Expression *expression)
{
    switch (expression->kind)
    {
        case Expr_Int: {
            println(output, "<Expression 'int' %ld />", expression->intConst);
        } break;
        
        case Expr_Identifier: {
            println(output, "<Expression 'id' '%.*s' />", STR_FMT(expression->name));
        } break;
        
        case Expr_Unary: {
            char *unaryName = "unknown";
            switch (expression->unary.op)
            {
                case Unary_Minus: unaryName = "-"; break;
                case Unary_Flip : unaryName = "~"; break;
                case Unary_Not  : unaryName = "!"; break;
                INVALID_DEFAULT_CASE;
            }
            println(output, "<Expression 'unary' '%s'>", unaryName);
            ++output->indent;
            print_expression(output, expression->unary.operand);
            --output->indent;
            println(output, "</Expression>");
        } break;
        
        case Expr_Binary: {
            char *binaryName = "unknown";
            switch (expression->binary.op)
            {
                case Binary_Add : binaryName = "+"; break;
                case Binary_Sub : binaryName = "-"; break;
                case Binary_Mul : binaryName = "*"; break;
                case Binary_Div : binaryName = "/"; break;
                INVALID_DEFAULT_CASE;
            }
            println(output, "<Expression 'binary' '%s'>", binaryName);
            ++output->indent;
            print_expression(output, expression->binary.left);
            --output->indent;
            println(output, "<Op '%s'>", binaryName);
            ++output->indent;
            print_expression(output, expression->binary.right);
            --output->indent;
            println(output, "</Expression>");
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
print_statement(FileStream *output, Statement *statement)
{
    switch (statement->kind)
    {
        case Stmt_Assign: {
            char *assignName = "unknown";
            switch (statement->assign.op)
            {
                case Assign_Set : assignName = "="; break;
                case Assign_Add : assignName = "+="; break;
                case Assign_Sub : assignName = "-="; break;
                case Assign_Mul : assignName = "*="; break;
                case Assign_Div : assignName = "/="; break;
                INVALID_DEFAULT_CASE;
            }
            println(output, "<Assign '%s'>", assignName);
            ++output->indent;
            print_expression(output, statement->assign.left);
            --output->indent;
            println(output, "<Op '%s'>", assignName);
            ++output->indent;
            print_expression(output, statement->assign.right);
            --output->indent;
            println(output, "</Assign>");
        } break;
        
        case Stmt_Return: {
            if (statement->expression) {
                println(output, "<Return>");
                ++output->indent;
                print_expression(output, statement->expression);
                --output->indent;
                println(output, "</Return>");
            }
            else
            {
                println(output, "<Return />");
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
print_stmt_block(FileStream *output, StmtBlock *block)
{
    println(output, "<Statement Block>");
    ++output->indent;
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        print_statement(output, block->statements[stmtIdx]);
    }
    --output->indent;
    println(output, "</Statement Block>");
}

internal void
print_function(FileStream *output, Function *function)
{
    println(output, "<Function '%.*s'>", STR_FMT(function->name));
    ++output->indent;
    print_stmt_block(output, function->body);
    --output->indent;
    println(output, "</Function>");
}

internal void
print_program(FileStream *output, Program *program)
{
    println(output, "<Program>");
    ++output->indent;
    print_function(output, program->main);
    --output->indent;
    println(output, "</Program>");
}

internal void
print_ast(FileStream *output, Ast *ast)
{
    print_program(output, &ast->program);
}

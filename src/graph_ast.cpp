internal void
graph_ast_expression(Ast *ast, FileStream *output, Expression *expression, String connection)
{
    char formatBuffer[128];
    switch (expression->kind)
    {
        case Expr_Int:
        {
            String intStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "int%p", expression);
            graph_label(output, intStr, "%lld", expression->intConst);
            graph_connect(output, connection, intStr);
        } break;
        
        case Expr_Identifier:
        {
            String name = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "id%p", expression);
            graph_label(output, name, "%.*s", STR_FMT(expression->name));
            graph_connect(output, connection, name);
        } break;
        
        case Expr_Unary:
        {
            String opStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "unOp%p", expression);
            switch (expression->unary.op)
            {
                case Unary_Minus: { graph_label(output, opStr, "-"); } break;
                case Unary_Flip : { graph_label(output, opStr, "~"); } break;
                case Unary_Not  : { graph_label(output, opStr, "!"); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_connect(output, connection, opStr);
            graph_ast_expression(ast, output, expression->unary.operand, opStr);
        } break;
        
        case Expr_Binary:
        {
            String opStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "binOp%p", expression);
            switch (expression->binary.op)
            {
                case Binary_Add: { graph_label(output, opStr, "+"); } break;
                case Binary_Sub: { graph_label(output, opStr, "-"); } break;
                case Binary_Mul: { graph_label(output, opStr, "*"); } break;
                case Binary_Div: { graph_label(output, opStr, "/"); } break;
                case Binary_Mod: { graph_label(output, opStr, "%"); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_connect(output, connection, opStr);
            graph_ast_expression(ast, output, expression->binary.left, opStr);
            graph_ast_expression(ast, output, expression->binary.right, opStr);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
graph_ast_statement(Ast *ast, FileStream *output, Statement *statement, String connection)
{
    char formatBuffer[128];
    switch (statement->kind)
    {
        case Stmt_Assign: {
            String assignStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "assign%p", statement);
            switch (statement->assign.op)
            {
                case Assign_Set : { graph_label(output, assignStr, "="); } break;
                case Assign_Add : { graph_label(output, assignStr, "+="); } break;
                case Assign_Sub : { graph_label(output, assignStr, "-="); } break;
                case Assign_Mul : { graph_label(output, assignStr, "*="); } break;
                case Assign_Div : { graph_label(output, assignStr, "/="); } break;
                case Assign_Mod : { graph_label(output, assignStr, "%="); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_connect(output, connection, assignStr);
            graph_ast_expression(ast, output, statement->assign.left, assignStr);
            graph_ast_expression(ast, output, statement->assign.right, assignStr);
        } break;
        
        case Stmt_Return: {
            String retStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "ret%p", statement);
            graph_label(output, retStr, "return");
            graph_connect(output, connection, retStr);
            if (statement->expression) {
                graph_ast_expression(ast, output, statement->expression, retStr);
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
graph_ast_stmt_block(Ast *ast, FileStream *output, StmtBlock *block, String connection)
{
    for (u32 stmtIdx = 0; stmtIdx < block->stmtCount; ++stmtIdx)
    {
        graph_ast_statement(ast, output, block->statements[stmtIdx], connection);
    }
}

internal void
graph_ast_function(Ast *ast, FileStream *output, Function *function)
{
    char formatBuffer[1024];
    String connection = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "func_%.*s", function->name);
    
    println(output, "subgraph cluster_%.*s {", STR_FMT(function->name));
    ++output->indent;
    graph_label(output, connection, "%.*s", STR_FMT(function->name));
    graph_ast_stmt_block(ast, output, function->body, connection);
    --output->indent;
    println(output, "}\n");
}

internal void
graph_ast(Ast *ast, char *fileName)
{
    i_expect(string_length(gSpaces) == 256);
    FileStream output = {0};
    output.file = gFileApi->open_file(string(fileName), FileOpen_Write);
    
    println(&output, "digraph ast {");
    ++output.indent;
    graph_ast_function(ast, &output, ast->program.main);
    --output.indent;
    println(&output, "}\n");
    
    gFileApi->close_file(&output.file);
}

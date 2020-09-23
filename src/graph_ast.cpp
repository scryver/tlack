internal void
graph_ast_expression(Ast *ast, FileStream *output, Expression *expression, String connection)
{
    char formatBuffer[128];
    switch (expression->kind)
    {
        case Expr_Int:
        {
            String intStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "int%p", expression);
            graph_label(output, intStr, "%lld", expression->eIntConst);
            graph_connect(output, connection, intStr);
        } break;
        
        case Expr_Identifier:
        {
            String name = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "id%p", expression);
            graph_label(output, name, "%.*s", STR_FMT(expression->eName));
            graph_connect(output, connection, name);
        } break;
        
        case Expr_Unary:
        {
            String opStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "unOp%p", expression);
            switch (expression->eUnary.op)
            {
                case Unary_Minus: { graph_label(output, opStr, "-"); } break;
                case Unary_Flip : { graph_label(output, opStr, "~"); } break;
                case Unary_Not  : { graph_label(output, opStr, "!"); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_connect(output, connection, opStr);
            graph_ast_expression(ast, output, expression->eUnary.operand, opStr);
        } break;
        
        case Expr_Binary:
        {
            String opStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "binOp%p", expression);
            switch (expression->eBinary.op)
            {
                case Binary_Add: { graph_label(output, opStr, "+"); } break;
                case Binary_Sub: { graph_label(output, opStr, "-"); } break;
                case Binary_Mul: { graph_label(output, opStr, "*"); } break;
                case Binary_Div: { graph_label(output, opStr, "/"); } break;
                case Binary_Mod: { graph_label(output, opStr, "%%"); } break;
                case Binary_Eq : { graph_label(output, opStr, "=="); } break;
                case Binary_Ne : { graph_label(output, opStr, "!="); } break;
                case Binary_Lt : { graph_label(output, opStr, "<"); } break;
                case Binary_Gt : { graph_label(output, opStr, ">"); } break;
                case Binary_LE : { graph_label(output, opStr, "<="); } break;
                case Binary_GE : { graph_label(output, opStr, ">="); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_connect(output, connection, opStr);
            graph_ast_expression(ast, output, expression->eBinary.left, opStr);
            graph_ast_expression(ast, output, expression->eBinary.right, opStr);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void graph_ast_stmt_block(Ast *ast, FileStream *output, StmtBlock *block, String connection);

internal void
graph_ast_statement(Ast *ast, FileStream *output, Statement *statement, String connection)
{
    char formatBuffer[128];
    String stmtStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "stmt%p", statement);
    graph_dash_connect(output, connection, stmtStr);
    println(output, "subgraph cluster_%p {", statement);
    ++output->indent;
    
    switch (statement->kind)
    {
        case Stmt_Assign: {
            switch (statement->sAssign.op)
            {
                case Assign_Set : { graph_label(output, stmtStr, "="); } break;
                case Assign_Add : { graph_label(output, stmtStr, "+="); } break;
                case Assign_Sub : { graph_label(output, stmtStr, "-="); } break;
                case Assign_Mul : { graph_label(output, stmtStr, "*="); } break;
                case Assign_Div : { graph_label(output, stmtStr, "/="); } break;
                case Assign_Mod : { graph_label(output, stmtStr, "%="); } break;
                INVALID_DEFAULT_CASE;
            }
            graph_ast_expression(ast, output, statement->sAssign.left, stmtStr);
            graph_ast_expression(ast, output, statement->sAssign.right, stmtStr);
        } break;
        
        case Stmt_Return: {
            graph_label(output, stmtStr, "return");
            if (statement->sExpression) {
                graph_ast_expression(ast, output, statement->sExpression, stmtStr);
            }
        } break;
        
        case Stmt_IfElse: {
            graph_label(output, stmtStr, "if/else");
            
            //String endIfStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "endif%p", statement);
            //graph_label(output, endIfStr, "end if");
            
            String ifStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "if%p", statement);
            
            println(output, "subgraph cluster_%.*s {", STR_FMT(ifStr));
            ++output->indent;
            
            graph_label(output, ifStr, "if");
            graph_dash_connect(output, stmtStr, ifStr);
            graph_ast_expression(ast, output, statement->sIf.ifBlock.condition, ifStr);
            
            String thenIfStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "ifthen%p", statement);
            graph_label(output, thenIfStr, "then");
            graph_dash_connect(output, ifStr, thenIfStr);
            graph_ast_stmt_block(ast, output, statement->sIf.ifBlock.block, thenIfStr);
            
            --output->indent;
            println(output, "}\n");
            
            //graph_dash_connect(output, thenIfStr, endIfStr);
            
            for (u32 index = 0; index < statement->sIf.elIfCount; ++index)
            {
                ConditionBlock *elif = statement->sIf.elIfBlocks + index;
                String elifStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "elif%p", (umm)statement + index);
                
                println(output, "subgraph cluster_%.*s {", STR_FMT(elifStr));
                ++output->indent;
                
                graph_label(output, elifStr, "elif");
                graph_dash_connect(output, stmtStr, elifStr);
                graph_ast_expression(ast, output, elif->condition, elifStr);
                
                String thenElIfStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "elifthen%p", (umm)statement + index);
                graph_label(output, thenElIfStr, "then");
                graph_dash_connect(output, elifStr, thenElIfStr);
                graph_ast_stmt_block(ast, output, elif->block, thenElIfStr);
                
                --output->indent;
                println(output, "}\n");
                
                //graph_dash_connect(output, thenElIfStr, endIfStr);
            }
            
            if (statement->sIf.elseBlock)
            {
                String elseStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "else%p", statement);
                
                println(output, "subgraph cluster_%.*s {", STR_FMT(elseStr));
                ++output->indent;
                
                graph_label(output, elseStr, "else");
                graph_dash_connect(output, stmtStr, elseStr);
                graph_ast_stmt_block(ast, output, statement->sIf.elseBlock, elseStr);
                
                --output->indent;
                println(output, "}\n");
                
                //graph_dash_connect(output, elseStr, endIfStr);
            }
        } break;
        
        case Stmt_DoWhile: {
            graph_label(output, stmtStr, "do/while");
            
            String doStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "doblock%p", statement);
            graph_label(output, doStr, "do");
            graph_dash_connect(output, stmtStr, doStr);
            
            graph_ast_stmt_block(ast, output, statement->sDo.block, doStr);
            String condStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "docond%p", statement);
            graph_label(output, condStr, "while");
            graph_dash_connect(output, doStr, condStr);
            graph_ast_expression(ast, output, statement->sDo.condition, condStr);
        } break;
        
        case Stmt_While: {
            graph_label(output, stmtStr, "while");
            
            String whileStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "whilecond%p", statement);
            graph_label(output, whileStr, "while");
            graph_dash_connect(output, stmtStr, whileStr);
            graph_ast_expression(ast, output, statement->sWhile.condition, whileStr);
            
            String blockStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "whileblock%p", statement);
            graph_label(output, blockStr, "then");
            graph_dash_connect(output, whileStr, blockStr);
            graph_ast_stmt_block(ast, output, statement->sDo.block, blockStr);
        } break;
        
        case Stmt_For: {
            graph_label(output, stmtStr, "for");
            
            String initStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "forinit%p", statement);
            graph_label(output, initStr, "init");
            graph_dash_connect(output, stmtStr, initStr);
            graph_ast_statement(ast, output, statement->sFor.init, initStr);
            
            String condStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "forcond%p", statement);
            graph_label(output, condStr, "cond");
            graph_dash_connect(output, stmtStr, condStr);
            graph_ast_expression(ast, output, statement->sFor.condition, condStr);
            
            String nextStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "fornext%p", statement);
            graph_label(output, nextStr, "next");
            graph_dash_connect(output, stmtStr, nextStr);
            graph_ast_statement(ast, output, statement->sFor.next, nextStr);
            
            String blockStr = minterned_string_fmt(&ast->interns, sizeof(formatBuffer), formatBuffer, "forbody%p", statement);
            graph_label(output, blockStr, "then");
            graph_dash_connect(output, stmtStr, blockStr);
            graph_ast_stmt_block(ast, output, statement->sFor.body, blockStr);
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    --output->indent;
    println(output, "}\n");
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

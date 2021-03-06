internal void
strip_newlines(Tokenizer *tokenizer)
{
    while (match_token(tokenizer, Token_Newline))
    {
        ;
    }
}

internal Expression *parse_expression(Tokenizer *tokenizer, Ast *ast);

internal Expression *
parse_base(Tokenizer *tokenizer, Ast *ast)
{
    Expression *result = 0;
    Token token = get_token(tokenizer);
    if (token.kind == Token_Integer)
    {
        s64 value = number_from_string(token.value);
        result = create_signed_int(ast, value);
        result->origin = token.origin;
    }
    else if (token.kind == Token_Name)
    {
        String name = token.value;
        SourcePos origin = token.origin;
        if (match_token(tokenizer, Token_ParenOpen))
        {
            Expression **arguments = 0;
            if (match_token(tokenizer, Token_ParenClose))
            {
                // NOTE(michiel): Do nothing
            }
            else
            {
                do {
                    Expression *expr = parse_expression(tokenizer, ast);
                    mbuf_push(arguments, expr);
                } while (match_token(tokenizer, Token_Comma));
                expect_token(tokenizer, Token_ParenClose);
            }
            
            result = create_func_call(ast, name, mbuf_count(arguments), arguments);
        }
        else
        {
            result = create_identifier(ast, name);
        }
        result->origin = origin;
    }
    else if (token.kind == Token_ParenOpen)
    {
        result = parse_expression(tokenizer, ast);
        expect_token(tokenizer, Token_ParenClose);
    }
    return result;
}

internal Expression *
parse_unary(Tokenizer *tokenizer, Ast *ast)
{
    Expression *result = 0;
    Token token = peek_token(tokenizer);
    if((token.kind == Token_Subtract) || (token.kind == Token_Invert) || (token.kind == Token_Not))
    {
        expect_token(tokenizer, token.kind);
        UnaryOpType opType = Unary_None;
        switch (token.kind)
        {
            case Token_Subtract: { opType = Unary_Minus; } break;
            case Token_Invert  : { opType = Unary_Flip; } break;
            case Token_Not     : { opType = Unary_Not; } break;
            INVALID_DEFAULT_CASE;
        }
        Expression *operand = parse_unary(tokenizer, ast);
        
        result = create_unary_expr(ast, opType, operand);
        result->origin = token.origin;
    }
    else
    {
        result = parse_base(tokenizer, ast);
    }
    
    return result;
}

internal Expression *
parse_mul_expr(Tokenizer *tokenizer, Ast *ast)
{
    SourcePos origin = tokenizer->origin;
    Expression *left = parse_unary(tokenizer, ast);
    Token token = peek_token(tokenizer);
    while ((token.kind == Token_Multiply) || (token.kind == Token_Divide) || (token.kind == Token_Modulus))
    {
        expect_token(tokenizer, token.kind);
        
        Expression *right = parse_unary(tokenizer, ast);
        
        if (token.kind == Token_Multiply)
        {
            left = create_binary_expr(ast, Binary_Mul, left, right);
        }
        else if (token.kind == Token_Divide)
        {
            left = create_binary_expr(ast, Binary_Div, left, right);
        }
        else
        {
            i_expect(token.kind == Token_Modulus);
            left = create_binary_expr(ast, Binary_Mod, left, right);
        }
        left->origin = origin;
        
        token = peek_token(tokenizer);
    }
    return left;
}

internal Expression *
parse_add_expr(Tokenizer *tokenizer, Ast *ast)
{
    SourcePos origin = tokenizer->origin;
    Expression *left = parse_mul_expr(tokenizer, ast);
    Token token = peek_token(tokenizer);
    while ((token.kind == Token_Add) || (token.kind == Token_Subtract))
    {
        expect_token(tokenizer, token.kind);
        
        Expression *right = parse_mul_expr(tokenizer, ast);
        
        if (token.kind == Token_Add)
        {
            left = create_binary_expr(ast, Binary_Add, left, right);
            left->origin = origin;
        }
        else
        {
            i_expect(token.kind == Token_Subtract);
            left = create_binary_expr(ast, Binary_Sub, left, right);
            left->origin = origin;
        }
        
        token = peek_token(tokenizer);
    }
    return left;
}

internal Expression *
parse_expression(Tokenizer *tokenizer, Ast *ast)
{
    SourcePos origin = tokenizer->origin;
    Expression *left = parse_add_expr(tokenizer, ast);
    Token token = peek_token(tokenizer);
    if ((token.kind >= Token_Eq) && (token.kind <= Token_GtEq))
    {
        expect_token(tokenizer, token.kind);
        
        Expression *right = parse_add_expr(tokenizer, ast);
        
        BinaryOpType binOp = Binary_None;
        switch (token.kind)
        {
            case Token_Eq:   { binOp = Binary_Eq; } break;
            case Token_Neq:  { binOp = Binary_Ne; } break;
            case Token_Lt:   { binOp = Binary_Lt; } break;
            case Token_Gt:   { binOp = Binary_Gt; } break;
            case Token_LtEq: { binOp = Binary_LE; } break;
            case Token_GtEq: { binOp = Binary_GE; } break;
            INVALID_DEFAULT_CASE;
        }
        left = create_binary_expr(ast, binOp, left, right);
        left->origin = origin;
        
        token = peek_token(tokenizer);
    }
    return left;
}

internal StmtBlock *parse_statement_block(Tokenizer *tokenizer, Ast *ast);

internal Statement *
parse_statement(Tokenizer *tokenizer, Ast *ast, b32 expectEnd = true)
{
    Statement *result = 0;
    Token base = expect_token(tokenizer, Token_Name);
    if (base.value == string("return"))
    {
        Expression *returnExpr = parse_expression(tokenizer, ast);
        if (expectEnd) {
            expect_token(tokenizer, Token_SemiColon);
        }
        result = create_return_stmt(ast, returnExpr);
    }
    else if (base.value == string("if"))
    {
        Expression *ifCondition = parse_expression(tokenizer, ast);
        strip_newlines(tokenizer);
        StmtBlock *ifBlock = parse_statement_block(tokenizer, ast);
        strip_newlines(tokenizer);
        ConditionBlock *elifBlocks = 0;
        //MBUF(ElIfBlock, elifBlocks);
        //elifBlocks = 0;
        StmtBlock *elseBlock = 0;
        while (match_token_name(tokenizer, string("elif")))
        {
            Expression *elifCondition = parse_expression(tokenizer, ast);
            strip_newlines(tokenizer);
            StmtBlock *elifBlock = parse_statement_block(tokenizer, ast);
            strip_newlines(tokenizer);
            mbuf_push(elifBlocks, ((ConditionBlock){elifCondition, elifBlock}));
        }
        if (match_token_name(tokenizer, string("else")))
        {
            strip_newlines(tokenizer);
            elseBlock = parse_statement_block(tokenizer, ast);
            strip_newlines(tokenizer);
        }
        result = create_if_stmt(ast, ifCondition, ifBlock, mbuf_count(elifBlocks), elifBlocks, elseBlock);
        mbuf_deallocate(elifBlocks);
    }
    else if (base.value == string("do"))
    {
        strip_newlines(tokenizer);
        StmtBlock *doBlock = parse_statement_block(tokenizer, ast);
        strip_newlines(tokenizer);
        expect_name(tokenizer, string("while"));
        Expression *doCond = parse_expression(tokenizer, ast);
        expect_token(tokenizer, Token_SemiColon);
        result = create_do_stmt(ast, doCond, doBlock);
    }
    else if (base.value == string("while"))
    {
        Expression *whileCond = parse_expression(tokenizer, ast);
        strip_newlines(tokenizer);
        StmtBlock *whileBlock = parse_statement_block(tokenizer, ast);
        result = create_while_stmt(ast, whileCond, whileBlock);
    }
    else if (base.value == string("for"))
    {
        expect_token(tokenizer, Token_ParenOpen);
        Statement *init = parse_statement(tokenizer, ast, false);
        i_expect(init->kind == Stmt_Assign);
        expect_token(tokenizer, Token_SemiColon);
        strip_newlines(tokenizer);
        Expression *cond = parse_expression(tokenizer, ast);
        expect_token(tokenizer, Token_SemiColon);
        strip_newlines(tokenizer);
        Statement *next = parse_statement(tokenizer, ast, false);
        i_expect(next->kind == Stmt_Assign);
        expect_token(tokenizer, Token_ParenClose);
        strip_newlines(tokenizer);
        StmtBlock *body = parse_statement_block(tokenizer, ast);
        result = create_for_stmt(ast, init, cond, next, body);
    }
    else if (base.value == string("func"))
    {
        Token name = expect_token(tokenizer, Token_Name);
        Token token = expect_token(tokenizer, Token_ParenOpen);
        
        String *arguments = 0;
        if (match_token(tokenizer, Token_ParenClose))
        {
            // NOTE(michiel): Do nothing
        }
        else
        {
            do
            {
                token = expect_token(tokenizer, Token_Name);
                mbuf_push(arguments, token.value);
            } while (match_token(tokenizer, Token_Comma));
            expect_token(tokenizer, Token_ParenClose);
        }
        
        strip_newlines(tokenizer);
        
        StmtBlock *stmtBody = parse_statement_block(tokenizer, ast);
        
        result = create_func_stmt(ast, name.value, mbuf_count(arguments), arguments, stmtBody);
        mbuf_deallocate(arguments);
    }
    else
    {
        AssignOpType opType = Assign_None;
        Expression *left = create_identifier(ast, base.value);
        Token op = get_token(tokenizer);
        switch (op.kind)
        {
            case Token_Assign:    { opType = Assign_Set; } break;
            case Token_AddAssign: { opType = Assign_Add; } break;
            case Token_SubAssign: { opType = Assign_Sub; } break;
            case Token_MulAssign: { opType = Assign_Mul; } break;
            case Token_DivAssign: { opType = Assign_Div; } break;
            case Token_ModAssign: { opType = Assign_Mod; } break;
            INVALID_DEFAULT_CASE;
        }
        Expression *right = parse_expression(tokenizer, ast);
        if (expectEnd) {
            expect_token(tokenizer, Token_SemiColon);
        }
        result = create_assign_stmt(ast, opType, left, right);
    }
    result->origin = base.origin;
    return result;
}

internal StmtBlock *
parse_statement_block(Tokenizer *tokenizer, Ast *ast)
{
    expect_token(tokenizer, Token_BraceOpen);
    strip_newlines(tokenizer);
    
    Statement **statements = 0;
    
    Token token = peek_token(tokenizer);
    while ((token.kind != Token_BraceClose) && (token.kind != Token_EOF))
    {
        Statement *statement = parse_statement(tokenizer, ast);
        mbuf_push(statements, statement);
        strip_newlines(tokenizer);
        token = peek_token(tokenizer);
    }
    
    expect_token(tokenizer, Token_BraceClose);
    
    StmtBlock *result = create_statement_block(ast, mbuf_count(statements), statements);
    mbuf_deallocate(statements);
    return result;
}

internal void
ast_from_tlk(Ast *ast, String filename, String text)
{
    Tokenizer tokenizer = {};
    tokenizer.origin.filename = filename;
    tokenizer.origin.line = 1;
    tokenizer.origin.column = 1;
    tokenizer.scanner = text;
    
    Statement **functions = 0;
    
    Token peekFunc = peek_token(&tokenizer);
    while ((peekFunc.kind != Token_EOF) && (peekFunc.kind == Token_Name) && (peekFunc.value == string("func")))
    {
        Statement *funcStmt = parse_statement(&tokenizer, ast);
        mbuf_push(functions, funcStmt);
        strip_newlines(&tokenizer);
        peekFunc = peek_token(&tokenizer);
    }
    
    ast->program.functionCount = mbuf_count(functions);
    ast->program.functionStmts = (Statement **)ast_allocate(ast, mbuf_count(functions)*sizeof(Statement *));
    copy(mbuf_count(functions)*sizeof(Statement *), functions, ast->program.functionStmts);
    
    mbuf_deallocate(functions);
}

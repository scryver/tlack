struct Expression;

enum DeclKind
{
    Decl_None,
    Decl_Int,
    //Decl_Float,
};
struct Declaration
{
    DeclKind kind;
    
    union {
        s64 intVal;
        //f64 floatVal;
    };
};

enum UnaryOpType
{
    Unary_None,
    Unary_Minus,
    Unary_Flip,
    Unary_Not,
};
struct UnaryOp
{
    UnaryOpType op;
    Expression *operand;
};

enum BinaryOpType
{
    Binary_None,
    Binary_Add,
    Binary_Sub,
    Binary_Mul,
    Binary_Div,
};
struct BinaryOp
{
    BinaryOpType op;
    Expression *left;
    Expression *right;
};

enum ExprKind
{
    Expr_None,
    Expr_Int,
    Expr_Identifier,
    Expr_Unary,
    Expr_Binary,
};
struct Expression
{
    //SourcePos origin;
    ExprKind kind;
    
    union {
        s64 intConst;
        String name;
        UnaryOp unary;
        BinaryOp binary;
        //Expr *nextFree;
    };
};

enum AssignOpType
{
    Assign_None,
    Assign_Set,
    Assign_Add,
    Assign_Sub,
    Assign_Mul,
    Assign_Div,
};
struct Assignment
{
    AssignOpType op;
    Expression *left; // TODO(michiel): Maybe make a variable?
    Expression *right;
};

enum StmtKind
{
    Stmt_None,
    Stmt_Assign,
    Stmt_Return,
};
struct Statement
{
    //SourcePos origin;
    StmtKind kind;
    
    union {
        Expression *expression;
        Assignment assign;
    };
};

struct StmtBlock
{
    u32 stmtCount;
    Statement **statements;
};

struct Function
{
    String name;
    StmtBlock *body;
};

struct Program
{
    Function *main;
};

struct Ast
{
    MemoryArena   arena;
    MemoryInterns interns;
    
    Program program;
    //StmtList statements;
    //Expr *exprFreeList;
    //Stmt *stmtFreeList;
};

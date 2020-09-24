struct Expression;
struct Statement;

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
    Binary_Mod,
    Binary_Eq,
    Binary_Ne,
    Binary_Lt,
    Binary_Gt,
    Binary_LE,
    Binary_GE,
};
struct BinaryOp
{
    BinaryOpType op;
    Expression *left;
    Expression *right;
};

struct FuncCall
{
    String name;
    u32 argCount;
    Expression **arguments;
};

enum ExprKind
{
    Expr_None,
    Expr_Int,
    Expr_Identifier,
    Expr_Unary,
    Expr_Binary,
    Expr_FuncCall,
    //Expr_Condition,??
};
struct Expression
{
    SourcePos origin;
    ExprKind kind;
    
    union {
        s64      eIntConst;
        String   eName;
        UnaryOp  eUnary;
        BinaryOp eBinary;
        FuncCall eFunction;
    };
};

struct StmtBlock
{
    u32 stmtCount;
    Statement **statements;
};

enum AssignOpType
{
    Assign_None,
    Assign_Set,
    Assign_Add,
    Assign_Sub,
    Assign_Mul,
    Assign_Div,
    Assign_Mod,
};
struct Assignment
{
    AssignOpType op;
    Expression  *left; // TODO(michiel): Maybe make a variable?
    Expression  *right;
};

struct ConditionBlock
{
    Expression *condition;
    StmtBlock  *block;
};

struct IfElse
{
    ConditionBlock  ifBlock;
    u32             elIfCount;
    ConditionBlock *elIfBlocks;
    StmtBlock      *elseBlock;
};

struct ForLoop
{
    Statement  *init;
    Expression *condition;
    Statement  *next;
    StmtBlock  *body;
};

struct Function
{
    SourcePos  origin;
    String     name;
    u32        argCount;
    String    *arguments; // TODO(michiel): type and stuff
    StmtBlock *body;
};

enum StmtKind
{
    Stmt_None,
    Stmt_Assign,
    Stmt_Return,
    Stmt_IfElse,
    Stmt_DoWhile,
    Stmt_While,
    Stmt_For,
    Stmt_Func,
};
struct Statement
{
    SourcePos origin;
    StmtKind kind;
    
    union {
        Expression    *sExpression;
        Assignment     sAssign;
        IfElse         sIf;
        ConditionBlock sDo;
        ConditionBlock sWhile;
        ForLoop        sFor;
        Function       sFunction;
    };
};

struct Program
{
    u32 functionCount;
    Statement **functionStmts;
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

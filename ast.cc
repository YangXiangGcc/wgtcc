#include "ast.h"

#include "code_gen.h"
#include "error.h"
#include "mem_pool.h"
#include "parser.h"
#include "token.h"
#include "visitor.h"


static MemPoolImp<BinaryOp>         binaryOpPool;
static MemPoolImp<ConditionalOp>    conditionalOpPool;
static MemPoolImp<FuncCall>         funcCallPool;
static MemPoolImp<Initialization>   initializationPool;
static MemPoolImp<Object>           objectPool;
static MemPoolImp<Identifier>       identifierPool;
static MemPoolImp<Enumerator>       enumeratorPool;
static MemPoolImp<Constant>         constantPool;
static MemPoolImp<TempVar>          tempVarPool;
static MemPoolImp<UnaryOp>          unaryOpPool;
static MemPoolImp<EmptyStmt>        emptyStmtPool;
static MemPoolImp<IfStmt>           ifStmtPool;
static MemPoolImp<JumpStmt>         jumpStmtPool;
static MemPoolImp<ReturnStmt>       returnStmtPool;
static MemPoolImp<LabelStmt>        labelStmtPool;
static MemPoolImp<CompoundStmt>     compoundStmtPool;
static MemPoolImp<FuncDef>          funcDefPool;

/*
 * Accept
 */

void Initialization::Accept(Visitor* v)
{
    v->VisitInitialization(this);
}

void EmptyStmt::Accept(Visitor* v) {
    //assert(false);
    //v->VisitEmptyStmt(this);
}

void LabelStmt::Accept(Visitor* v) {
    v->VisitLabelStmt(this);
}

void IfStmt::Accept(Visitor* v) {
    v->VisitIfStmt(this);
}

void JumpStmt::Accept(Visitor* v) {
    v->VisitJumpStmt(this);
}

void ReturnStmt::Accept(Visitor* v) {
    v->VisitReturnStmt(this);
}

void CompoundStmt::Accept(Visitor* v) {
    v->VisitCompoundStmt(this);
}

void BinaryOp::Accept(Visitor* v) {
    v->VisitBinaryOp(this);
}

void UnaryOp::Accept(Visitor* v) {
    v->VisitUnaryOp(this);
}

void ConditionalOp::Accept(Visitor* v) {
    v->VisitConditionalOp(this);
}

void FuncCall::Accept(Visitor* v) { 
    v->VisitFuncCall(this);
}

void Object::Accept(Visitor* v) {
    v->VisitObject(this);
}

void Constant::Accept(Visitor* v) {
    v->VisitConstant(this);
}

void Enumerator::Accept(Visitor* v)
{
    v->VisitEnumerator(this);
}

void TempVar::Accept(Visitor* v) {
    v->VisitTempVar(this);
}

void FuncDef::Accept(Visitor* v) {
    v->VisitFuncDef(this);
}

void TranslationUnit::Accept(Visitor* v) {
    v->VisitTranslationUnit(this);
}


BinaryOp* BinaryOp::New(const Token* tok, Expr* lhs, Expr* rhs)
{
    return New(tok, tok->Tag(), lhs, rhs);
}

BinaryOp* BinaryOp::New(const Token* tok, int op, Expr* lhs, Expr* rhs)
{
    switch (op) {
    case '.':
    case '=': 
    case ']': // SubScripting
    case '*':
    case '/':
    case '%':
    case '+':
    case '-':
    case '&':
    case '^':
    case '|':
    case '<':
    case '>':
    case Token::LEFT_OP:
    case Token::RIGHT_OP:
    case Token::LE_OP:
    case Token::GE_OP:
    case Token::EQ_OP:
    case Token::NE_OP: 
    case Token::AND_OP:
    case Token::OR_OP:
        break;

    default:
        assert(0);
    }

    auto ret = new (binaryOpPool.Alloc()) BinaryOp(op, lhs, rhs);
    ret->_pool = &binaryOpPool;
    
    ret->TypeChecking(tok);
    return ret;    
}


ArithmType* BinaryOp::Promote(const Token* errTok)
{
    // Both lhs and rhs are ensured to be have arithmetic type
    auto lhsType = _lhs->Type()->ToArithmType();
    auto rhsType = _rhs->Type()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _lhs = UnaryOp::New(errTok, Token::CAST, _lhs, type);
    }
    if (rhsType != type) {
        _rhs = UnaryOp::New(errTok, Token::CAST, _rhs, type);
    }
    
    return type;
}


/*
 * Type checking
 */

void BinaryOp::TypeChecking(const Token* errTok)
{
    switch (_op) {
    case '.':
        return MemberRefOpTypeChecking(errTok);

    case ']':
        return SubScriptingOpTypeChecking(errTok);

    case '*':
    case '/':
    case '%':
        return MultiOpTypeChecking(errTok);

    case '+':
    case '-':
        return AdditiveOpTypeChecking(errTok);

    case Token::LEFT_OP:
    case Token::RIGHT_OP:
        return ShiftOpTypeChecking(errTok);

    case '<':
    case '>':
    case Token::LE_OP:
    case Token::GE_OP:
        return RelationalOpTypeChecking(errTok);

    case Token::EQ_OP:
    case Token::NE_OP:
        return EqualityOpTypeChecking(errTok);

    case '&':
    case '^':
    case '|':
        return BitwiseOpTypeChecking(errTok);

    case Token::AND_OP:
    case Token::OR_OP:
        return LogicalOpTypeChecking(errTok);

    case '=':
        return AssignOpTypeChecking(errTok);

    default:
        assert(0);
    }
}

void BinaryOp::SubScriptingOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    if (nullptr == lhsType) {
        Error(errTok, "an pointer expected");
    }
    if (!_rhs->Type()->IsInteger()) {
        Error(errTok, "the operand of [] should be intger");
    }

    // The type of [] operator is the derived type
    _type = lhsType->Derived();    
}

void BinaryOp::MemberRefOpTypeChecking(const Token* errTok)
{
    _type = _rhs->Type();
}

void BinaryOp::MultiOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Type()->ToArithmType();
    auto rhsType = _rhs->Type()->ToArithmType();
    if (lhsType == nullptr || rhsType == nullptr) {
        Error(errTok, "operands should have arithmetic type");
    }

    if ('%' == _op && 
            !(_lhs->Type()->IsInteger()
            && _rhs->Type()->IsInteger())) {
        Error(errTok, "operands of '%%' should be integers");
    }

    //TODO: type promotion
    _type = Promote(errTok);
}

/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *     1. lhs of MINUS operator, and rhs must be integer or pointer;
 *     2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    auto rhsType = _rhs->Type()->ToPointerType();
    if (lhsType) {
        if (_op == Token::MINUS) {
            if ((rhsType && *lhsType != *rhsType)
                    || !_rhs->Type()->IsInteger()) {
                Error(errTok, "invalid operands to binary -");
            }
            _type = Type::NewArithmType(T_LONG); // ptrdiff_t
        } else if (!_rhs->Type()->IsInteger()) {
            Error(errTok, "invalid operands to binary -");
        } else {
            _type = _lhs->Type();
        }
    } else if (rhsType) {
        if (_op != Token::ADD || !_lhs->Type()->IsInteger()) {
            Error(errTok, "invalid operands to binary '%s'",
                    errTok->Str().c_str());
        }
        _type = _rhs->Type();
        std::swap(_lhs, _rhs); // To simplify code gen
    } else {
        auto lhsType = _lhs->Type()->ToArithmType();
        auto rhsType = _rhs->Type()->ToArithmType();
        if (lhsType == nullptr || rhsType == nullptr) {
            Error(errTok, "invalid operands to binary %s",
                    errTok->Str().c_str());
        }

        _type = Promote(errTok);
    }
}

void BinaryOp::ShiftOpTypeChecking(const Token* errTok)
{
    if (!_lhs->Type()->IsInteger() || !_rhs->Type()->IsInteger()) {
        Error(errTok, "expect integers for shift operator '%s'",
                errTok->Str());
    }

    Promote(errTok);
    _type = _lhs->Type();
}

void BinaryOp::RelationalOpTypeChecking(const Token* errTok)
{
    if (!_lhs->Type()->IsReal() || !_rhs->Type()->IsReal()) {
        Error(errTok, "expect integer/float"
                " for relational operator '%s'", errTok->Str());
    }

    Promote(errTok);
    _type = Type::NewArithmType(T_INT);    
}

void BinaryOp::EqualityOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    auto rhsType = _rhs->Type()->ToPointerType();
    if (lhsType || rhsType) {
        if (!lhsType->Compatible(*rhsType)) {
            Error(errTok, "incompatible pointers of operands");
        }
    } else {
        auto lhsType = _lhs->Type()->ToArithmType();
        auto rhsType = _rhs->Type()->ToArithmType();
        if (lhsType == nullptr || rhsType == nullptr) {
            Error(errTok, "invalid operands to binary %s",
                    errTok->Str());
        }

        Promote(errTok);
    }

    _type = Type::NewArithmType(T_INT);    
}

void BinaryOp::BitwiseOpTypeChecking(const Token* errTok)
{
    if (_lhs->Type()->IsInteger() || _rhs->Type()->IsInteger()) {
        Error(errTok, "operands of '&' should be integer");
    }
    
    _type = Promote(errTok);    
}

void BinaryOp::LogicalOpTypeChecking(const Token* errTok)
{
    if (!_lhs->Type()->IsScalar()
            || !_rhs->Type()->IsScalar()) {
        Error(errTok, "the operand should be arithmetic type or pointer");
    }
    
    Promote(errTok);
    _type = Type::NewArithmType(T_INT);
}

void BinaryOp::AssignOpTypeChecking(const Token* errTok)
{
    if (!_lhs->IsLVal()) {
        Error(errTok, "lvalue expression expected");
    } else if (_lhs->Type()->IsConst()) {
        Error(errTok, "can't modifiy 'const' qualified expression");
    } else if (!_lhs->Type()->Compatible(*_rhs->Type())) {
        Error(errTok, "uncompatible types");
    }
    // The other constraints are lefted to cast operator
    _rhs = UnaryOp::New(errTok,Token::CAST, _rhs, _lhs->Type());
    _type = _lhs->Type();
}


/*
 * Unary Operators
 */

UnaryOp* UnaryOp::New(const Token* tok,
        int op, Expr* operand, ::Type* type)
{
    auto ret = new (unaryOpPool.Alloc()) UnaryOp(op, operand, type);
    ret->_pool = &unaryOpPool;
    
    ret->TypeChecking(tok);
    return ret;
}

bool UnaryOp::IsLVal(void) {
    // only deref('*') could be lvalue;
    // so it's only deref will override this func
    return (_op == Token::DEREF && !Type()->ToArrayType());
}


void UnaryOp::TypeChecking(const Token* errTok)
{
    switch (_op) {
    case Token::POSTFIX_INC:
    case Token::POSTFIX_DEC:
    case Token::PREFIX_INC:
    case Token::PREFIX_DEC:
        return IncDecOpTypeChecking(errTok);

    case Token::ADDR:
        return AddrOpTypeChecking(errTok);

    case Token::DEREF:
        return DerefOpTypeChecking(errTok);

    case Token::PLUS:
    case Token::MINUS:
    case '~':
    case '!':
        return UnaryArithmOpTypeChecking(errTok);

    case Token::CAST:
        return CastOpTypeChecking(errTok);

    default:
        assert(false);
    }
}

void UnaryOp::IncDecOpTypeChecking(const Token* errTok)
{
    if (!_operand->IsLVal()) {
        Error(errTok, "lvalue expression expected");
    } else if (_operand->Type()->IsConst()) {
        Error(errTok, "can't modifiy 'const' qualified expression");
    }

    _type = _operand->Type();
}

void UnaryOp::AddrOpTypeChecking(const Token* errTok)
{
    FuncType* funcType = _operand->Type()->ToFuncType();
    if (funcType == nullptr && !_operand->IsLVal()) {
        Error(errTok, "expression must be an lvalue or function designator");
    }
    
    _type = Type::NewPointerType(_operand->Type());
}

void UnaryOp::DerefOpTypeChecking(const Token* errTok)
{
    auto pointerType = _operand->Type()->ToPointerType();
    if (pointerType == nullptr) {
        Error(errTok, "pointer expected for deref operator '*'");
    }

    _type = pointerType->Derived();    
}

void UnaryOp::UnaryArithmOpTypeChecking(const Token* errTok)
{
    if (Token::PLUS == _op || Token::MINUS == _op) {
        if (!_operand->Type()->IsArithm())
            Error(errTok, "Arithmetic type expected");
    } else if ('~' == _op) {
        if (!_operand->Type()->IsInteger())
            Error(errTok, "integer expected for operator '~'");
    } else if (!_operand->Type()->IsScalar()) {
        Error(errTok,
                "arithmetic type or pointer expected for operator '!'");
    }

    _type = _operand->Type();
}

void UnaryOp::CastOpTypeChecking(const Token* errTok)
{
    // The _type has been initiated to dest type
    if (!_type->IsScalar()) {
        Error(errTok, "the cast type should be arithemetic type or pointer");
    }

    if (_type->IsFloat() && _operand->Type()->ToPointerType()) {
        Error(errTok, "can't cast a pointer to floating");
    } else if (_type->ToPointerType() && _operand->Type()->IsFloat()) {
        Error(errTok, "can't cast a floating to pointer");
    }
}


/*
 * Conditional Operator
 */

ConditionalOp* ConditionalOp::New(const Token* tok,
        Expr* cond, Expr* exprTrue, Expr* exprFalse)
{
    auto ret = new (conditionalOpPool.Alloc())
            ConditionalOp(cond, exprTrue, exprFalse);
    ret->_pool = &conditionalOpPool;

    ret->TypeChecking(tok);
    return ret;
}


ArithmType* ConditionalOp::Promote(const Token* errTok)
{
    auto lhsType = _exprTrue->Type()->ToArithmType();
    auto rhsType = _exprFalse->Type()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _exprTrue = UnaryOp::New(errTok, Token::CAST, _exprTrue, type);
    }
    if (rhsType != type) {
        _exprFalse = UnaryOp::New(errTok, Token::CAST, _exprFalse, type);
    }
    
    return type;
}

void ConditionalOp::TypeChecking(const Token* errTok)
{
    if (!_cond->Type()->IsScalar()) {
        Error(errTok, "scalar is required");
    }

    auto lhsType = _exprTrue->Type();
    auto rhsType = _exprFalse->Type();
    if (!lhsType->Compatible(*rhsType)) {
        Error(errTok, "incompatible types of true/false expression");
    } else {
        _type = lhsType;
    }

    lhsType = lhsType->ToArithmType();
    rhsType = rhsType->ToArithmType();
    if (lhsType && rhsType) {
        _type = Promote(errTok);
    }
}


/*
 * Function Call
 */

FuncCall* FuncCall::New(const Token* tok,
        Expr* designator, const std::list<Expr*>& args)
{
    auto ret = new (funcCallPool.Alloc()) FuncCall(designator, args);
    ret->_pool = &funcCallPool;

    ret->TypeChecking(tok);
    return ret;
}


void FuncCall::TypeChecking(const Token* errTok)
{
    auto funcType = _designator->Type()->ToFuncType();
    if (funcType == nullptr) {
        Error(errTok, "'%s' is not a function", errTok->Str());
    }

    auto arg = _args.begin();
    for (auto paramType: funcType->ParamTypes()) {
        if (arg == _args.end()) {
            Error(errTok, "too few arguments for function ''");
        }

        if (!paramType->Compatible(*(*arg)->Type())) {
            // TODO(wgtdkp): function name
            Error(errTok, "incompatible type for argument 1 of ''");
        }

        ++arg;
    }
    
    if (arg != _args.end() && !funcType->Variadic()) {
        Error(errTok, "too many arguments for function ''");
    }
    
    _type = funcType->Derived();
}


/*
 * Identifier
 */

Identifier* Identifier::New(const Token* tok,
            ::Type* type, ::Scope* scope, enum Linkage linkage)
{
    auto ret = new (identifierPool.Alloc())
            Identifier(tok, type, scope, linkage);
    ret->_pool = &identifierPool;
    return ret;
}


std::string Identifier::Name(void) {
    return _tok->Str();
}


Enumerator* Enumerator::New(const Token* tok, ::Scope* scope, int val)
{
    auto ret = new (enumeratorPool.Alloc()) Enumerator(tok, scope, val);
    ret->_pool = &enumeratorPool;
    return ret;
}


Initialization* Initialization::New(Object* obj)
{
    auto ret = new (initializationPool.Alloc()) Initialization(obj);
    ret->_pool = &initializationPool;
    return ret;
}


/*
 * Object
 */

Object* Object::New(const Token* tok,
        ::Type* type, ::Scope* scope, int storage, enum Linkage linkage)
{
    auto ret = new (objectPool.Alloc())
            Object(tok, type, scope, storage, linkage);
    ret->_pool = &objectPool;
    return ret;
}


/*
 * Constant
 */

Constant* Constant::New(int tag, long val)
{
    auto type = Type::NewArithmType(tag);
    auto ret = new (constantPool.Alloc()) Constant(type, val);
    ret->_pool = &constantPool;
    return ret;
}

Constant* Constant::New(int tag, double val)
{
    auto type = Type::NewArithmType(tag);
    auto ret = new (constantPool.Alloc()) Constant(type, val);
    ret->_pool = &constantPool;
    return ret;
}

Constant* Constant::New(const std::string* val)
{
    static auto derived = Type::NewArithmType(T_CHAR);
    derived->SetQual(Q_CONST);
    static auto type = Type::NewPointerType(derived);

    auto ret = new (constantPool.Alloc()) Constant(type, val);
    ret->_pool = &constantPool;

    static long tag = 0;
    ret->_tag = tag++;

    return ret;
}


/*
 * TempVar
 */

TempVar* TempVar::New(::Type* type)
{
    auto ret = new (tempVarPool.Alloc()) TempVar(type);
    ret->_pool = &tempVarPool;
    return ret;
}


/*
 * Statement
 */

EmptyStmt* EmptyStmt::New(void)
{
    auto ret = new (emptyStmtPool.Alloc()) EmptyStmt();
    ret->_pool = &emptyStmtPool;
    return ret;
}

//else stmt Ĭ���� null
IfStmt* IfStmt::New(Expr* cond, Stmt* then, Stmt* els)
{
    auto ret = new (ifStmtPool.Alloc()) IfStmt(cond, then, els);
    ret->_pool = &ifStmtPool;
    return ret;
}

CompoundStmt* CompoundStmt::New(std::list<Stmt*>& stmts, ::Scope* scope)
{
    auto ret = new (compoundStmtPool.Alloc()) CompoundStmt(stmts, scope);
    ret->_pool = &compoundStmtPool;
    return ret;
}

JumpStmt* JumpStmt::New(LabelStmt* label)
{
    auto ret = new (jumpStmtPool.Alloc()) JumpStmt(label);
    ret->_pool = &jumpStmtPool;
    return ret;
}

ReturnStmt* ReturnStmt::New(Expr* expr)
{
    auto ret = new (returnStmtPool.Alloc()) ReturnStmt(expr);
    ret->_pool = &returnStmtPool;
    return ret;
}

LabelStmt* LabelStmt::New(void)
{
    auto ret = new (labelStmtPool.Alloc()) LabelStmt();
    ret->_pool = &labelStmtPool;
    return ret;
}

FuncDef* FuncDef::New(FuncType* type,
            const std::list<Object*>& params, CompoundStmt* stmt)
{
    auto ret = new (funcDefPool.Alloc()) FuncDef(type, params, stmt);
    ret->_pool = &funcDefPool;

    return ret;
}

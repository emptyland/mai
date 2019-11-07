#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"

namespace mai {
    
namespace nyaa {

namespace ast {

Location::Location(const NYAA_YYLTYPE &yyl)
    : begin_line(yyl.first_line)
    , end_line(yyl.last_line)
    , begin_column(yyl.first_column)
    , end_column(yyl.last_column) {
}

/*static*/ Location Location::Concat(const NYAA_YYLTYPE &first, const NYAA_YYLTYPE &last) {
    Location loc;
    loc.begin_line = first.first_line;
    loc.end_line = last.last_line;
    loc.begin_column = first.first_column;
    loc.end_column = last.last_column;
    return loc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// AST Serialization
////////////////////////////////////////////////////////////////////////////////////////////////////
class ASTSerializationVisitor : public ast::Visitor {
public:
    ASTSerializationVisitor(std::string *buf): buf_(DCHECK_NOTNULL(buf)) {
    }

    virtual ~ASTSerializationVisitor() override {}

    //----------------------------------------------------------------------------------------------
    // Implements from ast::Visitor
    //----------------------------------------------------------------------------------------------
    virtual IVal VisitLambdaLiteral(ast::LambdaLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteNames(node->params());
        WriteByte(node->vargs());
        WriteNode(node->value());
        return IVal::Void();
    }
    virtual IVal VisitIfStatement(ast::IfStatement *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->cond());
        WriteNode(node->then_clause());
        WriteNodeOptional(node->else_clause());
        return IVal::Void();
    }
    virtual IVal VisitForIterateLoop(ast::ForIterateLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteInt(node->trace_id());
        WriteNames(node->names());
        WriteNode(node->init());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitForStepLoop(ast::ForStepLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteInt(node->trace_id_base()); // only need base of trace_id
        WriteString(node->name());
        WriteNode(node->init());
        WriteByte(node->is_until());
        WriteNode(node->limit());
        WriteNodeOptional(node->step());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitWhileLoop(ast::WhileLoop *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteNode(node->cond());
        WriteNode(node->body());
        return IVal::Void();
    }
    virtual IVal VisitContinue(ast::Continue *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitBreak(ast::Break *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitMultiple(ast::Multiple *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteOperator(node->op());
        WriteInt(node->n_operands());
        for (int i = 0; i < node->n_operands(); ++i) {
            WriteNode(node->operand(i));
        }
        return IVal::Void();
    }
    virtual IVal VisitLogicSwitch(ast::LogicSwitch *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteOperator(node->op());
        WriteNode(node->lhs());
        WriteNode(node->rhs());
        return IVal::Void();
    }
    virtual IVal VisitObjectDefinition(ast::ObjectDefinition *node,
                                       ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteString(node->name());
        WriteByte(node->local());
        if (node->members()) {
            WriteI64(node->members()->size());
            for (auto stmt : *node->members()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitClassDefinition(ast::ClassDefinition *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteString(node->name());
        WriteByte(node->local());
        if (node->members()) {
            WriteI64(node->members()->size());
            for (auto stmt : *node->members()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitFunctionDefinition(ast::FunctionDefinition *node,
                                         ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNodeOptional(node->self());
        WriteString(node->name());
        //WriteNode(node->literal()); No need serialize lambda literal
        return IVal::Void();
    }
    virtual IVal VisitBlock(ast::Block *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        if (node->stmts()) {
            base::Slice::WriteVarint64(buf_, node->stmts()->size());
            for (auto stmt : *node->stmts()) {
                WriteNode(stmt);
            }
        } else {
            WriteByte(0);
        }
        return IVal::Void();
    }
    virtual IVal VisitVarDeclaration(ast::VarDeclaration *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteNames(node->names());
        WriteExprs(node->inits());
        return IVal::Void();
    }
    virtual IVal VisitPropertyDeclaration(ast::PropertyDeclaration *node,
                                          ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteNames(node->names());
        WriteExprs(node->inits());
        WriteByte(node->readonly());
        return IVal::Void();
    }
    virtual IVal VisitVariable(ast::Variable *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->name());
        return IVal::Void();
    }
    virtual IVal VisitAssignment(ast::Assignment *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteLvals(node->lvals());
        WriteExprs(node->rvals());
        return IVal::Void();
    }
    virtual IVal VisitIndex(ast::Index *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->self());
        WriteNode(node->index());
        return IVal::Void();
    }
    virtual IVal VisitDotField(ast::DotField *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->self());
        WriteString(node->index());
        return IVal::Void();
    }
    virtual IVal VisitSelfCall(ast::SelfCall *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteString(node->method());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitNew(ast::New *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitCall(ast::Call *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->trace_id());
        WriteNode(node->callee());
        WriteExprs(node->args());
        return IVal::Void();
    }
    virtual IVal VisitReturn(ast::Return *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteExprs(node->rets());
        return IVal::Void();
    }
    virtual IVal VisitNilLiteral(ast::NilLiteral *node, ast::VisitorContext *) override {
        WritePrefix(node);
        return IVal::Void();
    }
    virtual IVal VisitStringLiteral(ast::StringLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->value());
        return IVal::Void();
    }
    virtual IVal VisitApproxLiteral(ast::ApproxLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteF64(node->value());
        return IVal::Void();
    }
    virtual IVal VisitSmiLiteral(ast::SmiLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteI64(node->value());
        return IVal::Void();
    }
    virtual IVal VisitIntLiteral(ast::IntLiteral *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteString(node->value());
        return IVal::Void();
    }
    virtual IVal VisitMapInitializer(ast::MapInitializer *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteInt(node->end_line());
        WriteExprs(node->value());
        return IVal::Void();
    }
    virtual IVal VisitConcat(ast::Concat *node, ast::VisitorContext *x) override {
        WritePrefix(node);
        WriteExprs(node->operands());
        return IVal::Void();
    }
    virtual IVal VisitVariableArguments(ast::VariableArguments *node,
                                        ast::VisitorContext *x) override {
        WritePrefix(node);
        return IVal::Void();
    }
    
private:
    void WriteNode(ast::AstNode *node) { node->Accept(this, nullptr); }
    
    void WriteNodeOptional(ast::AstNode *node) {
        if (node) {
            WriteByte(1);
            WriteNode(node);
        } else {
            WriteByte(0);
        }
    }
    
    void WritePrefix(ast::Statement *stmt) {
        WriteKind(stmt->kind());
        WriteInt(stmt->line());
    }
    
    void WriteNames(const base::ArenaVector<const ast::String *> *names) {
        if (!names) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, names->size());
        for (const ast::String *name : *names) {
            WriteString(name);
        }
    }
    
    template<class T>
    void WriteExprs(const base::ArenaVector<T *> *exprs) {
        if (!exprs) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, exprs->size());
        for (ast::Expression *expr : *exprs) {
            WriteNode(expr);
        }
    }
    
    void WriteLvals(const base::ArenaVector<ast::LValue *> *exprs) {
        if (!exprs) {
            WriteByte(0);
            return;
        }
        base::Slice::WriteVarint64(buf_, exprs->size());
        for (ast::Expression *expr : *exprs) {
            WriteNode(expr);
        }
    }
    
    void WriteString(const ast::String *s) {
        base::Slice::WriteString(buf_, std::string_view(s->data(), s->size()));
    }
    
    void WriteKind(ast::AstNode::Kind kind) { WriteByte(static_cast<uint8_t>(kind)); }
    
    void WriteOperator(Operator::ID op) { WriteByte(static_cast<uint8_t>(op)); }
    
    void WriteInt(int n) { base::Slice::WriteVarint32(buf_, n); }
    
    void WriteF64(double n) { Write(&n, sizeof(n)); }
    
    void WriteI64(int64_t n) { base::Slice::WriteVarint64(buf_, n); }
    
    void Write(const void *p, size_t n) {
        buf_->append(reinterpret_cast<const char *>(p), n);
    }
    
    void WriteByte(uint8_t b) { buf_->append(1, static_cast<char>(b)); }
    
    std::string *buf_;
}; // class SerializationVisitor
    
class ASTSerializationLoader final {
public:
    ASTSerializationLoader(std::string_view buf, ast::Factory *factory)
        : rd_(buf)
        , factory_(factory) {}
    
    ast::AstNode *LoadNode() {
        ast::AstNode::Kind kind;
        int line;
        std::tie(kind, line) = ReadPrefix();
        switch (kind) {
        #define DEFINE_CALL_READ(name) \
            case ast::AstNode::k##name: \
                return Read##name(line);
                DECL_AST_NODES(DEFINE_CALL_READ)
        #undef DEFINE_CALL_READ
            default:
                NOREACHED() << kind << " line: " << line;
                break;
        }
        return nullptr;
    }
    
    ast::FunctionDefinition *ReadFunctionDefinition(int line) {
        return factory_->NewFunctionDefinition(ReadInt(), ReadExprOptional(), ReadString(),
                                               nullptr, Location(line));
    }
    
    ast::LambdaLiteral *ReadLambdaLiteral(int line) {
        int end_line = ReadInt();
        return factory_->NewLambdaLiteral(ReadNames(), ReadBool(), ReadBlock(),
                                          Location(line, end_line));
    }

    ast::ObjectDefinition *ReadObjectDefinition(int line) {
        int end_line = ReadInt();
        return factory_->NewObjectDefinition(ReadString(), ReadBool(), ReadStmts(),
                                             Location(line, end_line));
    }
    
    ast::ClassDefinition *ReadClassDefinition(int line) {
        int end_line = ReadInt();
        return factory_->NewClassDefinition(ReadString(), ReadBool(), ReadStmts(),
                                            Location(line, end_line));
    }
    
    ast::PropertyDeclaration *ReadPropertyDeclaration(int line) {
        return factory_->NewPropertyDeclaration(ReadNames(), ReadExprs(), ReadBool());
    }
    
    ast::Block *ReadBlock(int line) {
        int end_line = ReadInt();
        uint64_t n = rd_.ReadVarint64();
        ast::Block::StmtList *stmts = n == 0
            ? nullptr : factory_->NewList<ast::Statement *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            stmts->push_back(ReadStmt());
        }
        return factory_->NewBlock(stmts, Location(line, end_line));
    }
    
    ast::IfStatement *ReadIfStatement(int line) {
        return factory_->NewIfStatement(ReadInt(), ReadExpr(), ReadBlock(), ReadStmtOptional(),
                                        Location(line));
    }
    
    ast::ForStepLoop *ReadForStepLoop(int line) {
        int end_line = ReadInt();
        int next_trace_id = ReadInt();
        return factory_->NewForStepLoop(&next_trace_id, ReadString(), ReadExpr(), ReadBool(),
                                        ReadExpr(), ReadExprOptional(), ReadBlock(),
                                        Location(line, end_line));
    }
    
    ast::ForIterateLoop *ReadForIterateLoop(int line) {
        int end_line = ReadInt();
        return factory_->NewForIterateLoop(ReadInt(), ReadNames(), ReadExpr(), ReadBlock(),
                                           Location(line, end_line));
    }
    
    ast::WhileLoop *ReadWhileLoop(int line) {
        int end_line = ReadInt();
        return factory_->NewWhileLoop(ReadExpr(), ReadBlock(), Location(line, end_line));
    }
    
    ast::Break *ReadBreak(int line) { return factory_->NewBreak(Location(line)); }
    
    ast::Continue *ReadContinue(int line) { return factory_->NewContinue(Location(line)); }
    
    ast::Assignment *ReadAssignment(int line) {
        return factory_->NewAssignment(ReadLvals(), ReadExprs(), Location(line));
    }

    ast::Multiple *ReadMultiple(int line) {
        int trace_id = ReadInt();
        Operator::ID op = ReadOperator();
        int n = ReadInt();
        switch (n) {
            case 1:
                return factory_->NewUnary(trace_id, op, ReadExpr(), Location(line));
            case 2:
                return factory_->NewBinary(trace_id, op, ReadExpr(), ReadExpr(), Location(line));
            default:
                NOREACHED();
                break;
        }
        return nullptr;
    }
    
    ast::Concat *ReadConcat(int line) {
        return factory_->NewConcat(ReadExprs(), Location(line));
    }
    
    ast::LogicSwitch *ReadLogicSwitch(int line) {
        Operator::ID op = ReadOperator();
        switch (op) {
            case Operator::kOr:
                return factory_->NewOr(ReadExpr(), ReadExpr(), Location(line));
            case Operator::kAnd:
                return factory_->NewAnd(ReadExpr(), ReadExpr(), Location(line));
            default:
                NOREACHED();
                break;
        }
        return nullptr;
    }

    ast::Index *ReadIndex(int line) {
        return factory_->NewIndex(ReadInt(), ReadExpr(), ReadExpr(), Location(line));
    }
    
    ast::DotField *ReadDotField(int line) {
        return factory_->NewDotField(ReadInt(), ReadExpr(), ReadString(), Location(line));
    }
    
    ast::Variable *ReadVariable(int line) {
        return factory_->NewVariable(ReadString(), Location(line));
    }
    
    ast::VarDeclaration *ReadVarDeclaration(int line) {
        return factory_->NewVarDeclaration(ReadNames(), ReadExprs(), Location(line));
    }
    
    ast::SelfCall *ReadSelfCall(int line) {
        return factory_->NewSelfCall(ReadInt(), ReadExpr(), ReadString(), ReadExprs(),
                                     Location(line));
    }
    
    ast::New *ReadNew(int line) {
        return factory_->NewNew(ReadInt(), ReadExpr(), ReadExprs(), Location(line));
    }
    
    ast::Call *ReadCall(int line) {
        return factory_->NewCall(ReadInt(), ReadExpr(), ReadExprs(), Location(line));
    }
    
    ast::Return *ReadReturn(int line) {
        return factory_->NewReturn(ReadExprs(), Location(line));
    }
    
    ast::NilLiteral *ReadNilLiteral(int line) {
        return factory_->NewNilLiteral(Location(line));
    }
    
    ast::StringLiteral *ReadStringLiteral(int line) {
        return factory_->NewStringLiteral(ReadString(), Location(line));
    }
    
    ast::ApproxLiteral *ReadApproxLiteral(int line) {
        return factory_->NewApproxLiteral(rd_.ReadFloat64(), Location(line));
    }
    
    ast::IntLiteral *ReadIntLiteral(int line) {
        return factory_->NewIntLiteral(ReadString(), Location(line));
    }
    
    ast::SmiLiteral *ReadSmiLiteral(int line) {
        return factory_->NewSmiLiteral(rd_.ReadVarint64(), Location(line));
    }
    
    ast::MapInitializer *ReadMapInitializer(int line) {
        int end_line = ReadInt();
        return factory_->NewMapInitializer(ReadEntries(), Location(line, end_line));
    }
    
    ast::VariableArguments *ReadVariableArguments(int line) {
        return factory_->NewVariableArguments(Location(line));
    }
private:
    ast::Block *ReadBlock() { return DCHECK_NOTNULL(LoadNode()->ToBlock()); }
    
    ast::Expression *ReadExpr() {
        ast::AstNode *node = LoadNode();
        DCHECK(node->is_expression());
        return static_cast<ast::Expression *>(node);
    }
    
    ast::Expression *ReadExprOptional() {
        ast::AstNode *node = ReadNodeOptional();
        DCHECK(!node || node->is_expression());
        return static_cast<ast::Expression *>(node);
    }
    
    ast::Statement *ReadStmt() {
        ast::AstNode *node = LoadNode();
        DCHECK(node->is_statement());
        return static_cast<ast::Statement *>(node);
    }
    
    ast::Statement *ReadStmtOptional() {
        ast::AstNode *node = ReadNodeOptional();
        DCHECK(!node || node->is_statement());
        return static_cast<ast::Statement *>(node);
    }
    
    ast::AstNode *ReadNodeOptional() { return rd_.ReadByte() ? LoadNode() : nullptr; }

    base::ArenaVector<ast::Multiple *> *ReadEntries() { return ReadNodes<ast::Multiple>(); }
    
    base::ArenaVector<ast::LValue *> *ReadLvals() { return ReadNodes<ast::LValue>(); }
    
    base::ArenaVector<ast::Expression *> *ReadExprs() { return ReadNodes<ast::Expression>(); }
    
    base::ArenaVector<ast::Statement *> *ReadStmts() { return ReadNodes<ast::Statement>(); }
    
    template<class T>
    base::ArenaVector<T *> *ReadNodes() {
        uint64_t n = rd_.ReadVarint64();
        if (n == 0) {
            return nullptr;
        }
        base::ArenaVector<T *> *stmts = factory_->NewList<T *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            stmts->push_back(DCHECK_NOTNULL(T::Cast(LoadNode())));
        }
        return stmts;
    }
    
    base::ArenaVector<const ast::String *> *ReadNames() {
        uint64_t n = rd_.ReadVarint64();
        if (n == 0) {
            return nullptr;
        }
        base::ArenaVector<const ast::String *> *names =
            factory_->NewList<const ast::String *>(nullptr);
        for (uint64_t i = 0; i < n; ++i) {
            names->push_back(ReadString());
        }
        return names;
    }
    
    const ast::String *ReadString() {
        std::string_view s = rd_.ReadString();
        return factory_->NewRawString(s.data(), s.size());
    }
    
    bool ReadBool() { return rd_.ReadByte() != 0; }

    std::tuple<ast::AstNode::Kind, int> ReadPrefix() { return {ReadKind(), ReadInt()}; }
    
    ast::AstNode::Kind ReadKind() {
        return static_cast<ast::AstNode::Kind>(rd_.ReadByte());
    }
    
    Operator::ID ReadOperator() { return static_cast<Operator::ID>(rd_.ReadByte()); }

    int ReadInt() { return static_cast<int>(rd_.ReadVarint32()); }
    
    static ast::Location Location(int line) {
        ast::Location loc;
        loc.begin_line = line;
        return loc;
    }
    
    static ast::Location Location(int line, int end_line) {
        ast::Location loc;
        loc.begin_line = line;
        loc.end_line = end_line;
        return loc;
    }
    
    base::BufferReader rd_;
    ast::Factory *factory_;
}; // class ASTSerializationLoader

Error SerializeCompactedAST(ast::AstNode *node, std::string *buf) {
    ASTSerializationVisitor visitor(buf);
    DCHECK_NOTNULL(node)->Accept(&visitor, nullptr);
    return MAI_NOT_SUPPORTED("TODO:");
}
    
Error LoadCompactedAST(std::string_view buf, ast::Factory *factory, ast::AstNode **rv) {
    ASTSerializationLoader loader(buf, factory);
    *rv = loader.LoadNode();
    return Error::OK();
}

} //namespace ast

} // namespace nyaa
    
} // namespace mai

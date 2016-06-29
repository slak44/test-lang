#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include "ast.hpp"
#include "token.hpp"
#include "operator.hpp"

/*
  EBNF-ish format of a program:
  
  program = block ;
  block = [ statement, ";", { statement, ";" } ] ;
  statement = declaration | ( "define", function ) | for_loop | while_loop | block | if_statement | try_catch | throw_statement | expression ;
  declaration = "define" | type_list, ident, [ "=", expression ] ;
  for_loop = "for", expression, ";", expression, ";", expression, "do", block, "end" ;
  while_loop = "while", expression, "do", block, "end" ;
  if_statement = "if", expression, "do", block, [ "else", block | if_statement ] | "end" ;
  type_definition = "define", "type", ident, [ "inherits", type_list ], "do", [ contructor_definition ], [ { method_definition | member_definition } ], "end" ;
  constructor_definition = "define", "constructor", [ argument, {",", argument } ], "do", block, "end" ;
  method_definition = "define", [ visibility_specifier ], [ "static" ], function ;
  member_definition = "define", [ visibility_specifier ], [ "static" ], ident, [ "=", expression ] ;
  try_catch = "try", block, "catch", type_list, ident, "do", block, "end" ;
  throw_statement = "throw", expression ;
  
  function = "function", ident, [ argument, {",", argument } ], [ "=>", type_list ], "do", block, "end" ;
  visibility_specifier = "public" | "private" | "protected" ;
  type_list = ident, {",", ident} ;
  argument = ( "[", type_list, ident, "]" ) | ( "[", ident, ":", type_list, "]" ) ;
  expression = ? any expr ? ;
  function_call = ident, "(", expression, { ",", expression }, ")" ;
  ident = ? any valid identifier ? ;
*/

class Parser {
private:
  std::vector<Token> input;
  AST tree = AST();
  uint64 pos = 0;
  
  enum BlockType {
    ROOT_BLOCK, CODE_BLOCK, IF_BLOCK
  };
  
  inline Token current() {
    return input[pos];
  }
  // Skip a number of tokens, usually just advances to the next one
  inline void skip(int by = 1) {
    pos += by;
  }
  inline bool accept(TokenType tok) {
    return tok == current().type;
  }
  inline bool accept(std::string operatorName) {
    if (!current().isOperator()) return false;
    if (current().hasOperatorName(operatorName)) return true;
    return false;
  }
  inline bool accept(Fixity fixity) {
    return current().isOperator() && current().hasFixity(fixity);
  }
  inline bool acceptTerminal() {
    return current().isTerminal();
  }
  bool expect(TokenType tok, std::string errorMessage = "Unexpected symbol") {
    if (accept(tok)) {
      return true;
    } else {
      auto currentData = current().isOperator() ? current().getOperator().getName() : current().data;
      throw Error("SyntaxError", errorMessage + " (found: " + currentData + ")", current().line);
    }
  }
  inline Node<ExpressionNode>::Link exprFromCurrent() {
    return Node<ExpressionNode>::make(current());
  }
  Node<ExpressionNode>::Link expression() {
    return expressionImpl(parseExpressionPrimary(), 0);
  }
  Node<ExpressionNode>::Link parseExpressionPrimary() {
    Node<ExpressionNode>::Link expr;
    if (accept(C_PAREN_LEFT)) {
      skip();
      expr = expression();
      expect(C_PAREN_RIGHT, "Mismatched parenthesis");
      skip();
      return expr;
    } else if (acceptTerminal()) {
      expr = exprFromCurrent();
      skip();
      // Check if there are any postfix operators around
      if (accept(POSTFIX)) {
        expr->addChild(exprFromCurrent());
        skip();
      }
      return expr;
    // Prefix operators
    } else if (accept(PREFIX)) {
      auto lastNode = expr = exprFromCurrent();
      skip();
      while (accept(PREFIX)) {
        lastNode->addChild(exprFromCurrent());
        lastNode = lastNode->at(-1);
        skip();
      }
      lastNode->addChild(parseExpressionPrimary());
      return expr;
    } else {
      throw InternalError("Unimplemented primary expression", {METADATA_PAIRS, {"token", current().toString()}});
    }
  }
  Node<ExpressionNode>::Link expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence) {
    Node<ExpressionNode>::Link base = nullptr;
    
    Node<ExpressionNode>::Link lastExpr = nullptr;
    Token tok = current();
    while (tok.hasArity(BINARY) && tok.getPrecedence() >= minPrecedence) {
      auto tokExpr = Node<ExpressionNode>::make(tok);
      tokExpr->addChild(lhs);
      skip();
      auto rhs = parseExpressionPrimary();
      tok = current();
      while (tok.isOperator() && tok.hasArity(BINARY) &&
        (
          tok.getPrecedence() <= tokExpr->getToken().getPrecedence() ||
          (tok.getPrecedence() == tokExpr->getToken().getPrecedence() && tok.hasAssociativity(ASSOCIATE_FROM_RIGHT))
        )
      ) {
        tokExpr->addChild(rhs);
        tokExpr = expressionImpl(tokExpr, tok.getPrecedence());
        rhs = nullptr;
        tok = current();
      }
      lhs = rhs;
      if (base == nullptr) {
        base = lastExpr = tokExpr;
      } else {
        lastExpr->addChild(tokExpr);
        lastExpr = tokExpr;
      }
      if (accept(C_SEMI) || accept(C_PAREN_RIGHT) || accept(FILE_END)) break;
    }
    if (base == nullptr) {
      base = lhs;
    } else if (lhs != nullptr) {
      lastExpr->addChild(lhs);
    }
    return base;
  }
  void initialize(Node<DeclarationNode>::Link decl) {
    // Do initialization only if it exists
    if (accept("=")) {
      skip();
      decl->addChild(expression());
    }
  }
  inline void expectSemi() {
    expect(C_SEMI, "Expected semicolon");
    skip();
  }
  Node<DeclarationNode>::Link declarationFromTypes(TypeList typeList) {
    auto name = current().data;
    skip();
    auto decl = Node<DeclarationNode>::make(name, typeList);
    initialize(decl);
    expectSemi();
    return decl;
  }
  // This function assumes K_IF has been skipped
  Node<BranchNode>::Link parseIfStatement() {
    auto branch = Node<BranchNode>::make();
    branch->addCondition(expression());
    branch->addSuccessBlock(block(IF_BLOCK));
    skip(-1); // Go back to the block termination token
    if (accept(K_ELSE)) {
      skip();
      // Else-if structure
      if (accept(K_IF)) {
        skip();
        branch->addFailiureBlock(parseIfStatement());
      // Simple else block
      } else if (accept(K_DO)) {
        branch->addFailiureBlock(block(CODE_BLOCK));
      } else {
        throw Error("SyntaxError", "Else must be followed by a block or an if statement", current().line);
      }
    }
    return branch;
  }
  ASTNode::Link statement() {
    if (accept(K_DEFINE)) {
      skip();
      // Dynamic variable declaration
      if (accept(IDENTIFIER)) {
        auto name = current().data;
        skip();
        auto decl = Node<DeclarationNode>::make(name);
        initialize(decl);
        expectSemi();
        return decl;
      // Function declaration
      } else if (accept(K_FUNCTION)) {
        skip();
        // TODO
        throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "function def"}});
      } else {
        throw Error("SyntaxError", "Unexpected token after define keyword", current().line);
      }
    } else if (accept(IDENTIFIER)) {
      auto ident = current().data;
      skip();
      // Single-type declaration
      if (accept(IDENTIFIER)) {
        return declarationFromTypes({ident});
      }
      // Multi-type declaration
      if (accept(",")) {
        TypeList types = {ident};
        do {
          skip();
          expect(IDENTIFIER, "Expected identifier in type list");
          types.insert(current().data);
          skip();
        } while (accept(","));
        expect(IDENTIFIER);
        return declarationFromTypes(types);
      }
      skip(-1); // Undo the skip above, so the identifier is included in the expression
      return expression();
    } else if (accept(K_IF)) {
      skip();
      return parseIfStatement();
    } else if (accept(K_FOR)) {
      skip();
      // TODO
      throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "for loop"}});
    } if (accept(K_WHILE)) {
      skip();
      // TODO
      throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "while loop"}});
    } else if (accept(K_DO)) {
      return block(CODE_BLOCK);
    } else {
      return expression();
    }
  }
  Node<BlockNode>::Link block(BlockType type) {
    if (type != ROOT_BLOCK) expect(K_DO, "Expected code block");
    Node<BlockNode>::Link block = Node<BlockNode>::make();
    while (!accept(K_END)) {
      if (type == IF_BLOCK && accept(K_ELSE)) break;
      if (type == ROOT_BLOCK && accept(FILE_END)) break;
      block->addChild(statement());
    }
    skip(); // Skip block end
    return block;
  }
public:
  Parser() {}
  
  void parse(std::vector<Token> input) {
    this->input = input;
    pos = 0;
    tree = AST();
    tree.setRoot(*block(ROOT_BLOCK));
  }
  
  AST getTree() {
    return tree;
  }
};

#endif

#include "parser.h"
#include "error.h"
#include "vcc.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*! \brief A LL(k) parser*/

/**
 * S -> Expr
 * Expr -> MD ("+" MD | "-" MD)*
 * MD -> AS ("*" AS | "/" AS)*
 * AS ->"(" Expr ")" | num
 */

Node *root;
int node_num = 0;

static bool _at_end() { return token->kind == TK_EOF; }

static int _at_end_k(int k) {
  Token *token_copy = token;
  while (k--) {
    token_copy = token_copy->next;
    if (_at_end(token_copy))
      return true;
  }
  return false;
}

// check the current token

static bool _is_op() { return token->kind == TK_PUNC; }

static bool _is_integer() { return token->kind == TK_INTEGER; }

static bool _is_open_bracket() {
  return _is_op(token) && token->code[0] == '(';
}

static bool _is_closed_bracket() {
  return _is_op(token) && token->code[0] == ')';
}

static void _expect_closed_bracket() {
  if (!_is_closed_bracket(token)) {
    error_at(token->code, "Expect )");
  }
}

static void _expect_number() {
  if (!_is_integer(token)) {
    error_at(token->code, "Expect a number");
  }
}

static void _expect_not_at_end() {
  if (_at_end(token)) {
    report("Should not be at end");
  }
}

// consumes a token

static void _eat() { token = token->next; }

static void _eat_op() {
  if (!_is_op()) {
    error_at(token->code, "Expect an operator");
  }
  _eat();
}

static void _eat_integer() {
  if (!_is_integer()) {
    error_at(token->code, "Expect an integer");
  }
  _eat();
}

/*! \brief look ahead the next k tokens, check if the tokens can be concatenated
 * into c*/

bool lookahead(int k, char *c) {
  int siz = 0;
  Token *tmp_token = token;
  for (int i = 0; i < k; i++) {
    siz += tmp_token->len;
    tmp_token = tmp_token->next;
    _expect_not_at_end();
  }
  char text[siz + 1];
  int len = 0;
  tmp_token = token;
  for (int i = 0; i < k; i++) {
    strncpy(text + len, tmp_token->code, tmp_token->len);
    len += tmp_token->len;
    tmp_token = tmp_token->next;
  }
  text[siz] = '\0';
  if (!_at_end_k(k) && strncmp(text, c, siz) == 0) {
    while (k--)
      _eat();
    return true;
  }
  return false;
}

// AST creation

static Node *_new_number_node(int val) {
  Node *node = malloc(sizeof(Node));
  node->content.val = val;
  node->kind = ND_NUM;
  node->left = NULL;
  node->right = NULL;
  node_num++;
  return node;
}

static Node *_new_binary(NodeKind kind, Node *left, Node *right) {
  Node *root = malloc(sizeof(Node));
  root->kind = kind;
  switch (kind) {
  case ND_ADD:
    root->content.op = "+";
    break;
  case ND_SUB:
    root->content.op = "-";
    break;
  case ND_DIV:
    root->content.op = "/";
    break;
  case ND_MUL:
    root->content.op = "*";
    break;
  default:
    break;
  }
  root->left = left;
  root->right = right;
  node_num++;
  return root;
}

static Node *_add_or_sub();

static Node *_num_or_bracket() {
  Node *node;
  if (_is_open_bracket()) {
    _eat(); // eat the open bracket
    node = _add_or_sub();
    // check for the existence of the closed bracket
    _expect_closed_bracket();
    _eat(); // eat the closed bracket
  } else {
    _expect_number();
    node = _new_number_node(token->val);
    _eat(); // eat the number token
  }
  return node;
}

static Node *_mul_or_div() {
  Node *node = _num_or_bracket();
  while (!_at_end()) {
    if (lookahead(1, "*")) {
      node = _new_binary(ND_MUL, node, _num_or_bracket());
    } else if (lookahead(1, "/")) {
      node = _new_binary(ND_DIV, node, _num_or_bracket());
    } else {
      break;
    }
  }
  return node;
}

static Node *_add_or_sub() {
  Node *node = _mul_or_div();
  while (!_at_end()) {
    if (lookahead(1, "+")) {
      node = _new_binary(ND_ADD, node, _mul_or_div());
    } else if (lookahead(1, "-")) {
      node = _new_binary(ND_SUB, node, _mul_or_div());
    } else {
      break;
    }
  }
  return node;
}

void AST() { root = _add_or_sub(token); }
/**
 * @file xemmai grammar for tree-sitter
 * @author Shin-ichi MORITA
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

export default grammar({
  name: "xemmai",
  externals: $ => [$._newline, $.comment, $._indent, $._dedent],
  word: $ => $.symbol,
  rules: {
    source_file: $ => seq(
      seq(optional($.block), optional($._newline)),
      optional($._body)
    ),
    block: $ => seq($._indent, $._body, $._dedent),
    _body: $ => seq($._expression, repeat(seq($._newline, $._expression))),
    _expression: $ => seq(
      choice(seq(repeat1($._term), optional($.comment)), $.comment),
      optional($.block)
    ),
    _term: $ => choice(
      $.symbol,
      $.self,
      $.lambda,
      $.literal,
      $.keyword,
      $._other
    ),
    symbol: $ => /[A-Za-z_]\w*/,
    self: $ => "$",
    lambda: $ => "@",
    literal: $ => choice(
      "null",
      "true",
      "false",
      /\d+/,
      /"(\\.|[^"])*"/,
      /'[A-Za-z_]\w*/
    ),
    keyword: $ => choice(
      "break",
      "continue",
      "return",
      "throw",
      "if",
      "else",
      "while",
      "for",
      "try",
      "catch",
      "finally"
    ),
    _other: $ => choice(
      "!",
      "%",
      "&",
      "(",
      ")",
      "*",
      "+",
      ",",
      "-",
      ".",
      "/",
      ":",
      ";",
      "<",
      "=",
      ">",
      "?",
      "[",
      "]",
      "^",
      "{",
      "|",
      "}",
      "~"
    )
  }
});

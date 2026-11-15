#include "tree_sitter/parser.h"
#include "tree_sitter/alloc.h"
#include <ctype.h>
#include <string.h>

enum TokenType { NEWLINE, COMMENT, INDENT, DEDENT };

#define REMAIN (TREE_SITTER_SERIALIZATION_BUFFER_SIZE - sizeof(size_t) * 2)

typedef struct
{
	size_t length;
	size_t depth;
	int32_t indent[REMAIN / 2 / sizeof(int32_t)];
	size_t levels[REMAIN / 2 / sizeof(size_t)];
} Scanner;

void* tree_sitter_xemmai_external_scanner_create()
{
	return ts_calloc(1, sizeof(Scanner));
}

void tree_sitter_xemmai_external_scanner_destroy(void* payload)
{
	ts_free(payload);
}

unsigned tree_sitter_xemmai_external_scanner_serialize(void* payload, char* buffer)
{
	Scanner* scanner = (Scanner*)payload;
	size_t n0 = sizeof(size_t) * 2 + scanner->length * sizeof(int32_t);
	memcpy(buffer, scanner, n0);
	size_t n1 = (scanner->depth + 1) * sizeof(size_t);
	memcpy(buffer + n0, scanner->levels, n1);
	return (unsigned)(n0 + n1);
}

void tree_sitter_xemmai_external_scanner_deserialize(void* payload, const char* buffer, unsigned length)
{
	Scanner* scanner = (Scanner*)payload;
	size_t n = sizeof(size_t) * 2;
	if (length < n) {
		memset(scanner, 0, n);
		return;
	}
	memcpy(scanner, buffer, n);
	buffer += n;
	n = scanner->length * sizeof(int32_t);
	memcpy(scanner->indent, buffer, n);
	memcpy(scanner->levels, buffer + n, (scanner->depth + 1) * sizeof(size_t));
}

static bool mark_end(TSLexer* lexer, bool success)
{
	lexer->mark_end(lexer);
	return success;
}

bool tree_sitter_xemmai_external_scanner_scan(void* payload, TSLexer* lexer, const bool* valid_symbols)
{
	if (!(valid_symbols[NEWLINE] || valid_symbols[COMMENT] || valid_symbols[INDENT] || valid_symbols[DEDENT])) return false;
	Scanner* scanner = (Scanner*)payload;
	while (true) {
		if (scanner->length < scanner->levels[scanner->depth]) {
			if (scanner->length <= scanner->levels[--scanner->depth]) {
				lexer->result_symbol = DEDENT;
				return true;
			}
			scanner->levels[++scanner->depth] = scanner->length;
		}
		lexer->mark_end(lexer);
		bool newline = false;
		if (lexer->get_column(lexer) > 0) {
			while (lexer->lookahead != '\n' && isspace(lexer->lookahead)) lexer->advance(lexer, true);
			lexer->mark_end(lexer);
			if (lexer->lookahead == '\n') {
				lexer->advance(lexer, false);
				newline = true;
			}
		}
		size_t i;
		while (true) {
			i = 0;
			for (; i < scanner->length && lexer->lookahead != '\n' && isspace(lexer->lookahead); ++i) {
				if (lexer->lookahead != scanner->indent[i]) return mark_end(lexer, false);
				lexer->advance(lexer, false);
			}
			while (lexer->lookahead != '\n' && isspace(lexer->lookahead)) {
				scanner->indent[i++] = lexer->lookahead;
				lexer->advance(lexer, false);
			}
			if (lexer->lookahead != '\n') break;
			lexer->advance(lexer, false);
		}
		if (lexer->lookahead == '\0') {
			if (scanner->length <= 0) return mark_end(lexer, false);
			scanner->length = 0;
			continue;
		} else if (!newline && i <= 0) {
			if (lexer->lookahead != '#') return mark_end(lexer, false);
			do lexer->advance(lexer, false); while (lexer->lookahead != '\0' && lexer->lookahead != '\n');
			lexer->result_symbol = COMMENT;
			return mark_end(lexer, true);
		} else if (i == scanner->length) {
			lexer->result_symbol = NEWLINE;
			return mark_end(lexer, true);
		}
		scanner->length = i;
		if (i > scanner->levels[scanner->depth]) {
			scanner->levels[++scanner->depth] = i;
			lexer->result_symbol = INDENT;
			return mark_end(lexer, true);
		}
	}
}

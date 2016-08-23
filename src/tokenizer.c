/**
 *  This file is part of the IRCbot project.
 *  Copyright (C) 2007  Michal Kuchta
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

/**
 * Allocate tokens structure. It is used internally by tokenizer_tokenize
 * and created structure is returned.
 * @param tok Pointer to TOKENS. TOKENS shouldn't be initialized.
 */
void tokenizer_init(TOKENS *tok) {
	*tok = malloc(sizeof(sTOKENS));
	(*tok)->original = NULL;
	(*tok)->tokenized = NULL;
	(*tok)->separator = 0;
	(*tok)->length = 0;
	(*tok)->tokens = NULL;
	(*tok)->count = 0;
} // tokenizer_init

/**
 * Tokenize string
 * @param str String to tokenize
 * @param separator Tokens separator
 * @return TOKENS structure
 */
TOKENS tokenizer_tokenize(const char *str, const char separator) {
	TOKENS tok;
	tokenizer_init(&tok);

	tok->original = strdup(str);
	tok->tokenized = strdup(str);
	tok->separator = separator;

	char lastchar = 0;

	const int BUFF_SIZE = 16;
	tok->tokens = malloc(BUFF_SIZE * sizeof(int));
	if (tok->tokens == NULL) {
		free(tok->original);
		free(tok->tokenized);
		return NULL;
	}

	tok->allocated = BUFF_SIZE;

	// First token of string begins at 0.
	tok->tokens[tok->count] = 0;
	tok->count++;

	for (size_t i = 0; i <= strlen(str); i++) {
		// If current char is separator, end current token and proceed
		// to next one.
		if (str[i] == separator && lastchar != separator) {

			// Allocate new space if there is nothing left for
			// current token.
			if (tok->allocated <= tok->count) {
				tok->allocated += BUFF_SIZE;
				tok->tokens = realloc(tok->tokens,
					tok->allocated*sizeof(int));

				if (tok->tokens == NULL) {
					free(tok->original);
					free(tok->tokenized);
					return NULL;
				}
			}

			tok->tokenized[i] = '\0';
			tok->tokens[tok->count] = i+1;
			tok->count++;
		}
		lastchar = str[i];
	}

	return tok;
} // tokenizer_tokenize

/**
 * Get one token from string
 * @param tok TOKENS structure
 * @param num Token number
 * @return Token.
 */
char *tokenizer_gettok(TOKENS tok, size_t num) {
	if (num >= tok->count) {
		// Out of range
		return tok->original + strlen(tok->original);
	}

	return tok->tokenized + tok->tokens[num];
} // tokenizer_gettok

/**
 * Get all tokens exept num left ones.
 * @param tok TOKENS structure
 * @param num Number of tokens to skip
 * @return Tokens.
 */
char *tokenizer_gettok_skipleft(TOKENS tok, size_t num) {
	if (num >= tok->count) return tok->original + strlen(tok->original);
	if (num == 0) return tok->original;
	return tok->original + tok->tokens[num];
} // tokenizer_gettok_skipleft

/**
 * Free TOKENS structure
 * @param tok TOKENS structure
 */
void tokenizer_free(TOKENS tok) {
	// Free original string
	free(tok->original);

	// Free tokenized string
	free(tok->tokenized);

	// Free array of tokens
	free(tok->tokens);

	// Free tokens structure
	free(tok);
} // tokenizer_free

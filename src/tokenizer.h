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

#ifndef _TOKENIZER_H
#define _TOKENIZER_H 1

/**
 * Used by tokenizer as data storage.
 */
typedef struct {
	char *original;		/**< Original string. */
	char *tokenized;	/**< Original string with \0 instead of separators. */
	size_t length;		/**< Length of original string, because it cannot be
							 calculated when replacing separators with \0. */
	char separator;		/**< Char used to separate messages */
	int *tokens;		/**< Dynamically allocated array of tokens contains
							 starting position in original string of token. */
	size_t count;		/**< Number of tokens. */
	size_t allocated;	/**< Number of allocated array entrys. */
} sTOKENS;

/**
 * All tokenizer functions works with pointer to sTOKENS structure.
 */
typedef sTOKENS *TOKENS;

/**
 * Tokenize string
 * @param str String to tokenize
 * @param separator Tokens separator
 * @return TOKENS structure
 */
extern TOKENS tokenizer_tokenize(const char *str, const char separator);

/**
 * Free TOKENS structure
 * @param tok TOKENS structure
 */
extern void tokenizer_free(TOKENS tok);

/**
 * Get one token from string
 * @param tok TOKENS structure
 * @param num Token number
 * @return Token.
 */
char *tokenizer_gettok(TOKENS tok, size_t num);

/**
 * Get all tokens exept num left ones.
 * @param tok TOKENS structure
 * @param num Number of tokens to skip
 * @return Tokens.
 */
char *tokenizer_gettok_skipleft(TOKENS tok, size_t num);

#endif

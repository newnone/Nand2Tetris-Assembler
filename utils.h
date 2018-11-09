#ifndef UTILS_H
#define UTILS_H


#define ASCII_LETTERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define	IS_SPACE(c)	(c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v')


/**
 * Stores into `dest' `source' stripped of whitespaces (`\n', `\t', ...).
 *
 * Returns: `1' if an error occurred, `0' otherwise.
 */
int	n2t_strip(char const *source, char *dest);
/**
 * Strips away any single-line comments from `source', storing its altered form
 * to `dest'. It is assumed that `dest' has as much (or more) storage as
 * `source' does.
 *
 * Returns: `1' if an error occurred, `0' otherwise.
 */
int	n2t_decomment(char const *source, char *dest);
/**
 * Returns: `1' if `source' is composed entirely of `a-zA-Z' characters and
 * additional ones specified by `extra'.
 */
int n2t_is_alpha(char const *source, char const *extra);
/**
 * Returns: `1' if `source' is composed entirely of numeric characters, `0'
 * otherwise.
 */
int	n2t_is_numeric(char const *source);
/**
 * Returns: `1' if `s' is composed entirely of whitespaces, `0' otherwise.
 */
int n2t_is_whitespace(char const *s);
/**
 * Returns: `1' if `s' is composed of characters only found in `set', `0'
 * othewrise.
 */
int n2t_composed_of(char const *s, char const *set);


#endif

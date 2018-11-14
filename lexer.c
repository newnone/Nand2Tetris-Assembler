#include "lexer.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/**
 * Param `norm_repr': a normalized representation for the A instruction.
 * Param `dest': `instr_t' variable on which to store the decoded instruction.
 *
 * Returns: `1' if an error occurs, `0' otherwise.
 */
static int n2t_str_to_ainstr(char const *norm_repr, instr_t *dest);
/**
 * Param `norm_repr': a normalized representation for the C instruction.
 * Param `dest': `instr_t' variable on which to store the decoded instruction.
 *
 * Returns: `1' if an error occurs parsing the `dest' portion, `2' parsing the
 * `comp' portion and `3' if parsing the `jump' portion, `0' otherwise.
 */
static int n2t_str_to_cinstr(char const *const norm_repr, instr_t *dest);
/**
 * Parses the computation portion of a C-instruction.
 *
 * Param `norm_repr': a normalized representation for this ALU instruction.
 * Returns: the integer value associated to the ALU instruction or `-1' if no
 * correct parsing could be performed.
 */
static int n2t_parse_cinstr_comp(char const *norm_repr);


int n2t_instr_to_bitstr(instr_t const in, char *const dest) {
	int bitmask = 1 << 15, i = 0;

	while (bitmask > 0 && i < 16) {
		dest[i] = in.bits & bitmask ? '1': '0';

		bitmask >>= 1;
		i++;
	}

	dest[i] = '\0';

	return i + 1;
}

int n2t_str_to_instr(char const *str_repr, instr_t *dest) {
	char normalized[strlen(str_repr) + 1];

	if (n2t_strip(str_repr, normalized)) {
		return 1;
	}

	if (normalized[0] == '@') {
		return n2t_str_to_ainstr(normalized, dest);
	} else {
		return n2t_str_to_cinstr(normalized, dest);
	}
}

int n2t_str_to_label(char const *str_repr, label_t *dest) {
	size_t const len = strlen(str_repr);
	char copy[len - 1];

	if (str_repr[0] != '(' || str_repr[len - 1] != ')') {
		return 1;
	}

	strncpy(copy, str_repr + 1, len - 2);
	copy[len - 2] = '\0';

	if (n2t_is_alpha(copy, ".$_0123456789")) {
		strncpy(dest->text_repr, str_repr, BUFFSIZE_MED);

		return 0;
	} else {
		return 1;
	}
}

int n2t_tokenseq_full(tokenseq_t const *s) {
	return s->last >= s->ntokens;
}

int n2t_set_dest(instr_t *dest, int dest_reg) {
	if (0 <= dest_reg && dest_reg < 8) {
		dest->bits |= dest_reg << 3;

		return 0;
	}

	return 1;
}

int n2t_get_dest(instr_t *dest) {
	return (dest->bits && (7 << 3)) >> 3;
}

int n2t_set_jump(instr_t *dest, int jump_cond) {
	if (jump_cond <= 0 && jump_cond <= 7) {
		dest->bits |= jump_cond;

		return 0;
	} else {
		return 1;
	}
}

int n2t_get_jump(instr_t *dest) {
	return dest->bits & 7;
}

instr_type n2t_instr_type(instr_t const in) {
	// 0x8000 = 1 << 15 = 1000 0000 0000 0000 (binary form).
	if (in.bits & 0x8000) {
		return C;
	}

	return A;
}

tokenseq_t* n2t_tokenseq_alloc(size_t n) {
	tokenseq_t *o;

	if (n <= 0)
		return NULL;

	o = (tokenseq_t*) malloc(sizeof(tokenseq_t));
	if (o == NULL)
		return NULL;

	o->tokens = (void*) calloc(n, sizeof(token_t));
	if (o->tokens == NULL) {
		free(o);
		return NULL;
	}

	o->ntokens = n;
	o->last = 0;

	return o;
}

tokenseq_t* n2t_tokenseq_extend(tokenseq_t *s, size_t n) {
	token_t *t;

	if (n > 0) {
		t = realloc(s->tokens, sizeof(token_t) * (s->ntokens + n));

		if (t == NULL) {
			return NULL;
		} else {
			s->tokens = t;
			s->ntokens += n;
		}
	}

	return s;
}

int n2t_tokenseq_append_instr(tokenseq_t *s, instr_t const i) {
	if (s == NULL)
		return 1;

	s->tokens[s->last].data.instr = i;
	s->last++;

	return 0;
}

int n2t_tokenseq_append_label(tokenseq_t *s, label_t const l) {
	if (s == NULL)
		return 1;

	strncpy(
		s->tokens[s->last].data.label.text_repr, l.text_repr, BUFFSIZE_MED
	);
	s->tokens[s->last].data.label.location = l.location;
	s->last++;

	return 0;
}

void n2t_tokenseq_free(tokenseq_t *l) {
	free(l->tokens);
	free(l);
}

tokenseq_t* n2t_tokenize(const char *filepath) {
	FILE *fin;
	char buff[BUFFSIZE_LARGE];

	tokenseq_t *seq;
	instr_t i;
	label_t l;

	if ((fin = fopen(filepath, "r")) == NULL) {
		return NULL;
	}

	if ((seq = n2t_tokenseq_alloc(BUFFSIZE_MED)) == NULL) {
		fclose(fin);
		return NULL;
	}
	
	while (fgets(buff, BUFFSIZE_LARGE, fin)) {
		n2t_decomment(buff, buff);
		n2t_strip(buff, buff);

		// If `buff' contains nothing after taking away comments and
		// whitespaces:
		if (buff[0] == '\0')
			continue;

		if (n2t_tokenseq_full(seq)) {
			// Double the size at each new refill.
			n2t_tokenseq_extend(seq, seq->ntokens);
		}

		if (n2t_str_to_instr(buff, &i) == 0) {
			n2t_tokenseq_append_instr(seq, i);
			memset(&i, 0, sizeof(instr_t));
		} else if (n2t_str_to_label(buff, &l) == 0) {
			n2t_tokenseq_append_label(seq, l);
			memset(&l, 0, sizeof(label_t));
		}
	}

	fclose(fin);

	return seq;
}

static int n2t_str_to_ainstr(char const *norm_repr, instr_t *dest) {
	if (norm_repr[0] != '@') {
		return 1;	// Not an A-instruction.
	}

	if (n2t_is_numeric(norm_repr + 1) && norm_repr[1] != '0') {
		// `[1-9]\d*' digits.
		dest->bits = atoi(norm_repr + 1);
		dest->loaded = 1;
	} else if (	// @R0, @R1, ..., @R15
		norm_repr[1] == 'R' && n2t_is_numeric(norm_repr + 2) &&\
		atoi(norm_repr + 2) < 16
	) {
		dest->bits = atoi(norm_repr + 2);
		dest->loaded = 1;
	} else if (n2t_is_alpha(norm_repr + 1, "_")) {		// @LABEL, @label
		dest->loaded = 0;
	} else {
		return 1;
	}

	strncpy(dest->text_repr, norm_repr, BUFFSIZE_MED);

	return 0;
}

static int n2t_str_to_cinstr(char const *const norm_repr, instr_t *dest) {
	char const
		*const dest_field_tail = index(norm_repr, '='),
		*const jump_field_head = index(norm_repr, ';');
	char comp_field[strlen(norm_repr) + 1];
	int comp_encoding;
	size_t dest_offset = 0;

	char parsed_dest[3] = "   ";
	size_t parsed_dest_index = 0;

	memset(dest, 0, sizeof(instr_t));

	// Parse the `dest' part.
	// If '=' was not found in `norm_repr', `dest_field_tail' will be `NULL'.
	while (norm_repr + dest_offset < dest_field_tail) {
		if (!IS_SPACE(norm_repr[dest_offset])) {
			if (index(parsed_dest, norm_repr[dest_offset])) {
				return 1;	// We already parsed `*norm_repr'!
			}

			switch (norm_repr[dest_offset]) {
				case 'M':
					n2t_set_dest(dest, DEST_M);
					parsed_dest[parsed_dest_index] = 'M';
					break;
				case 'D':
					n2t_set_dest(dest, DEST_D);
					parsed_dest[parsed_dest_index] = 'D';
					break;
				case 'A':
					n2t_set_dest(dest, DEST_A);
					parsed_dest[parsed_dest_index] = 'A';
					break;
				default:
					return 1;
			}

			parsed_dest_index++;
		}

		dest_offset++;
	}

	// Parse the `jump' part.
	if (jump_field_head) {
		if (!strcmp(jump_field_head + 1, "JGT")) {
			n2t_set_jump(dest, JUMP_GT);
		} else if (!strcmp(jump_field_head + 1, "JEQ")) {
			n2t_set_jump(dest, JUMP_EQ);
		} else if (!strcmp(jump_field_head + 1, "JGE")) {
			n2t_set_jump(dest, JUMP_GE);
		} else if (!strcmp(jump_field_head + 1, "JLT")) {
			n2t_set_jump(dest, JUMP_LT);
		} else if (!strcmp(jump_field_head + 1, "JNE")) {
			n2t_set_jump(dest, JUMP_NE);
		} else if (!strcmp(jump_field_head + 1, "JLE")) {
			n2t_set_jump(dest, JUMP_LE);
		} else if (!strcmp(jump_field_head + 1, "JMP")) {
			n2t_set_jump(dest, JUMP_ALWAYS);
		} else {
			return 3;
		}
	}

	// Parse the `comp' part.
	if (dest_field_tail)
		strcpy(comp_field, dest_field_tail + 1);
	else
		strcpy(comp_field, norm_repr);

	if (jump_field_head)
		*index(comp_field, ';') = '\0';

	n2t_strip(comp_field, comp_field);
	n2t_collapse_any(comp_field, " \t", comp_field);

	if ((comp_encoding = n2t_parse_cinstr_comp(comp_field)) > -1) {
		dest->bits |= comp_encoding << 6;
	} else {
		return 2;
	}

	dest->bits |= (7 << 13);
	strncpy(dest->text_repr, norm_repr, BUFFSIZE_MED);

	return 0;
}

static int n2t_parse_cinstr_comp(char const *norm_repr) {
	if (!strcmp(norm_repr, "0")) {
		return 32 + 8 + 2;
	} else if (!strcmp(norm_repr, "1")) {
		return 63;
	} else if (!strcmp(norm_repr, "-1")) {
		return 32 + 16 + 8 + 2;
	} else if (!strcmp(norm_repr, "D")) {
		return 8 + 4;
	} else if (!strcmp(norm_repr, "A")) {
		return 32 + 16;
	} else if (!strcmp(norm_repr, "!D")) {
		return 8 + 4 + 1;
	} else if (!strcmp(norm_repr, "!A")) {
		return 32 + 16 + 1;
	} else if (!strcmp(norm_repr, "-D")) {
		return 8 + 4 + 2 + 1;
	} else if (!strcmp(norm_repr, "-A")) {
		return 32 + 16 + 2 + 1;
	} else if (!strcmp(norm_repr, "D+1")) {
		return 16 + 8 + 4 + 2 + 1;
	} else if (!strcmp(norm_repr, "A+1")) {
		return 32 + 16 + 4 + 2 + 1;
	} else if (!strcmp(norm_repr, "D-1")) {
		return 8 + 4 + 2;
	} else if (!strcmp(norm_repr, "A-1")) {
		return 32 + 16 + 2;
	} else if (!strcmp(norm_repr, "D+A")) {
		return 2;
	} else if (!strcmp(norm_repr, "D-A")) {
		return 16 + 2 + 1;
	} else if (!strcmp(norm_repr, "A-D")) {
		return 4 + 2 + 1;
	} else if (!strcmp(norm_repr, "D&A")) {
		return 0;
	} else if (!strcmp(norm_repr, "D|A")) {
		return 16 + 4 + 1;
	} else if (!strcmp(norm_repr, "M")) {
		return 64 + 32 + 16;
	} else if (!strcmp(norm_repr, "!M")) {
		return 64 + 32 + 16 + 1;
	} else if (!strcmp(norm_repr, "-M")) {
		return 64 + 32 + 16 + 2 + 1;
	} else if (!strcmp(norm_repr, "M+1")) {
		return 64 + 32 + 16 + 8 + 4 + 2;
	} else if (!strcmp(norm_repr, "M-1")) {
		return 64 + 32 + 16 + 2;
	} else if (!strcmp(norm_repr, "D+M")) {
		return 2;
	} else if (!strcmp(norm_repr, "D-M")) {
		return 16 + 2 + 1;
	} else if (!strcmp(norm_repr, "M-D")) {
		return 4 + 2 + 1;
	} else if (!strcmp(norm_repr, "D&M")) {
		return 64;
	} else if (!strcmp(norm_repr, "D|M")) {
		return 64 + 16 + 4 + 1;
	}

	return -1;
}

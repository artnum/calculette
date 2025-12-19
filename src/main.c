#include "include/array.h"
#include "include/operand.h"
#include <assert.h>
#include <math.h>
#include <notcurses/nckeys.h>
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum print_kind {
  KIND_DOUBLE,
  KIND_DECIMAL,
  KIND_HEXDECIMAL,
  KIND_OCTAL,
  KIND_BINARY
};

struct calc_value {
  double dbl;
  uint64_t in;
  bool can_dbl;
  bool can_int;
  bool likely_dbl;
  bool likely_int;
  enum print_kind print;
};

struct calc_op {
  int width;
  int reg;
  char *expression;
  bool highlight;
  struct calc_value result;
};

struct calc_var {
  char *name;
  struct calc_value value;
};

struct txt_input {
  char *content;
  size_t length;
  size_t capacity;
};

unsigned int rows = 0, cols = 0;
static uint64_t _hex_decode(char *ptr) {
  const char *d = "0123456789abcdefABCDEF";
  uint64_t r = 0;
  while (*ptr != '\0') {
    for (int i = 0; d[i] != '\0'; i++) {
      if (d[i] == *ptr) {
        r = (r << 4) | (i % 16);
      }
    }
    ptr++;
  }
  return r;
}

static void _compute_op(struct array *compute_stack, struct array *history,
                        struct calc_op *op) {
  char *str = strdup(op->expression);
  for (char *p = strtok(str, " "); p; p = strtok(NULL, " ")) {
    if (*p == ' ') {
      continue;
    }
    /* operand */
    for (int i = 0; OPERANDS[i].type < __CALC_MAX_OPERAND__; i++) {
      if (strcmp(OPERANDS[i].string, p) == 0) {
        if (array_size(compute_stack) <= 0) {
          break;
        }
        bool w_loaded = false;
        struct calc_value w;
        for (struct calc_value *v = array_pop(compute_stack); v;
             v = array_pop(compute_stack)) {
          if (!w_loaded) {
            memcpy(&w, v, sizeof(w));
            w_loaded = true;
            continue;
          }
          if (!v) {
            continue;
          }
          /* TODO : maybe simplify this, I think it's a bit messy */
          if (OPERANDS[i].ctype == CALC_INT && OPERANDS[i].in_op) {
            if (v->likely_int && w.likely_int) {
              w.in = OPERANDS[i].in_op(v->in, w.in);
              w.dbl = (double)w.in;
              w.print = v->print;
            }
          } else {
            if (w.likely_int && v->likely_int && OPERANDS[i].in_op) {
              w.in = OPERANDS[i].in_op(v->in, w.in);
              w.dbl = (double)w.in;
              w.print = v->print == w.print ? w.print : KIND_DECIMAL;
            } else if (OPERANDS[i].dbl_op) {
              w.dbl = OPERANDS[i].dbl_op(v->dbl, w.dbl);
              w.in = (int64_t)w.dbl;
              if (fmod(w.dbl, 1.0) == 0) {
                w.print = v->print == w.print ? w.print : KIND_DOUBLE;
              } else {
                w.print = KIND_DOUBLE;
              }
            }
          }
        }
        array_push(compute_stack, &w);
        break; /* for operands */
      }
    }

    if (*p == '_') {
      array_push(compute_stack,
                 &((struct calc_op *)array_get(history, op->reg - 2))->result);
      continue;
    }
    if (*p == ':' && *(p + 1) != '\0') {
      int id = atoi(p + 1);
      if (id >= 0 || id < array_size(history)) {
        array_push(compute_stack,
                   &((struct calc_op *)array_get(history, id - 1))->result);
      }
      continue;
    }

    if (strcmp("PI", p) == 0 || strcmp("π", p) == 0) {
      array_push(compute_stack, &(struct calc_value){M_PI});
    } else {
      if (strncmp("0X", p, 2) == 0 || strncmp("0x", p, 2) == 0) {
        struct calc_value v = {0};
        v.in = _hex_decode(p + 2);
        v.dbl = v.in;
        v.likely_int = true;
        v.likely_dbl = false;
        v.can_int = true;
        v.can_dbl = true;
        v.print = KIND_HEXDECIMAL;
        array_push(compute_stack, &v);
      } else {
        char *endptr_dbl = NULL;
        char *endptr_int = NULL;
        struct calc_value v = {0};
        v.dbl = strtod(p, &endptr_dbl);
        v.in = strtol(p, &endptr_int, 10);
        if (endptr_dbl > p || endptr_int > p) {
          v.can_dbl = endptr_dbl > p;
          v.can_int = endptr_int > p;
          v.likely_int = endptr_dbl == endptr_int;
          v.likely_dbl = !v.likely_int;
          array_push(compute_stack, &v);
        }
      }
    }
  }
  struct calc_value *w = array_pop(compute_stack);
  if (w) {
    memcpy(&op->result, w, sizeof(*w));
  }
  array_reset(compute_stack);
  free(str);
}

static void _print_op(struct ncplane *plane, struct calc_op *op, size_t ypos) {

  char line[200];
  ncplane_set_bg_alpha(plane, 255);
  uint64_t x = 0;
  ncchannels_set_fg_rgb8(&x, 128, 128, 127);
  if (op->highlight) {
    ncchannels_set_bg_rgb8(&x, 60, 60, 0);
    ncplane_set_bg_rgb8(plane, 60, 60, 0);
  } else {
    ncchannels_set_bg_rgb8(&x, 0, 0, 0);
    ncplane_set_bg_rgb8(plane, 0, 0, 0);
  }
  struct nccell c = NCCELL_INITIALIZER(' ', NCSTYLE_NONE, x);
  ncplane_cursor_move_yx(plane, ypos, 0);
  ncplane_hline(plane, &c, cols);
  ncplane_cursor_move_yx(plane, ypos, 0);
  ncplane_set_fg_rgb8(plane, 128, 128, 128);
  snprintf(line, 200, "%s%03d | ", op->highlight ? "•" : " ", op->reg);
  ncplane_putstr(plane, line);
  ncplane_set_fg_rgb8(plane, 0, 255, 0);
  switch (op->result.print) {
  default:
  case KIND_DOUBLE:
    ncplane_printf(plane, "%f", op->result.dbl);
    break;
  case KIND_DECIMAL:
    ncplane_printf(plane, "%ld", op->result.in);
    break;
  case KIND_HEXDECIMAL:
    ncplane_printf(plane, "0x%lx", op->result.in);
    break;
  case KIND_OCTAL:
    ncplane_printf(plane, "o%lo", op->result.in);
    break;
  }
  ncplane_set_fg_rgb8(plane, 128, 128, 0);
  ncplane_putstr_yx(plane, -1,
                    ncplane_dim_x(plane) -
                        ncstrwidth(op->expression, NULL, NULL) - 1,
                    op->expression);
  op->width = ncplane_x(plane) + 1;
}

static void _init_txt_prompt(struct txt_input *input) {
  if (input->capacity <= 2) {
    void *tmp = realloc(input->content, 3);
    if (!tmp) {
      abort();
    }
    input->content = tmp;
    input->capacity = 2;
    input->length = 2;
  }
  input->length = 2;
  snprintf(input->content, 3, "> ");
}

int main(int argc, char **argv) {
  struct array *history = array_new(sizeof(struct calc_op));
  struct array *compute_stack = array_new(sizeof(struct calc_value));
  if (!history) {
    return EXIT_FAILURE;
  }

  int stop = 0;
  struct notcurses *nc =
      notcurses_init(&(struct notcurses_options){.flags = 0}, NULL);

  if (!nc) {
    perror("notcurses_init");
    return EXIT_FAILURE;
  }

  ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);

  uint64_t input_channel = 0;
  ncchannels_set_fg_rgb8(&input_channel, 0x00, 0x00, 0x00);
  ncchannels_set_bg_rgb8(&input_channel, 0xA0, 0xA0, 0xA0);
  uint64_t default_channel = 0;
  ncchannels_set_fg_rgb8(&default_channel, 0x00, 0xFF, 0x00);
  ncchannels_set_bg_rgb8(&default_channel, 0x00, 0x00, 0x00);
  ncplane_set_channels(notcurses_stdplane(nc), default_channel);
  struct ncplane *input_field = ncplane_create(
      notcurses_stdplane(nc),
      &(struct ncplane_options){.x = 0, .y = 0, .cols = cols, .rows = 1});
  ncplane_set_base_cell(input_field, &(nccell)NCCELL_INITIALIZER(
                                         ' ', NCSTYLE_NONE, input_channel));
  ncplane_erase(input_field);

  ncplane_putstr(input_field, "> ");
  notcurses_cursor_enable(nc, 0, 2);

  int y = 0;
  struct txt_input input = {0};
  _init_txt_prompt(&input);
  int y_highlight = 0;
  do {
    struct ncinput event;

    notcurses_render(nc);
    uint32_t id = notcurses_get_blocking(nc, &event);

    if (id == NCKEY_RESIZE) {
      notcurses_refresh(nc, &rows, &cols);
      ncplane_erase(notcurses_stdplane(nc));
      for (size_t i = array_size(history); i > 0; i--) {
        _print_op(notcurses_stdplane(nc), array_get(history, i - 1),
                  array_size(history) - i + 1);
      }
      continue;
    }

    if (*event.utf8 == '\0' || *event.utf8 == '\033') {
      switch (event.id) {
      case NCKEY_BACKSPACE: {
        if (input.length <= 2) {
          break;
        }
        ncplane_cursor_move_rel(input_field, 0, -1);
        nccell c = {0};
        ncplane_at_cursor_cell(input_field, &c);
        input.length -= strlen(nccell_extended_gcluster(input_field, &c));
        input.content[input.length] = '\0';
        ncplane_erase(input_field);
        ncplane_putnstr(input_field, input.length, input.content);
        notcurses_cursor_enable(nc, 0, ncplane_cursor_x(input_field));

      } break;
      case NCKEY_DOWN: {
        if (array_size(history) <= 0) {
          break;
        }
        if (y_highlight > 0) {
          struct calc_op *p = array_get(history, y_highlight - 1);
          p->highlight = false;
        }
        if (y_highlight - 1 <= 0) {
          y_highlight = array_size(history);
        } else {
          y_highlight--;
        }
        struct calc_op *c = array_get(history, y_highlight - 1);
        c->highlight = true;
      } break;

      case NCKEY_ESC: {
        if (y_highlight > 0) {
          struct calc_op *p = array_get(history, y_highlight - 1);
          p->highlight = false;
        }
        y_highlight = 0;
      } break;
      case NCKEY_UP: {
        if (array_size(history) <= 0) {
          break;
        }
        if (y_highlight > 0) {
          struct calc_op *p = array_get(history, y_highlight - 1);
          p->highlight = false;
        }
        if (y_highlight + 1 > array_size(history)) {
          y_highlight = 1;
        } else {
          y_highlight++;
        }
        struct calc_op *c = array_get(history, y_highlight - 1);
        c->highlight = true;
      } break;

      case NCKEY_RETURN: {
        if (y_highlight > 0) {
          struct calc_op *p = array_get(history, y_highlight - 1);
          p->highlight = false;
          _init_txt_prompt(&input);
          memcpy(input.content + 2, p->expression, strlen(p->expression) + 1);
          input.content[strlen(p->expression) + 2] = '\0';
          input.length = strlen(p->expression) + 2;
          y_highlight = 0;
          break;
        }
        if (input.length <= 2) {
          break;
        }
        if (strcmp("quit", input.content + 2) == 0) {
          stop = true;
          break;
        }

        struct calc_op new_op = {0};

        new_op.expression = strndup(input.content + 2, input.length - 2);
        new_op.reg = array_size(history) + 1;
        _compute_op(compute_stack, history, &new_op);

        array_push(history, &new_op);
        _init_txt_prompt(&input);
      } break;
      }

      ncplane_erase(input_field);
      ncplane_putnstr(input_field, input.length, input.content);
      notcurses_cursor_enable(nc, ncplane_cursor_y(input_field),
                              ncplane_cursor_x(input_field));
      ncplane_erase(notcurses_stdplane(nc));
      for (size_t i = array_size(history); i > 0; i--) {
        _print_op(notcurses_stdplane(nc), array_get(history, i - 1),
                  array_size(history) - i + 1);
      }

      y++;
      notcurses_render(nc);

    } else {
      const size_t clen = strlen(event.utf8);
      if (input.capacity < input.length + clen + 1) {
        char *tmp = realloc(input.content, input.length + clen + 1);
        if (!tmp) {
          abort();
        }
        input.content = tmp;
        input.capacity = input.length + clen + 1;
      }
      strncat(input.content, event.utf8, clen);
      input.length += clen;
      ncplane_erase(input_field);
      ncplane_putnstr(input_field, input.length, input.content);

      notcurses_cursor_enable(nc, 0, ncplane_cursor_x(input_field));
    }

  } while (!stop);

  for (size_t i = 0; i < array_size(history); i++) {
    struct calc_op *op = array_get(history, i);
    if (op) {
      free(op->expression);
    }
  }
  array_destroy(history);
  array_destroy(compute_stack);
  free(input.content);

  notcurses_stop(nc);
  return EXIT_SUCCESS;
}

#ifndef OPERAND_H__
#define OPERAND_H__

#include <math.h>
#include <stdint.h>
enum calc_operand {
  CALC_NONE = 0,
  CALC_MULTIPLY,
  CALC_DIVIDE,
  CALC_ADD,
  CALC_SUBSTRACT,
  CALC_POWER,
  CALC_ROOT,
  CALC_PERCENT,
  CALC_PERCENT_ADD,
  CALC_PERCENT_SUB,

  CALC_AND,
  CALC_OR,
  CALC_XOR,
  CALC_SHIFTL,
  CALC_SHIFTR,
  CALC_ANDCMP,
  CALC_ORCMP,

  __CALC_MAX_OPERAND__
};

enum calc_type { CALC_DOUBLE = 0, CALC_INT = 1 };

struct operand {
  const char *string;
  enum calc_operand type;
  double (*dbl_op)(double, double);
  uint64_t (*in_op)(uint64_t, uint64_t);
  enum calc_type ctype;
};

static inline double _multiply(double a, double b) { return a * b; }
static inline double _divide(double a, double b) {
  return b == 0.0 ? 0.0 : a / b;
}
static inline double _add(double a, double b) { return a + b; }
static inline double _sub(double a, double b) { return a - b; }
static inline uint64_t _imultiply(uint64_t a, uint64_t b) { return a * b; }
static inline uint64_t _idivide(uint64_t a, uint64_t b) {
  return b == 0 ? 0 : a / b;
}
static inline uint64_t _iadd(uint64_t a, uint64_t b) { return a + b; }
static inline uint64_t _isub(uint64_t a, uint64_t b) { return a - b; }
static inline double _pow(double a, double b) { return pow(a, b); }
static inline double _root(double a, double b) {
  return b == 0.0 ? 0.0 : pow(a, 1 / b);
}
static inline double _percent(double a, double b) { return a * b / 100; }
static inline double _percent_add(double a, double b) {
  return a + (a * b / 100);
}
static inline double _percent_sub(double a, double b) {
  return a - (a * b / 100);
}
static inline uint64_t _and(uint64_t a, uint64_t b) { return a & b; }
static inline uint64_t _andcmp(uint64_t a, uint64_t b) { return a && b; }
static inline uint64_t _or(uint64_t a, uint64_t b) { return a | b; }
static inline uint64_t _orcmp(uint64_t a, uint64_t b) { return a || b; }
static inline uint64_t _xor(uint64_t a, uint64_t b) { return a ^ b; }
static inline uint64_t _shiftl(uint64_t a, uint64_t b) { return a << b; }
static inline uint64_t _shiftr(uint64_t a, uint64_t b) { return a >> b; }

const struct operand OPERANDS[] = {
    {.string = "*",
     .type = CALC_MULTIPLY,
     .dbl_op = _multiply,
     .in_op = _imultiply},
    {.string = "/", .type = CALC_DIVIDE, .dbl_op = _divide, .in_op = _idivide},
    {.string = "+", .type = CALC_ADD, .dbl_op = _add, .in_op = _iadd},
    {.string = "-", .type = CALC_SUBSTRACT, .dbl_op = _sub, .in_op = _isub},
    {.string = "**", .type = CALC_POWER, .dbl_op = _pow},
    {.string = "//", .type = CALC_ROOT, .dbl_op = _root},
    {.string = "%", .type = CALC_PERCENT, .dbl_op = _percent},
    {.string = "%+", .type = CALC_PERCENT_ADD, .dbl_op = _percent_add},
    {.string = "%-", .type = CALC_PERCENT_SUB, .dbl_op = _percent_sub},

    {.string = "&", .type = CALC_AND, .in_op = _and, .ctype = CALC_INT},
    {.string = "&&", .type = CALC_ANDCMP, .in_op = _andcmp, .ctype = CALC_INT},
    {.string = "|", .type = CALC_OR, .in_op = _or, .ctype = CALC_INT},
    {.string = "||", .type = CALC_ORCMP, .in_op = _orcmp, .ctype = CALC_INT},
    {.string = "^", .type = CALC_XOR, .in_op = _xor, .ctype = CALC_INT},
    {.string = "<<", .type = CALC_SHIFTL, .in_op = _shiftl, .ctype = CALC_INT},
    {.string = ">>", .type = CALC_SHIFTR, .in_op = _shiftr, .ctype = CALC_INT},

    /* closing entry */
    {.string = NULL, .type = __CALC_MAX_OPERAND__, .dbl_op = NULL}};

#endif /* OPERAND_H__ */

#ifndef NVIM_VIML_PARSER_PARSER_H
#define NVIM_VIML_PARSER_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include "nvim/lib/kvec.h"
#include "nvim/func_attr.h"

/// One parsed line
typedef struct {
  const char *data;  ///< Parsed line pointer
  size_t size;  ///< Parsed line size
} ParserLine;

/// Line getter type for parser
///
/// Line getter must return {NULL, 0} for EOF.
typedef void (*ParserLineGetter)(void *cookie, ParserLine *ret_pline);

/// Parser position in the input
typedef struct {
  size_t line;  ///< Line index in ParserInputReader.lines.
  size_t col;  ///< Byte index in the line.
} ParserPosition;

/// Parser state item.
typedef struct {
  enum {
    kPTopStateParsingCommand = 0,
    kPTopStateParsingExpression,
  } type;
  union {
    struct {
      enum {
        kExprUnknown = 0,
      } type;
    } expr;
  } data;
} ParserStateItem;

/// Structure defining input reader
typedef struct {
  /// Function used to get next line.
  ParserLineGetter get_line;
  /// Data for get_line function.
  void *cookie;
  /// All lines obtained by get_line.
  kvec_withinit_t(ParserLine, 4) lines;
} ParserInputReader;

/// Highlighted region definition
///
/// Note: one chunk may highlight only one line.
typedef struct {
  ParserPosition start;  ///< Start of the highlight: line and column.
  size_t end_col;  ///< End column, points to the start of the next character.
  const char *group;  ///< Highlight group.
} ParserHighlightChunk;

/// Highlighting defined by a parser
typedef kvec_withinit_t(ParserHighlightChunk, 16) ParserHighlight;

/// Structure defining parser state
typedef struct {
  /// Line reader.
  ParserInputReader reader;
  /// Position up to which input was parsed.
  ParserPosition pos;
  /// Parser state stack.
  kvec_withinit_t(ParserStateItem, 16) stack;
  /// Highlighting support.
  ParserHighlight *colors;
  /// True if line continuation can be used.
  bool can_continuate;
} ParserState;

static inline bool viml_parser_get_remaining_line(ParserState *const pstate,
                                                  ParserLine *const ret_pline)
  REAL_FATTR_ALWAYS_INLINE REAL_FATTR_WARN_UNUSED_RESULT REAL_FATTR_NONNULL_ALL;

/// Get currently parsed line, shifted to pstate->pos.col
///
/// @param  pstate  Parser state to operate on.
///
/// @return True if there is a line, false in case of EOF.
static inline bool viml_parser_get_remaining_line(ParserState *const pstate,
                                                  ParserLine *const ret_pline)
{
  const size_t num_lines = kv_size(pstate->reader.lines);
  if (pstate->pos.line == num_lines) {
    pstate->reader.get_line(pstate->reader.cookie, ret_pline);
    kvi_push(pstate->reader.lines, *ret_pline);
  } else {
    *ret_pline = kv_last(pstate->reader.lines);
  }
  assert(pstate->pos.line == kv_size(pstate->reader.lines) - 1);
  if (ret_pline->data != NULL) {
    ret_pline->data += pstate->pos.col;
    ret_pline->size -= pstate->pos.col;
  }
  return ret_pline->data != NULL;
}

static inline void viml_parser_advance(ParserState *const pstate,
                                       const size_t len)
  REAL_FATTR_ALWAYS_INLINE REAL_FATTR_NONNULL_ALL;

/// Advance position by a given number of bytes
///
/// At maximum advances to the next line.
///
/// @param  pstate  Parser state to advance.
/// @param[in]  len  Number of bytes to advance.
static inline void viml_parser_advance(ParserState *const pstate,
                                       const size_t len)
{
  assert(pstate->pos.line == kv_size(pstate->reader.lines) - 1);
  const ParserLine pline = kv_last(pstate->reader.lines);
  if (pstate->pos.col + len >= pline.size) {
    pstate->pos.line++;
    pstate->pos.col = 0;
  } else {
    pstate->pos.col += len;
  }
}

#endif  // NVIM_VIML_PARSER_PARSER_H

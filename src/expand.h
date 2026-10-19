#ifndef CROSSCC_SRC_EXPAND_H
#define CROSSCC_SRC_EXPAND_H

#include "support/cc_include.h"
#include "utils.h"

int expand( string * input, ... );

/* Expands the {LATEST} macro within span by searching for matching files or
 * directories on disk that fit `prefix + {version_number} + suffix`, where
 * prefix is input up to span->begin and suffix is input after span->end.
 *
 * Parameters:
 *   input: Pointer to string object containing the macro.
 *   span:  Pointer to strspan_t defining the bounds of the {LATEST} macro.
 *
 * Returns:
 *   void. On success, {LATEST} in input is replaced with the highest version
 *   substring found (sorted via strvicmp). If no matching file or directory
 *   is found, {LATEST} is erased from input.
 *
 * Failure modes:
 *   - If input, span, span->begin, or span->end is NULL, or if span->begin >= span->end,
 *     the function returns immediately without modifying input.
 *   - If span does not exactly match "{LATEST}" (extra characters inside braces),
 *     the macro is not expanded and is left untouched or erased.
 *   - If the directory cannot be opened, or no candidates match, the macro span is erased.
 */
void expand_latest( string * input, strspan_t * span );

/* Expands default macros (referencing settings from context) within span.
 *
 * Parameters:
 *   input: Pointer to string object containing the macro.
 *   span:  Pointer to strspan_t defining the bounds of the macro.
 *
 * Returns:
 *   void. On success, the macro in input is replaced with the corresponding
 *   setting value from context. If the setting is not found, the macro is erased.
 *
 * Failure modes:
 *   - If input, span, span->begin, or span->end is NULL, or if span->begin >= span->end,
 *     the function returns immediately without modifying input.
 *   - If the variable name is empty (e.g., "{}"), the macro is erased.
 *   - If the setting is not found in context, the macro is erased.
 */
void expand_default( string * input, strspan_t * span );

#endif

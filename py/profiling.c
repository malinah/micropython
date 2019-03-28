
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) SatoshiLabs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/runtime.h"

#if MICROPY_PY_SYS_TRACE

#include "py/profiling.h"
#include "py/objstr.h"
#include "py/objfun.h"
#include "py/emitglue.h"
#include "py/runtime.h"
#include "py/bc0.h"

#include <stdio.h>
#include <string.h>

#define DECODE_UINT { \
    unum = 0; \
    do { \
        unum = (unum << 7) + (*ip & 0x7f); \
    } while ((*ip++ & 0x80) != 0); \
}
#define DECODE_ULABEL do { unum = (ip[0] | (ip[1] << 8)); ip += 2; } while (0)
#define DECODE_SLABEL do { unum = (ip[0] | (ip[1] << 8)) - 0x8000; ip += 2; } while (0)

#define DECODE_QSTR \
    qst = ip[0] | ip[1] << 8; \
    ip += 2;
#define DECODE_PTR \
    DECODE_UINT; \
    ptr = (const byte*)const_table[unum]
#define DECODE_OBJ \
    DECODE_UINT; \
    obj = (mp_obj_t)const_table[unum]

const byte *prof_opcode_decode(const byte *ip, const mp_uint_t *const_table, mp_dis_instruction *instruction) {
    mp_uint_t unum;
    const byte* ptr;
    mp_obj_t obj;
    qstr qst;

    instruction->opname = mp_const_none;
    instruction->arg = mp_const_none;
    instruction->argval = mp_const_none;
    instruction->cache = mp_const_none;

    switch (*ip++) {
        case MP_BC_LOAD_CONST_FALSE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_FALSE);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_FALSE);
            break;

        case MP_BC_LOAD_CONST_NONE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_NONE);
            break;

        case MP_BC_LOAD_CONST_TRUE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_TRUE);
            break;

        case MP_BC_LOAD_CONST_SMALL_INT: {
            mp_int_t num = 0;
            if ((ip[0] & 0x40) != 0) {
                // Number is negative
                num--;
            }
            do {
                num = (num << 7) | (*ip & 0x7f);
            } while ((*ip++ & 0x80) != 0);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_SMALL_INT);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(num);
            break;
        }

        case MP_BC_LOAD_CONST_STRING:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_STRING);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_LOAD_CONST_OBJ:
            DECODE_OBJ;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_OBJ);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            instruction->argval = obj;
            break;

        case MP_BC_LOAD_NULL:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_NULL);
            break;

        case MP_BC_LOAD_FAST_N:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_FAST_N);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_LOAD_DEREF:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_DEREF);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_LOAD_NAME:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_NAME);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                instruction->cache = MP_OBJ_NEW_SMALL_INT(*ip++);
            }
            break;

        case MP_BC_LOAD_GLOBAL:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_GLOBAL);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                instruction->cache = MP_OBJ_NEW_SMALL_INT(*ip++);
            }
            break;

        case MP_BC_LOAD_ATTR:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_ATTR);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                instruction->cache = MP_OBJ_NEW_SMALL_INT(*ip++);
            }
            break;

        case MP_BC_LOAD_METHOD:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_METHOD);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_LOAD_SUPER_METHOD:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_SUPER_METHOD);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_LOAD_BUILD_CLASS:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_BUILD_CLASS);
            break;

        case MP_BC_LOAD_SUBSCR:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_SUBSCR);
            break;

        case MP_BC_STORE_FAST_N:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_FAST_N);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_STORE_DEREF:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_DEREF);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_STORE_NAME:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_NAME);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_STORE_GLOBAL:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_GLOBAL);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_STORE_ATTR:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_ATTR);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                instruction->cache = MP_OBJ_NEW_SMALL_INT(*ip++);
            }
            break;

        case MP_BC_STORE_SUBSCR:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_SUBSCR);
            break;

        case MP_BC_DELETE_FAST:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DELETE_FAST);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_DELETE_DEREF:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DELETE_DEREF);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_DELETE_NAME:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DELETE_NAME);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_DELETE_GLOBAL:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DELETE_GLOBAL);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_DUP_TOP:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DUP_TOP);
            break;

        case MP_BC_DUP_TOP_TWO:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_DUP_TOP_TWO);
            break;

        case MP_BC_POP_TOP:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_POP_TOP);
            break;

        case MP_BC_ROT_TWO:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_ROT_TWO);
            break;

        case MP_BC_ROT_THREE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_ROT_THREE);
            break;

        case MP_BC_JUMP:
            DECODE_SLABEL;
            // printf("JUMP " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_JUMP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_POP_JUMP_IF_TRUE:
            DECODE_SLABEL;
            // printf("POP_JUMP_IF_TRUE " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_POP_JUMP_IF_TRUE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_POP_JUMP_IF_FALSE:
            DECODE_SLABEL;
            // printf("POP_JUMP_IF_FALSE " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_POP_JUMP_IF_FALSE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_JUMP_IF_TRUE_OR_POP:
            DECODE_SLABEL;
            // printf("JUMP_IF_TRUE_OR_POP " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_JUMP_IF_TRUE_OR_POP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_JUMP_IF_FALSE_OR_POP:
            DECODE_SLABEL;
            // printf("JUMP_IF_FALSE_OR_POP " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_JUMP_IF_FALSE_OR_POP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_SETUP_WITH:
            DECODE_ULABEL; // loop-like labels are always forward
            // printf("SETUP_WITH " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_SETUP_WITH);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_WITH_CLEANUP:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_WITH_CLEANUP);
            break;

        case MP_BC_UNWIND_JUMP:
            DECODE_SLABEL;
            // printf("UNWIND_JUMP " UINT_FMT " %d", (mp_uint_t)(ip + unum - code_start), *ip);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_UNWIND_JUMP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_SETUP_EXCEPT:
            DECODE_ULABEL; // except labels are always forward
            // printf("SETUP_EXCEPT " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_SETUP_EXCEPT);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_SETUP_FINALLY:
            DECODE_ULABEL; // except labels are always forward
            // printf("SETUP_FINALLY " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_SETUP_FINALLY);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_END_FINALLY:
            // if TOS is an exception, reraises the exception (3 values on TOS)
            // if TOS is an integer, does something else
            // if TOS is None, just pops it and continues
            // else error
            instruction->opname = MP_ROM_QSTR(MP_QSTR_END_FINALLY);
            break;

        case MP_BC_GET_ITER:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_GET_ITER);
            break;

        case MP_BC_GET_ITER_STACK:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_GET_ITER_STACK);
            break;

        case MP_BC_FOR_ITER:
            DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
            // printf("FOR_ITER " UINT_FMT, (mp_uint_t)(ip + unum - code_start));
            instruction->opname = MP_ROM_QSTR(MP_QSTR_FOR_ITER);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_POP_BLOCK:
            // pops block and restores the stack
            instruction->opname = MP_ROM_QSTR(MP_QSTR_POP_BLOCK);
            break;

        case MP_BC_POP_EXCEPT:
            // pops block, checks it's an exception block, and restores the stack, saving the 3 exception values to local threadstate
            instruction->opname = MP_ROM_QSTR(MP_QSTR_POP_EXCEPT);
            break;

        case MP_BC_BUILD_TUPLE:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_BUILD_TUPLE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_BUILD_LIST:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_BUILD_LIST);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_BUILD_MAP:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_BUILD_MAP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_STORE_MAP:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_MAP);
            break;

        case MP_BC_BUILD_SET:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_BUILD_SET);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

#if MICROPY_PY_BUILTINS_SLICE
        case MP_BC_BUILD_SLICE:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_BUILD_SLICE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;
#endif

        case MP_BC_STORE_COMP:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_COMP);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_UNPACK_SEQUENCE:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_UNPACK_SEQUENCE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_UNPACK_EX:
            DECODE_UINT;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_UNPACK_EX);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_MAKE_FUNCTION:
            DECODE_PTR;
            // printf("MAKE_FUNCTION %p", (void*)(uintptr_t)unum);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_MAKE_FUNCTION);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            instruction->argval = mp_obj_new_int_from_ull((uint64_t)ptr);
            break;

        case MP_BC_MAKE_FUNCTION_DEFARGS:
            DECODE_PTR;
            // printf("MAKE_FUNCTION_DEFARGS %p", (void*)(uintptr_t)unum);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_MAKE_FUNCTION_DEFARGS);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            instruction->argval = mp_obj_new_int_from_ull((uint64_t)ptr);
            break;

        case MP_BC_MAKE_CLOSURE: {
            DECODE_PTR;
            mp_uint_t n_closed_over = *ip++;
            // printf("MAKE_CLOSURE %p " UINT_FMT, (void*)(uintptr_t)unum, n_closed_over);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_MAKE_CLOSURE);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            instruction->argval = mp_obj_new_int_from_ull((uint64_t)ptr);
            instruction->cache = MP_OBJ_NEW_SMALL_INT(n_closed_over);
            break;
        }

        case MP_BC_MAKE_CLOSURE_DEFARGS: {
            DECODE_PTR;
            mp_uint_t n_closed_over = *ip++;
            // printf("MAKE_CLOSURE_DEFARGS %p " UINT_FMT, (void*)(uintptr_t)unum, n_closed_over);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_MAKE_CLOSURE_DEFARGS);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            instruction->argval = mp_obj_new_int_from_ull((uint64_t)ptr);
            instruction->cache = MP_OBJ_NEW_SMALL_INT(n_closed_over);
            break;
        }

        case MP_BC_CALL_FUNCTION:
            DECODE_UINT;
            // printf("CALL_FUNCTION n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_CALL_FUNCTION);
            // instruction->arg = MP_OBJ_NEW_SMALL_INT(unum & 0xff);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum & 0xff);
            instruction->cache = MP_OBJ_NEW_SMALL_INT((unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_FUNCTION_VAR_KW:
            DECODE_UINT;
            // printf("CALL_FUNCTION_VAR_KW n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_CALL_FUNCTION_VAR_KW);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum & 0xff);
            instruction->cache = MP_OBJ_NEW_SMALL_INT((unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_METHOD:
            DECODE_UINT;
            // printf("CALL_METHOD n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_CALL_METHOD);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum & 0xff);
            instruction->cache = MP_OBJ_NEW_SMALL_INT((unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_METHOD_VAR_KW:
            DECODE_UINT;
            // printf("CALL_METHOD_VAR_KW n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            instruction->opname = MP_ROM_QSTR(MP_QSTR_CALL_METHOD_VAR_KW);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum & 0xff);
            instruction->cache = MP_OBJ_NEW_SMALL_INT((unum >> 8) & 0xff);
            break;

        case MP_BC_RETURN_VALUE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_RETURN_VALUE);
            break;

        case MP_BC_RAISE_VARARGS:
            unum = *ip++;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_RAISE_VARARGS);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(unum);
            break;

        case MP_BC_YIELD_VALUE:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_YIELD_VALUE);
            break;

        case MP_BC_YIELD_FROM:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_YIELD_FROM);
            break;

        case MP_BC_IMPORT_NAME:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_IMPORT_NAME);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_IMPORT_FROM:
            DECODE_QSTR;
            instruction->opname = MP_ROM_QSTR(MP_QSTR_IMPORT_FROM);
            instruction->arg = MP_OBJ_NEW_SMALL_INT(qst);
            instruction->argval = MP_OBJ_NEW_QSTR(qst);
            break;

        case MP_BC_IMPORT_STAR:
            instruction->opname = MP_ROM_QSTR(MP_QSTR_IMPORT_STAR);
            break;

        default:
            if (ip[-1] < MP_BC_LOAD_CONST_SMALL_INT_MULTI + 64) {
                // printf("LOAD_CONST_SMALL_INT " INT_FMT, (mp_int_t)ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16);
                instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_CONST_SMALL_INT);
                instruction->arg = MP_OBJ_NEW_SMALL_INT((mp_int_t)ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16);
            } else if (ip[-1] < MP_BC_LOAD_FAST_MULTI + 16) {
                // printf("LOAD_FAST " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_LOAD_FAST_MULTI);
                instruction->opname = MP_ROM_QSTR(MP_QSTR_LOAD_FAST);
                instruction->arg = MP_OBJ_NEW_SMALL_INT((mp_uint_t)ip[-1] - MP_BC_LOAD_FAST_MULTI);
            } else if (ip[-1] < MP_BC_STORE_FAST_MULTI + 16) {
                // printf("STORE_FAST " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_STORE_FAST_MULTI);
                instruction->opname = MP_ROM_QSTR(MP_QSTR_STORE_FAST);
                instruction->arg = MP_OBJ_NEW_SMALL_INT((mp_uint_t)ip[-1] - MP_BC_STORE_FAST_MULTI);
            } else if (ip[-1] < MP_BC_UNARY_OP_MULTI + MP_UNARY_OP_NUM_BYTECODE) {
                // printf("UNARY_OP " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_UNARY_OP_MULTI);
                instruction->opname = MP_ROM_QSTR(MP_QSTR_UNARY_OP);
                instruction->arg = MP_OBJ_NEW_SMALL_INT((mp_uint_t)ip[-1] - MP_BC_UNARY_OP_MULTI);
            } else if (ip[-1] < MP_BC_BINARY_OP_MULTI + MP_BINARY_OP_NUM_BYTECODE) {
                mp_uint_t op = ip[-1] - MP_BC_BINARY_OP_MULTI;
                // printf("BINARY_OP " UINT_FMT " %s", op, MP_OBJ_NEW_QSTR(mp_binary_op_method_name[op]));
                instruction->opname = MP_ROM_QSTR(MP_QSTR_BINARY_OP);
                instruction->arg = MP_OBJ_NEW_SMALL_INT(op);
            } else {
                // printf("code %p, byte code 0x%02x not implemented\n", ip-1, ip[-1]);
                // assert(0);
                return ip;
            }
            break;
    }

    return ip;
}

uint get_line(const byte *line_info, size_t bc) {
    const byte *ip = line_info;
    size_t source_line = 1;
    size_t c;

    while ((c = *ip)) {
        size_t b, l;
        if ((c & 0x80) == 0) {
            // 0b0LLBBBBB encoding
            b = c & 0x1f;
            l = c >> 5;
            ip += 1;
        } else {
            // 0b1LLLBBBB 0bLLLLLLLL encoding (l's LSB in second byte)
            b = c & 0xf;
            l = ((c << 4) & 0x700) | ip[1];
            ip += 2;
        }
        if (bc >= b) {
            bc -= b;
            source_line += l;
        } else {
            // found source line corresponding to bytecode offset
            break;
        }
    }

    return source_line;
}

#define WORD_SIZE			sizeof(unsigned int)
#define ALIGN(v, a)  (((((size_t) (v))-1) | ((a)-1))+1)
#define WORD_ALIGN(x)		ALIGN(x, WORD_SIZE)

void prof_extract_prelude(const byte *bytecode, mp_bytecode_prelude_t *prelude) {
    const byte *ip = bytecode;

    prelude->n_state = mp_decode_uint(&ip); // ip++
    prelude->n_exc_stack = mp_decode_uint(&ip); // ip++
    prelude->scope_flags = *ip++;
    prelude->n_pos_args = *ip++;
    prelude->n_kwonly_args = *ip++;
    prelude->n_def_pos_args = *ip++;

    prelude->code_info = ip;
    size_t code_info_size = mp_decode_uint(&ip); // ip++

    #if MICROPY_PERSISTENT_CODE
    qstr block_name = ip[0] | (ip[1] << 8);
    qstr source_file = ip[2] | (ip[3] << 8);
    ip += 4;
    #else
    qstr block_name = mp_decode_uint(&ip); // ip++
    qstr source_file = mp_decode_uint(&ip); // ip++
    #endif
    prelude->qstr_block_name = block_name;
    prelude->qstr_source_file = source_file;

    prelude->line_info = ip;

    prelude->locals = (void*)(
        (size_t)(prelude->code_info + code_info_size)
         & (~(sizeof(uint)-1)) // word align
    );

    ip = prelude->locals;
    while (*ip++ != 255);
    prelude->bytecode = ip;
}


//
// CODE
//

STATIC void code_print(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_code_t *o = MP_OBJ_TO_PTR(o_in);
    const mp_raw_code_t *rc = o->rc;
    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    mp_printf(print, "<code object %q at 0x%p, file \"%q\", line %d>", prelude->qstr_block_name, o, prelude->qstr_source_file, rc->line_def);
}

STATIC mp_obj_tuple_t* code_consts(const mp_raw_code_t *rc) {

    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    int start = prelude->n_pos_args + prelude->n_kwonly_args;
    int stop  = start + rc->data.u_byte.n_obj + rc->data.u_byte.n_raw_code;
    mp_obj_tuple_t *consts = mp_obj_new_tuple(stop - start, NULL);
    size_t const_no = 0;
    int i = 0;

    start = 0;
    stop  = prelude->n_pos_args + prelude->n_kwonly_args;
    for (i = start; i < stop; i++ ) {
        consts->items[const_no++] = (mp_obj_t)rc->data.u_byte.const_table[i];
    }

    start = prelude->n_pos_args + prelude->n_kwonly_args;
    stop  = prelude->n_pos_args + prelude->n_kwonly_args + rc->data.u_byte.n_obj;
    for (i = start; i < stop; i++ ) {
        consts->items[const_no++] = (mp_obj_t)rc->data.u_byte.const_table[i];
    }

    start = prelude->n_pos_args + prelude->n_kwonly_args + rc->data.u_byte.n_obj;
    stop  = prelude->n_pos_args + prelude->n_kwonly_args + rc->data.u_byte.n_obj + rc->data.u_byte.n_raw_code;
    for (i = start; i < stop; i++ ) {
        consts->items[const_no++] = mp_obj_new_code((const mp_raw_code_t*)rc->data.u_byte.const_table[i]);
    }

    return consts;
}

STATIC mp_obj_t code_lnotab(const mp_raw_code_t *rc) {
    const uint buffer_chunk_size = 128*2; // magic
    uint buffer_size = buffer_chunk_size;
    byte *buffer = m_new(byte, buffer_size);
    uint buffer_index = 0;

    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    uint start = prelude->bytecode - prelude->locals;
    uint stop = rc->data.u_byte.bc_len - start;

    uint last_lineno = get_line(prelude->line_info, prelude->bytecode - prelude->locals);
    uint lasti = 0;
    int i;
    for (i = start; i < stop; i ++) {
        uint lineno = get_line(prelude->line_info, i);
        size_t line_diff = lineno - last_lineno;
        if (line_diff > 0) {
            uint instr_diff = (i - start) - lasti;
            // mp_printf(&mp_plat_print, "lnotab %d, %d\n", instr_diff, line_diff);
            if (buffer_index + 2 > buffer_size) {
                assert(instr_diff < 256);
                assert(line_diff < 256);
                #if MICROPY_MALLOC_USES_ALLOCATED_SIZE
                buffer = m_realloc(buffer, buffer_size, buffer_size + buffer_chunk_size);
                #else
                buffer = m_realloc(buffer, buffer_size + buffer_chunk_size);
                #endif
                buffer_size = buffer_size + buffer_chunk_size;
            }
            last_lineno = lineno;
            lasti = i - start;
            buffer[buffer_index++] = instr_diff;
            buffer[buffer_index++] = line_diff;
        }
    }

    mp_obj_t o = mp_obj_new_bytes(buffer, buffer_index);
    m_free(buffer, buffer_size);
    return o;
}

// STATIC mp_obj_t code_locals() {
//     const byte *ip = prelude->locals;
//     int n = 0;
//     while(ip < prelude->bytecode) {
//         mp_printf(&mp_plat_print, "local%d @ %p: %d\n", ip-prelude->locals, ip, *ip);
//         ip++;
//         n++;
//     }
// }

STATIC void code_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] != MP_OBJ_NULL) {
        // not load attribute
        return;
    }
    mp_obj_code_t *o = MP_OBJ_TO_PTR(self_in);
    const mp_raw_code_t *rc = o->rc;
    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    switch(attr) {
        case MP_QSTR_co_code:
            dest[0] = mp_obj_new_bytes(
                (void*)prelude->bytecode,
                rc->data.u_byte.bc_len - (prelude->bytecode - rc->data.u_byte.bytecode)
            );
            break;
        case MP_QSTR_co_consts:
            dest[0] = MP_OBJ_FROM_PTR(code_consts(rc));
            break;
        case MP_QSTR_co_filename:
            dest[0] = MP_OBJ_NEW_QSTR(prelude->qstr_source_file);
            break;
        case MP_QSTR_co_firstlineno:
            dest[0] = MP_OBJ_NEW_SMALL_INT(get_line(prelude->line_info, prelude->bytecode - prelude->locals));
            break;
        case MP_QSTR_co_name:
            dest[0] = MP_OBJ_NEW_QSTR(prelude->qstr_block_name);
            break;
        case MP_QSTR_co_names:
            dest[0] = MP_OBJ_FROM_PTR(o->dict_locals);
            break;
        case MP_QSTR_co_lnotab:
            dest[0] = mp_obj_new_bytearray(rc->data.u_byte.bc_len - (prelude->bytecode - rc->data.u_byte.bytecode), (void*)prelude->line_info);
            dest[0] = code_lnotab(rc);
            break;
    }
}

const mp_obj_type_t mp_type_code = {
    { &mp_type_type },
    .name = MP_QSTR_code,
    .print = code_print,
    .unary_op = mp_generic_unary_op,
    .attr = code_attr,
};

mp_obj_t mp_obj_new_code(const mp_raw_code_t *rc) {
    mp_obj_code_t *o = m_new_obj(mp_obj_code_t);
    o->base.type = &mp_type_code;
    o->rc = rc;
    o->dict_locals = mp_locals_get(); // this assumes wrongly! how to do this properly?
    return MP_OBJ_FROM_PTR(o);
}

//
// FRAME
//

STATIC void frame_print(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_frame_t *frame = MP_OBJ_TO_PTR(o_in);
    mp_obj_code_t *code = frame->code;
    const mp_raw_code_t *rc = code->rc;
    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    mp_printf(print, "<frame at 0x%p, file '%q', line %d, code %q>", frame, prelude->qstr_source_file, frame->lineno, prelude->qstr_block_name);
}

STATIC void frame_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] != MP_OBJ_NULL) {
        // not load attribute
        return;
    }
    mp_obj_frame_t *o = MP_OBJ_TO_PTR(self_in);;
    // mp_obj_code_t *code = o->code;
    // const mp_raw_code_t *rc = code->rc;
    // const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    switch(attr) {
        case MP_QSTR_f_back:
            dest[0] = mp_const_none;
            if (o->code_state->prev_state) {
                dest[0] = o->code_state->prev_state->frame;
            }
            break;
        case MP_QSTR_f_code:
            dest[0] = o->code;
            break;
        case MP_QSTR_f_globals:
            dest[0] = o->code_state->fun_bc->globals;
            break;
        case MP_QSTR_f_lasti:
            dest[0] = MP_OBJ_NEW_SMALL_INT(o->lasti);
            break;
        case MP_QSTR_f_lineno:
            dest[0] = MP_OBJ_NEW_SMALL_INT(o->lineno);
            break;
    }
}

const mp_obj_type_t mp_type_frame = {
    { &mp_type_type },
    .name = MP_QSTR_frame,
    .print = frame_print,
    .unary_op = mp_generic_unary_op,
    .attr = frame_attr,
};

mp_obj_t mp_obj_new_frame(const mp_code_state_t *code_state) {
    mp_obj_frame_t *o = m_new_obj(mp_obj_frame_t);
    mp_obj_code_t *code = o->code = mp_obj_new_code(code_state->fun_bc->rc);
    const mp_raw_code_t *rc = code->rc;
    const mp_bytecode_prelude_t *prelude = &rc->data.u_byte.prelude;
    o->code_state = code_state;
    o->base.type = &mp_type_frame;
    o->back = NULL;
    o->code = code;
    // code->dict_locals = code_state->fun_bc->base.type->locals_dict;
    o->lasti = code_state->ip - prelude->locals;
    o->lineno = get_line(prelude->line_info, o->lasti);
    o->trace_opcodes = false;

    size_t n_state = mp_decode_uint_value(code_state->fun_bc->bytecode);
    mp_obj_t const * fastn = &code_state->state[n_state - 1];
    const byte *ip = prelude->locals;
    while(*ip != 255) {
        mp_printf(&mp_plat_print, "fastn[-%d] %p\n", *ip, fastn[-(*ip)]);
        ip++;
    }
    // int i;
    // for (i = 1; i<n_state; i++) {
    //     // mp_printf(&mp_plat_print, "n_state[%d] %x\n", i, code_state->state[i]);
    //     mp_printf(&mp_plat_print, "fastn[%d] %p\n", i, fastn[i]);
    // }
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t prof_build_frame(mp_code_state_t *code_state) {

    return MP_OBJ_FROM_PTR(mp_obj_new_frame(code_state));

    assert(code_state != code_state->prev_state);

    const byte *ip = code_state->ip;

    const mp_bytecode_prelude_t *prelude = &code_state->fun_bc->rc->data.u_byte.prelude;


    mp_obj_t *code = mp_obj_new_code(code_state->fun_bc->rc);

    //
    // FRAME
    //
    static const qstr frame_fields[] = {
        MP_QSTR_f_back,
        // MP_QSTR_f_builtins,
        MP_QSTR_f_code,
        MP_QSTR_f_globals,
        MP_QSTR_f_lasti,
        MP_QSTR_f_lineno,
        MP_QSTR_f_locals,
        // MP_QSTR_f_trace,
        MP_QSTR_f_trace_opcodes,
    };
    mp_obj_t frame = mp_obj_new_attrtuple(
        frame_fields,
        MP_ARRAY_SIZE(frame_fields),
        NULL
    );
    mp_obj_tuple_t *frame_attr = MP_OBJ_TO_PTR(frame);

    frame_attr->items[0] = code_state->prev_state ?
            code_state->prev_state->frame : mp_const_false;
    frame_attr->items[1] = code;
    frame_attr->items[2] = mp_globals_get();
    frame_attr->items[3] = MP_OBJ_NEW_SMALL_INT(
        ip - prelude->bytecode
    );
    frame_attr->items[4] = MP_OBJ_NEW_SMALL_INT(
        get_line(prelude->line_info, ip - prelude->locals)
    );
    frame_attr->items[5] = mp_locals_get();
    frame_attr->items[6] = mp_const_true;

    return frame;
}

mp_obj_t prof_settrace(mp_obj_t callback) {
    if (mp_obj_is_callable(callback)) {
        MP_STATE_THREAD(prof_call_trace_callback) = callback;
    } else {
        MP_STATE_THREAD(prof_call_trace_callback) = mp_const_none;
    }
    return mp_const_none;
}

mp_obj_t prof_callback_invoke(mp_obj_t callback, prof_callback_args *args) {

    assert(mp_obj_is_callable(callback));
    
    MP_STATE_THREAD(prof_instr_tick_callback_is_executing) = true;

    mp_obj_t a[3] = {args->frame, args->event, args->arg};
    mp_obj_t top = mp_call_function_n_kw(callback, 3, 0, a);

    MP_STATE_THREAD(prof_instr_tick_callback_is_executing) = false;

    if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL) {
        mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
        MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
        nlr_raise(obj);
    }
    return top;
}

mp_obj_t prof_instr_tick(mp_code_state_t *code_state, bool isException) {

    // Detecet execution recursion.
    assert(!MP_STATE_THREAD(prof_instr_tick_callback_is_executing));

    // Detecet data recursion.
    assert(code_state != code_state->prev_state);

    mp_obj_t top = mp_const_none;
    mp_obj_t callback = MP_STATE_THREAD(prof_instr_tick_callback);

    prof_callback_args _args, *args=&_args;
    args->frame = code_state->frame;
    args->event = mp_const_none;
    args->arg = mp_const_none;

    // CALL event's are handled inside the vm.c

    // EXCEPTION
    if (isException) {
        args->event = MP_ROM_QSTR(MP_QSTR_exception);
        top = prof_callback_invoke(callback, args);
        return top;
    }

    // LINE
    const mp_bytecode_prelude_t *prelude = &code_state->fun_bc->rc->data.u_byte.prelude;
    
    size_t prev_line_no = args->frame->lineno;;
    size_t current_line_no = get_line(prelude->line_info, code_state->ip - prelude->locals);
    if (prev_line_no != current_line_no) {
        args->frame->lineno = current_line_no;
        args->event = MP_ROM_QSTR(MP_QSTR_line);
        top = prof_callback_invoke(callback, args);
    }
#if false // old
    const mp_bytecode_prelude_t *prelude = &code_state->fun_bc->rc->data.u_byte.prelude;
    size_t prev_line_no = MP_OBJ_SMALL_INT_VALUE(((mp_obj_tuple_t*)MP_OBJ_TO_PTR(args->frame))->items[4]);
    size_t current_line_no = get_line(prelude->line_info, code_state->ip - prelude->locals);
    if (prev_line_no != current_line_no) {
        ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(args->frame))->items[4] = MP_OBJ_NEW_SMALL_INT(current_line_no);
        args->event = MP_ROM_QSTR(MP_QSTR_line);
        top = prof_callback_invoke(callback, args);
    }
#endif

    // RETURN
    const byte *ip = code_state->ip;
    if (*ip == MP_BC_RETURN_VALUE || *ip == MP_BC_YIELD_VALUE) {
        args->event = MP_ROM_QSTR(MP_QSTR_return);
        top = prof_callback_invoke(callback, args);
        if (code_state->prev_state && *ip == MP_BC_RETURN_VALUE) {
            code_state->next_tracing_callback = mp_const_none;
        }
    }

    // OPCODE
    if (false) {
        args->event = MP_ROM_QSTR(MP_QSTR_opcode);
        top = prof_callback_invoke(callback, args);
    }

    MP_STATE_THREAD(prof_instr_tick_callback_is_executing) = false;
    return top;
}

void prof_function_def_line(mp_obj_t fun, mp_code_state_t *code_state) {
    const mp_bytecode_prelude_t *prelude = &code_state->fun_bc->rc->data.u_byte.prelude;
    mp_obj_t line_no = MP_OBJ_NEW_SMALL_INT(get_line(prelude->line_info, code_state->ip - prelude->locals));
    if (mp_obj_get_type(fun) == &mp_type_fun_bc) {
        mp_obj_fun_bc_t *self_fun = (mp_obj_fun_bc_t*)fun;
        self_fun->line_def = line_no;
    } else if (mp_obj_get_type(fun) == &mp_type_gen_wrap) {
        mp_obj_gen_wrap_t *self_gen = (mp_obj_gen_wrap_t *)fun;
        mp_obj_fun_bc_t *self_fun = (mp_obj_fun_bc_t*)self_gen->fun;
        assert(self_fun->base.type == &mp_type_fun_bc);
        self_fun->line_def = line_no;
    } else {
        mp_printf(&mp_plat_print, "unmakeable %s %p\n", mp_obj_get_type_str(fun), mp_obj_get_type(fun));
        assert(!"dunno how to handle this");
    }
}

#endif // MICROPY_PY_SYS_TRACE

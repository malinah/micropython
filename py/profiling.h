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

#ifndef MICROPY_INCLUDED_PY_PROFILING_H
#define MICROPY_INCLUDED_PY_PROFILING_H

#include "py/objtype.h"
#include "py/objgenerator.h"
#include "py/objfun.h"
#include "py/bc.h"

#if MICROPY_PY_SYS_TRACE

typedef struct _mp_obj_code_t {
    mp_obj_base_t base;
    const mp_raw_code_t *rc;
    mp_obj_dict_t *dict_locals;
} mp_obj_code_t;

typedef struct _mp_obj_frame_t {
    mp_obj_base_t base;
    const mp_code_state_t *code_state;
    struct _mp_obj_frame_t *back;
    mp_obj_code_t *code;
    mp_uint_t lasti;
    mp_uint_t lineno;
    bool trace_opcodes;
} mp_obj_frame_t;

typedef struct __attribute__((__packed__)) {
    struct _mp_obj_frame_t * frame;
    mp_obj_t event;
    mp_obj_t arg;
} prof_callback_args;

typedef struct _mp_dis_instruction {
    mp_obj_t opcode;
    mp_obj_t opname;
    mp_obj_t arg;
    mp_obj_t argval;
    mp_obj_t cache;
} mp_dis_instruction;

typedef struct _mp_obj_closure_t {
    mp_obj_base_t base;
    mp_obj_t fun;
    size_t n_closed;
    mp_obj_t closed[];
} mp_obj_closure_t;

typedef struct _prof_line_stats {
    size_t line_exec;
    size_t instr_no;
    size_t instr_exec;
} prof_line_stats;

mp_obj_t mp_obj_new_code(const mp_raw_code_t *rc);
mp_obj_t mp_obj_new_frame(const mp_code_state_t *code_state);

void prof_extract_prelude(const byte *bytecode, mp_bytecode_prelude_t *prelude);
uint get_line(const byte *line_info, size_t bc);

const byte *prof_opcode_decode(const byte *ip, const mp_uint_t *const_table, mp_dis_instruction *instruction);


// For each instruction in VM execute this function.
mp_obj_t prof_instr_tick(mp_code_state_t *code_state, bool isException);

mp_obj_t prof_build_frame(mp_code_state_t *code_state);

mp_obj_t prof_settrace(mp_obj_t callback);


mp_obj_t prof_callback_invoke(mp_obj_t callback, prof_callback_args *args);

#endif

#endif // MICROPY_INCLUDED_PY_PROFILING_H

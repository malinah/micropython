# if not '__profiling__' in globals():
#     raise Exception('Micropython Profiling feature is required!')

import sys

import dis

def format_instruction(instr):
    return "%s %s %s"%(
        instr[2],
        '#{0!r}'.format(instr[3]) if instr[3] != None else "",
        '${0!r}'.format(instr[4]) if instr[4] != None else ""
        # '@{0!r}'.format(instr[5]) if instr[5] != None else ""
    )

def print_stacktrace(frame, level = 0):
    back = frame.f_back

    f_lineno = -3
    # if hasattr(frame, 'f_lineno'):
    f_lineno = frame.f_lineno


    # print("*%2d: %s0x%04x@%s:%s => %s:%d"%(
    print("*%2d: %s@%s:%s => %s:%d"%(
        level,
        "  " * level,

        # frame.f_lasti,
        frame.f_globals['__name__'],
        frame.f_code.co_name,

        frame.f_code.co_filename,
        f_lineno,
    ))

    if (back):
        print_stacktrace(back, level + 1)

def print_bytecode(name, bytecode):
    for instr in bytecode:
        print("^0x%04x@%s\t%s"%(
            instr[0],
            name,
            format_instruction(instr)
        ))

def lnotab(pairs, first_lineno=0):
    """Yields byte integers representing the pairs of integers passed in."""
    assert first_lineno <= pairs[0][1]
    cur_byte, cur_line = 0, first_lineno
    for byte_off, line_off in pairs:
        byte_delta = byte_off - cur_byte
        line_delta = line_off - cur_line
        assert byte_delta >= 0
        assert line_delta >= 0
        while byte_delta > 255:
            yield 255 # byte
            yield 0   # line
            byte_delta -= 255
        yield byte_delta
        while line_delta > 255:
            yield 255 # line
            yield 0   # byte
            line_delta -= 255
        yield line_delta
        cur_byte, cur_line = byte_off, line_off

def lnotab_string(pairs, first_lineno=0):
    return "".join(chr(b) for b in lnotab(pairs, first_lineno))

def byte_pairs(lnotab):
    """Yield pairs of integers from a string."""
    for i in range(0, len(lnotab), 2):
        yield ord(lnotab[i]), ord(lnotab[i+1])
        
def lnotab_numbers(lnotab, first_lineno=0):
    """Yields the byte, line offset pairs from a packed lnotab string."""

    last_line = None
    cur_byte, cur_line = 0, first_lineno
    for byte_delta, line_delta in byte_pairs(lnotab):
        if byte_delta:
            if cur_line != last_line:
                yield cur_byte, cur_line
                last_line = cur_line
            cur_byte += byte_delta
        cur_line += line_delta
    if cur_line != last_line:        
        yield cur_byte, cur_line


class _Prof:
    display_flags = 0
    DISPLAY_INSTRUCITON = 1<<0
    DISPLAY_STACKTRACE = 1<<1

    instr_count = 0
    bytecodes = {}

    def instr_tick(self, frame, event, arg):
        if event == 'exception':
            print('!! exception at ',end='')
        else:
            self.instr_count += 1

        if event == 'call':
            if frame.f_code.co_name == '<module>':
                print(frame)
                print(" [[ %s ]]"%(frame.f_code.co_name))
                print(frame.f_code.co_names)
                print(locals())
                # print(frame.f_code.co_code)
                # print(dir(dis))
                # print(dis.disassemble(frame.f_code))
                # print(str(frame.f_code.co_lnotab))
                # print(frame.f_code.co_consts)
                # print(frame.f_code.co_firstlineno)
                # print(list(lnotab_numbers( frame.f_code.co_lnotab.decode('utf-8') )))
                # print(list(lnotab_numbers('\x08\x01\x08\x01')))




        # print(frame)
        # print(frame.f_code.co_code)

        instr = None
        if hasattr(frame, 'f_disasm'):
            instr = frame.f_disasm
        # print(instr)
        #print("%-6s"%(self.instr_count), format_instruction(instr) if instr else  "")
        print(format_instruction(instr) if instr else  "")

        modulename = None
        if hasattr(frame, 'f_modulename'):
            modulename = frame.f_modulename

        if self.display_flags & _Prof.DISPLAY_STACKTRACE:
            # print(frame)
            print_stacktrace(frame)


global __prof__
if not '__prof__' in globals():
    __prof__ = _Prof()
__prof__.display_flags |= _Prof.DISPLAY_INSTRUCITON
__prof__.display_flags |= _Prof.DISPLAY_STACKTRACE



def instr_tick_handler_what(frame, event, arg):
    print("###xxxx ", event, end=' ')
    __prof__.instr_tick(frame, event, arg)

    return None

whated = 0

def instr_tick_handler_wack(frame, event, arg):
    print("###wack ", event, end=' ')
    __prof__.instr_tick(frame, event, arg)
    return None

wacked = 0

def instr_tick_handler(frame, event, arg):
    if frame.f_globals['__name__'].startswith('importlib.'):
        return
    
    print("###nati ", event, end=' ')
    __prof__.instr_tick(frame, event, arg)
    # sys.settrace(instr_tick_handler)
    # if frame.f_trace_opcodes == False:
    #     frame.f_trace_opcodes = True

    global wacked
    if event == 'call' and wacked == 0:
        wacked = 1
        return instr_tick_handler_wack

    global whated
    if event == 'call' and whated == 0:
        whated = 1
        return instr_tick_handler_what

    # return None
    return instr_tick_handler

def atexit():
    print("\n------------------ script exited ------------------")
    print("Total instructions executed: ", __prof__.instr_count)
    return
    print("\nSections in module __main__:")
    for section in list(__prof__.bytecodes['__main__']):
        print(section)
    print("\nModule __main__ bytecode dump of do():",)
    print_bytecode('do', __prof__.bytecodes['__main__']['/<module>/do'])

def factorial(n):
    if n == 0:
        # Display the bubbling stacktrace from this nested call.
        # __prof__.display_flags |= _Prof.DISPLAY_STACKTRACE
        return 1
    else:
        return n * factorial(n-1)

def factorials_up_to(x, b):
    a = 1
    for i in range(1, x+1):
        a = a * i
        print("inbetween bin_op")
        a = a + a
        yield a + b


exc = Exception('ExceptionallyLoud')
zero = 0

def do():
    # These commands are here to demonstrate some execution being traced.
    print("Who loves the sun?")

    from test_print import xxx
    xxx()
    # import test_print
    # return

    # what_happened_async()


    # what_happened_async()
    # what_happened_async()
    # what_happened_async()

    try:
        raise exc
    except:
        pass

    # try:
    #     try:
    #         raise exc
    #     finally:
    #         print(exc)
    # except:
    #     pass
    
    # def wack():
    #     # 1/zero
    #     # print("Yo, this is wack!")
    #     factorial(3)

    # wack() 

    # factorial(2)

    # # print("WAKWAKWA!")

    # return
    # TODO FIXME This causes inconsistency in the codepath
    # because both array generators have the same name.

    # [x**3
    #         for
    #             x
    #                 in
    #                     range(10)
    #                     ]

    # print("array_gen2 =>", [x**2 for x in range(10)] )
    # print("array_gen3 =>", [x**3 for x in range(10)] )
    
    # factorials_up_to(3, 0)

    # print(
    #     "factorials_up_to =>",
    #         list(
    #             factorials_up_to(3)))

    def factorials_up_to_yield_wrap(n):
        r = []
        for f in factorials_up_to(n, 0):
            r.append(f)
        
        print("factorials_up_to_yield_wrap =>", r)
    
    factorials_up_to_yield_wrap(3)

    print("factorial =>", factorial(4))
    # __prof__.display_flags &= ~_Prof.DISPLAY_STACKTRACE
    
    print("asd")

    def make_multiplier_of(n):
        def multiplier(x):
            return x * n
        return multiplier

    # Multiplier of 3
    times3 = make_multiplier_of(3)

    # Multiplier of 5
    times5 = make_multiplier_of(5)

    # Output: 27
    print(times3(9))

    # Output: 15
    print(times5(3))

    # Output: 30
    print(times5(times3(2)))

    func_obj_1 = lambda a, b: a + b
    print(func_obj_1(10, 20))

    func_obj_2 = lambda a, b: a * b + c
    c = 5
    print(func_obj_2(30, 20))

    # import ubinascii
    # # exec("def xoxo():\n\tprint(999)\n\nxoxo()")
    # # print(ubinascii.b2a_base64("def xoxo():\n\tprint(999)\n\nxoxo()"))
    # exec(ubinascii.a2b_base64("ZGVmIHhveG8oKToKCXByaW50KDk5OSkKCnhveG8oKQ=="))

    exec("print(3+4)\nprint(2+1)\n1+1")
    print('>3')

    eval("print(4+1)")

    print('>2')
    print('>1')

# Register atexit handler that will be executed before sys.exit().
# sys.uatexit(atexit)

# Register the tracing callback.
sys.settrace(instr_tick_handler)


do()

def gen():
    def example_gen():
        # print("after first send(None)")
        x = 5
        yield 1
        x = x + 1
        yield 2 + x
        return 3

    g = example_gen()
    try:
        r = g.send(None)
        # print("r", r)
        while True:
            r = g.send(None)
            # print("r", r)
    except StopIteration as e:
        # print(e)
        pass

wacked = False
whated = False
gen()

#     # print("return", e)
#     # import math

# Trigger the atexit handler.
# sys.exit()
# raise Exception('done')

sys.settrace(None)

print('nothing to trace here')
print('nothing to trace here')
print('nothing to trace here')
print('nothing to trace here')

atexit()

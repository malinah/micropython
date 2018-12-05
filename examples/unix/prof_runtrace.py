# if not '__profiling__' in globals():
#     raise Exception('Micropython Profiling feature is required!')

import sys

def format_instruction(instr):
    return "%s %s %s"%(
        instr[2],
        '#{0!r}'.format(instr[3]) if instr[3] != None else "",
        '${0!r}'.format(instr[4]) if instr[4] != None else ""
        # '@{0!r}'.format(instr[5]) if instr[5] != None else ""
    )

def print_stacktrace(frame, level = 0):
    back = frame.f_back

    print("*%2d: %s0x%04x@%s:%s => %s:%d"%(
        level,
        "  " * level,

        frame.f_instroffset,
        frame.f_modulename,
        frame.f_code.co_name,

        frame.f_code.co_filename,
        frame.f_lineno,
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

        instr = frame.f_disasm
        # print(instr)
        print("%-6s"%(self.instr_count), format_instruction(instr))

        # modulename = frame.f_modulename
        # if not modulename in self.bytecodes:
        #     self.bytecodes[modulename] = sys.bytecode(modulename)

        # # print(dir(frame.f_code))
        # if frame.f_code.co_codepath in self.bytecodes[modulename].keys():
        #     bytecode = self.bytecodes[modulename][frame.f_code.co_codepath]

        #     if self.display_flags & _Prof.DISPLAY_INSTRUCITON:
        #         for instr in bytecode:
        #             # print(instr)
        #             if instr[0] == frame.f_instroffset:
        #                 print(
        #                     ("{:<50}".format("# %2d: 0x%04x@%s:%s "%(
        #                         self.instr_count,
        #                         frame.f_instroffset,
        #                         frame.f_modulename,
        #                         frame.f_code.co_codepath,
        #                     )))+
        #                     format_instruction(instr)
        #                 )
        #                 # print(instr[2], instr[3], instr[4] if instr[4] else "")
        #                 break

        if self.display_flags & _Prof.DISPLAY_STACKTRACE:
            print_stacktrace(frame)
            # print(frame)


global __prof__
if not '__prof__' in globals():
    __prof__ = _Prof()
__prof__.display_flags |= _Prof.DISPLAY_INSTRUCITON
__prof__.display_flags |= _Prof.DISPLAY_STACKTRACE

def instr_tick_handler(frame, event, arg):
    __prof__.instr_tick(frame, event, arg)
    if event == 'call':
        sys.settrace(instr_tick_handler)
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

def factorials_up_to(x):
    a = 1
    for i in range(1, x+1):
        a *= i
        yield a

def do():
    # These commands are here to demonstrate some execution being traced.
    # print("Who loves the sun?")

    def wack():
        print("Yo, this is wack!")
        print("Yo, this is wack!")
        print("Yo, this is wack!")

    wack()

    print("WAKWAKWA!")

    try:
        raise Exception('ExceptionallyLoud')
    except:
        pass

    # TODO FIXME This causes inconsistency in the codepath
    # because both array generators have the same name.
    print("array_gen2 =>", [x**2 for x in range(10)] )
    print("array_gen3 =>", [x**3 for x in range(10)] )
    
    print("factorials_up_to =>", list(factorials_up_to(3)))

    def factorials_up_to_yield_wrap(n):
        r = []
        for f in factorials_up_to(n):
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

    import ubinascii
    # exec("def xoxo():\n\tprint(999)\n\nxoxo()")
    # print(ubinascii.b2a_base64("def xoxo():\n\tprint(999)\n\nxoxo()"))
    exec(ubinascii.a2b_base64("ZGVmIHhveG8oKToKCXByaW50KDk5OSkKCnhveG8oKQ=="))

    eval("print(2+1)")
    print('>3')

    eval("print(4+1)")

    print('>2')
    print('>1')

# Register atexit handler that will be executed before sys.exit().
# sys.atexit(atexit)

# Don't start tracing yet.
# sys.prof_mode(0)

# Register the tracing callback.
sys.settrace(instr_tick_handler)

# Start the tracing now.
# sys.prof_mode(1)

do()
# import math

# Stop the tracing now.
# sys.prof_mode(0)

# Trigger the atexit handler.
# sys.exit()
# raise Exception('done')

print('nothing to trace here')
print('nothing to trace here')
print('nothing to trace here')
print('nothing to trace here')


sys.settrace(None)
atexit()

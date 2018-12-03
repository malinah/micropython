import sys
import opcode

def test(a):
    return a * 7 + 2



def tracefunc(frame, event, arg):
    if event == 'call':
        frame.f_trace_opcodes = True
        frame.f_trace_lines = False
        return tracefunc
    if event == 'opcode':
        _opcode, _oparg = frame.f_code.co_code[frame.f_lasti], frame.f_code.co_code[frame.f_lasti+1]
        print(opcode.opname[_opcode])
        print(frame.f_back)

sys.settrace(tracefunc)

test(55)

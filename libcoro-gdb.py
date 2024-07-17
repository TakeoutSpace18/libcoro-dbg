"""
Usage from gdb:
    source <this script path>/libcoro-gdb.py
    libcoro-bt
"""
import gdb

def get_state_table():
    # TODO: now this only works when libcoro is compiled with -O0 -g
    st = gdb.lookup_symbol("__coro_state_table")
    return st[0].value().cast(gdb.lookup_type("ste_t").pointer())

def get_coro_states():
    states = []
    state_table_ptr = get_state_table()
    while state_table_ptr["sp"] != 0:
        states.append({
            "rsp": state_table_ptr["sp"],
            "rbp": state_table_ptr["fp"],
            "rip": state_table_ptr["pc"]
             })
        state_table_ptr += 1

    return states;


regs_to_save = ["rsp", "rbp", "rip"]

class CoroutineBt(gdb.Command):
    '''Display backtrace including coroutine switches'''
    def __init__(self):
        gdb.Command.__init__(self, 'libcoro-bt', gdb.COMMAND_STACK,
                             gdb.COMPLETE_NONE)

    def invoke(self, arg, from_tty):
        coro_states = get_coro_states()

        print("Current backtrace:")
        gdb.execute("bt")

        old = {}
        for reg in regs_to_save:
            old[reg] = gdb.parse_and_eval("(uint64_t)$%s" % reg) 

        for state in coro_states:
            for reg, val in state.items():
                gdb.execute('set $%s = %s' % (reg, val))

            print("\nCoroutine (stack pointer: %x)" % state["rsp"])
            gdb.execute("bt")

        for reg, val in old.items():
            gdb.execute("set $%s = %s" % (reg, val))



CoroutineBt()

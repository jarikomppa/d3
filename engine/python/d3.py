import sys
import json
import random

# needed to enable console codes in windows:
import os
import ctypes


class d3:
    debugmode = False
    current_card = 0
    current_section = 0
    current_paragraph = 0
    gosub_stack = []
    returnfrom = False
    text = ""
    answers = []
    answerno = []
    d = {"cards": []}
    state = {}
    numstate = {}

    def print_state(self):
        live = []
        for k in self.state:
            if self.state[k] == 1:
                live.append(self.d["syms"][int(k)])
        for k in self.numstate:
            live.append(self.d["nsyms"][int(k)] + "=" + str(self.numstate[k]))
        print("state:", live)

    def choose(self, option):
        o = self.answerno[int(option)]
        for p in self.d["cards"][self.current_card][o]["p"]:
            if self.check_predicates(p["code"]):
                self.run_code(p["code"], True)

        self.current_card = int(self.d["cards"][self.current_card][o]["id"])
        self.parse_card()

    def touch_number(self, s):
        if s not in self.numstate:
            if self.debugmode:
                print("new number", s)
            self.numstate[s] = 0

    def touch_symbol(self, s):
        if s not in self.state:
            if self.debugmode:
                print("new symbol", s)
            self.state[s] = 0

    def check_predicates(self, code):
        def op_nop(a1, a2):
            return True

        def op_has(a1, a2):
            self.touch_symbol(a1)
            return self.state[a1] == 1

        def op_not(a1, a2):
            self.touch_symbol(a1)
            return self.state[a1] != 1

        def op_gt(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] > self.numstate[a2]

        def op_gtc(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] > int(a2)

        def op_lt(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] < self.numstate[a2]

        def op_ltc(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] < int(a2)

        def op_gte(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] >= self.numstate[a2]

        def op_gtec(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] >= int(a2)

        def op_lte(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] <= self.numstate[a2]

        def op_ltec(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] <= int(a2)

        def op_eq(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] == self.numstate[a2]

        def op_eqc(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] == int(a2)

        def op_ieq(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            return self.numstate[a1] != self.numstate[a2]

        def op_ieqc(a1, a2):
            self.touch_number(a1)
            return self.numstate[a1] != int(a2)

        def op_rnd(a1, a2):
            return random.randrange(100) < int(a1)

        op_func = {
            "0": op_nop,
            "1": op_has,  # OP_HAS
            "2": op_not,  # OP_NOT
            "3": op_nop,
            "4": op_nop,
            "5": op_nop,
            "6": op_rnd,  # OP_RND
            "7": op_nop,
            "8": op_nop,
            "9": op_nop,
            "10": op_gt,    # OP_GT
            "11": op_gtc,   # OP_GTC
            "12": op_lt,    # OP_LT
            "13": op_ltc,   # OP_LTC
            "14": op_gte,   # OP_GTE
            "15": op_gtec,  # OP_GTEC
            "16": op_lte,   # OP_LTE
            "17": op_ltec,  # OP_LTEC
            "18": op_eq,    # OP_EQ
            "19": op_eqc,   # OP_EQC
            "20": op_ieq,   # OP_IEQ
            "21": op_ieqc,  # OP_IEQC
            "22": op_nop,
            "23": op_nop,
            "24": op_nop,
            "25": op_nop,
            "26": op_nop,
            "27": op_nop,
            "28": op_nop,
            "29": op_nop,
            "30": op_nop,
            "31": op_nop,
            "32": op_nop
        }
        ret = True
        for x in code:
            op = x["op"]
            a1 = x["a1"]
            a2 = x["a2"]
            if not op_func[op](a1, a2):
                ret = False
        return ret

    def run_code(self, code, execute):
        ret = ""

        def op_nop(a1, a2):
            return ""

        def op_set(a1, a2):
            self.touch_symbol(a1)
            self.state[a1] = 1
            return ""

        def op_clr(a1, a2):
            self.touch_symbol(a1)
            self.state[a1] = 0
            return ""

        def op_xor(a1, a2):
            self.touch_symbol(a1)
            if self.state[a1] == 1:
                self.state[a1] = 0
            else:
                self.state[a1] = 1
            return ""

        def op_assign(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            self.numstate[a1] = self.numstate[a2]
            return ""

        def op_assignc(a1, a2):
            self.touch_number(a1)
            self.numstate[a1] = int(a2)
            return ""

        def op_add(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            self.numstate[a1] += self.numstate[a2]
            return ""

        def op_addc(a1, a2):
            self.touch_number(a1)
            self.numstate[a1] += int(a2)
            return ""

        def op_sub(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            self.numstate[a1] -= self.numstate[a2]
            return ""

        def op_subc(a1, a2):
            self.touch_number(a1)
            self.numstate[a1] -= int(a2)
            return ""

        def op_mul(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            self.numstate[a1] *= self.numstate[a2]
            return ""

        def op_mulc(a1, a2):
            self.touch_number(a1)
            self.numstate[a1] *= int(a2)
            return ""

        def op_div(a1, a2):
            self.touch_number(a1)
            self.touch_number(a2)
            if self.numstate[a2] != 0:
                self.numstate[a1] = int(self.numstate[a1] / self.numstate[a2])
            return ""

        def op_divc(a1, a2):
            self.touch_number(a1)
            if int(a2) != 0:
                self.numstate[a1] = int(self.numstate[a1] / int(a2))
            return ""

        def op_print(a1, a2):
            self.touch_number(a1)
            return str(self.numstate[a1])

        def op_goto(a1, a2):
            self.current_card = int(a1)
            self.current_paragraph = -1
            self.current_section = -1
            return ""

        def op_goto(a1, a2):
            self.current_card = int(a1)
            self.current_paragraph = -1
            self.current_section = -1
            return ""

        def op_gosub(a1, a2):
            self.gosub_stack.append(self.current_card)
            self.gosub_stack.append(self.current_section)
            self.gosub_stack.append(self.current_paragraph)
            self.current_card = int(a1)
            self.current_paragraph = -1
            self.current_section = -1
            return ""

        op_func = {
            "0": op_nop,
            "1": op_nop,
            "2": op_nop,
            "3": op_set,  # OP_SET
            "4": op_clr,  # OP_CLR
            "5": op_xor,  # OP_XOR
            "6": op_nop,
            "7": op_goto,   # OP_GO
            "8": op_gosub,  # OP_GOSUB
            "9": op_print,  # OP_PRINT
            "10": op_nop,
            "11": op_nop,
            "12": op_nop,
            "13": op_nop,
            "14": op_nop,
            "15": op_nop,
            "16": op_nop,
            "17": op_nop,
            "18": op_nop,
            "19": op_nop,
            "20": op_nop,
            "21": op_nop,
            "22": op_assign,   # OP_ASSIGN
            "23": op_assignc,  # OP_ASSIGNC
            "24": op_add,      # OP_ADD
            "25": op_addc,     # OP_ADDC
            "26": op_sub,      # OP_SUB
            "27": op_subc,     # OP_SUBC
            "28": op_mul,      # OP_MUL
            "29": op_mulc,     # OP_MULC
            "30": op_div,      # OP_DIV
            "31": op_divc,     # OP_DIVC
            "32": op_nop
        }
        ret = ""
        for x in code:
            op = x["op"]
            a1 = x["a1"]
            a2 = x["a2"]
            # execute "print" even if execute is false
            if execute is True or op == "9":
                ret = ret + op_func[op](a1, a2)
        return ret

    def parse_card(self):
        no = 1
        self.text = ""
        self.answers = []
        self.answerno = []
        self.current_section = 0
        while self.current_section < len(self.d["cards"][self.current_card]):
            section = self.d["cards"][self.current_card][self.current_section]
            if section["type"] == "Q" and not self.returnfrom:
                self.touch_symbol(section["id"])
                self.state[section["id"]] = 1

            if (section["type"] == "Q") or len(self.gosub_stack) == 0:
                tx = ""
                if not self.returnfrom:
                    self.current_paragraph = 0
                self.returnfrom = False
                while self.current_paragraph < len(self.d["cards"][self.current_card][self.current_section]["p"]):
                    x = self.d["cards"][self.current_card][self.current_section]["p"][self.current_paragraph]
                    if self.check_predicates(x["code"]):
                        # OP_PRINT produces text
                        tx = tx + self.run_code(x["code"], section["type"] == "Q")
                        tx = tx + x["t"]
                    self.current_paragraph += 1
                    # goto and gosub set this to -1, so when incremented we get 0..
                    if self.current_paragraph == 0:
                        break
                ##
                if section["type"] == "Q":
                    self.text += tx
                else: # type = A
                    if tx != "":
                        self.answers.append(tx)
                        self.answerno.append(no)
                    no += 1

            self.current_section += 1
            if self.current_section == len(self.d["cards"][self.current_card]) and len(self.gosub_stack) > 0:
                self.current_paragraph = self.gosub_stack.pop()
                self.current_section = self.gosub_stack.pop()
                self.current_card = self.gosub_stack.pop()
                self.current_paragraph += 1
                self.returnfrom = True

    def load_json(self, fn):
        with open(fn, "r") as f:
            self.d = json.load(f)
        self.parse_card()

    def dump(self):
        print(self.d)


if __name__ == '__main__':
    # enable console control codes on windows
    if os.name == 'nt':
        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)

    print("d3 python interpreter test")

    if len(sys.argv) < 2:
        print("Give me a .json file to munch on")
        quit()

    print("Loading", sys.argv[1])
    d = d3()
    d.load_json(sys.argv[1])
    print("Running..")
    showstate = False
    while True:
        if d.debugmode is False:
            # clear screen, move cursor to top
            print("\033[1J\033[1;1H", end='')
        else:
            print(d.answers)
            print(d.answerno)
        if showstate:
            d.print_state()
            print("-   -  - ----- -  -   -")
        print(d.text)
        print("-   -  - ----- -  -   -")
        for i in range(len(d.answers)):
            print(i, ")", d.answers[i])
        print("s ) show state")
        print("d ) debug mode")
        print("q ) quit")
        ok = False
        while ok is False:
            i = input(">")
            if i == "q":
                quit()
            if i == "s":
                showstate = showstate is not True
                ok = True
            if i == "d":
                d.debugmode = d.debugmode is not True
                ok = True
            else:
                valid = False
                intv = 0
                try:
                    intv = int(i)
                    valid = True
                except ValueError:
                    print("Invalid option")
                if valid:
                    if intv < 0 or intv > len(d.answers) - 1:
                        print("Choice out of range,", intv, "not within 0 ..", len(d.answers) - 1)
                    else:
                        d.choose(i)
                        ok = True

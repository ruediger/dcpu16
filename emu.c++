#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <limits>

typedef std::uint16_t word;
typedef std::uint32_t nword;

enum Regs {
  A = 0x0,
  B = 0x1,
  C = 0x2,
  X = 0x3,
  Y = 0x4,
  Z = 0x5,
  I = 0x6,
  J = 0x7
};

enum Ops {
  NBI = 0x0, // non-basic instruction
  SET = 0x1,
  ADD = 0x2,
  SUB = 0x3,
  MUL = 0x4,
  DIV = 0x5,
  MOD = 0x6,
  SHL = 0x7,
  SHR = 0x8,
  AND = 0x9,
  BOR = 0xa,
  XOR = 0xb,
  IFE = 0xc,
  IFN = 0xd,
  IFG = 0xe,
  IFB = 0xf
};

enum NbiOps {
  JSR = 0x01
};

enum Verbose {
  Silent = 0,
  Warn = 1,
  Debug = 2,
  Verbose = 3
} verbosity = Verbose;
#define INF(x, l) do { if(verbosity >= (l)) { std::clog << x << std::endl; } } while(0)
#define DBG(x) INF(x, Debug)
#define WRN(x) INF(x, Warn)
#define TLK(x) INF(x, Verbose)

std::vector<word> regs;
std::vector<word> mem;
word pc; // program counter
word sp; // stack pointer
word o;  // overflow

word nil = 0;

void dump_regs() {
  if(verbosity >= Debug) {
    std::clog << "Regs | ";
    for(auto const &w : regs) {
      std::clog << std::hex << std::setfill('0') << std::setw(4) << w << " | ";
    }
    std::clog << "PC: " << std::hex << std::setfill('0') << std::setw(4) << pc << " | SP: "  << sp << " | O: " << o << " |" << std::endl;
  }
}

void dump_mem() {
  if(verbosity >= Debug) {
    std::clog << "mem:";
    unsigned n = 0;
    for(auto w : mem) {
      if(n++ % 8 == 0) {
        std::clog << "\n" << std::hex << std::setfill('0') << std::setw(4) << n << ": ";
      }
      else {
        std::clog << " ";
      }
      std::clog << std::hex << std::setfill('0') << std::setw(4) << w;
      if(n >= 104) break; // DBG
    }
    std::clog << std::endl;
  }
}

word next_word() {
  if(pc >= mem.size()) {
    throw std::runtime_error("EOP"); // TODO
  }
  // DEBUG!!!!!!
  if(pc > 100) {
    DBG("XXX");
    throw "end";
  }
  return mem[pc++];
}

word &decode_manip_value(std::uint8_t v, bool force = true) {
  if(v <= 0x07) { // Regs
    return regs[v];
  }
  else if(0x08 <= v and v <= 0x0f) { // [Reg]
    return mem[regs[v-0x08]];
  }
  else if(0x10 <= v and v <= 0x17) { // [Next Word + Reg]
    return mem[next_word()+regs[v-0x10]];
  }
  else if(v == 0x18) { // pop
    return mem[sp++];
  }
  else if(v == 0x19) { // peek
    return mem[sp];
  }
  else if(v == 0x1a) { // push
    return mem[--sp];
  }
  else if(v == 0x1b) { // sp
    return sp;
  }
  else if(v == 0x1c) { // pc
    return pc;
  }
  else if(v == 0x1d) { // o
    return o;
  }
  else if(v == 0x1e) { // [next_word]
    return mem[next_word()];
  }
  else if(force) {
    throw std::runtime_error("wrong value"); // TODO
  }
  return nil;
}

word decode_value(std::uint8_t v) {
  if(v < 0x1f) {
    return decode_manip_value(v, false);
  }
  else if(v == 0x1f) { // next_word
    return next_word();
  }
  else if(0x20 <= v and v <= 0x3f) { // lit
    return v - 0x20;
  }
  throw std::runtime_error("unknown value"); // TODO
  return nil;
}

void load_mem() {
  std::cin.sync_with_stdio(false);
  std::cin.read(reinterpret_cast<char*>(&mem[0]), mem.size());
  dump_mem();
  DBG("memory loaded");
}

int main() try {
  regs.resize(8);
  std::size_t const n = 0x1000; // TODO
  mem.resize(n);
  pc = 0;
  sp = n;
  o = 0;

  load_mem();

  bool perform = true;

  for(;;) {
    word const w = next_word();
    if(perform) {
      std::uint8_t const opcode = w & 0xf;
      std::uint8_t const a = (w & 0x3f0) >> 4;
      std::uint8_t const b = (w & 0xfc00) >> 10;
      TLK(std::hex << w << ": " << unsigned(opcode) << " " << unsigned(a) << " " << unsigned(b));
      switch(opcode) {
      case NBI: {
        // a -> opcode and b -> a
        if(a == 0) { // reserved but used for exit here
          DBG("HALT");
          dump_mem();
          dump_regs();
          return 0;
        }
        else if(a == JSR) {
          mem[--sp] = pc;
          pc = decode_value(b);
        }
        else {
          throw std::runtime_error("Unknown NBI Op"); // TODO
        }
        
      }
      case SET:
        decode_manip_value(a) = decode_value(b);
        break;
      case ADD: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        if(nword(a_) + b_ > std::numeric_limits<word>::max()) {
          o = 0x0001;
        }
        else {
          o = 0x0;
        }
        a_ += b_;
      }
        break;
      case SUB: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        o = (a_ < b_) ? 0xffff : 0x0; // underflow
        a_ -= b_;
      }
        break;
      case MUL: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        o = ((a_*b_)>>16)&0xffff;
        a_ *= b_;
      }
        break;
      case DIV: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        if(b_ == 0) {
          a_ = o = 0x0;
        }
        else {
          o = ((a_<<16)/b_)&0xffff;
          a_ /= b_;
        }
      }
        break;
      case MOD: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        if(b_ == 0) {
          a_ = 0x0;
        }
        else {
          a_ %= b_;
        }
      }
        break;
      case SHL: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        o = ((a_<<b_)>>16)&0xffff;
        a_ <<= b_;
      }
        break;
      case SHR: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        o = ((a_<<16)>>b_)&0xffff;
        a_ >>= b_;
      }
        break;
      case AND: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        a_ &= b_;
      }
        break;
      case BOR: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        a_ |= b_;
      }
        break;
      case XOR: {
        word &a_ = decode_manip_value(a);
        word const b_ = decode_value(b);
        a_ ^= b_;
      }
        break;
      case IFE: {
        word const a_ = decode_value(a);
        word const b_ = decode_value(b);
        perform = a_ == b_;
      }
        break;
      case IFN: {
        word const a_ = decode_value(a);
        word const b_ = decode_value(b);
        perform = a_ != b_;
      }
        break;
      case IFG: {
        word const a_ = decode_value(a);
        word const b_ = decode_value(b);
        perform = a_ > b_;
      }
        break;
      case IFB: {
        word const a_ = decode_value(a);
        word const b_ = decode_value(b);
        perform = (a_ & b_) != 0;
      }
        break;
      }

      dump_regs();
    }
    else {
      TLK("skipped: " << std::hex << w);
      perform = true;
    }
  }
}
 catch(std::exception &e) {
   std::cerr << "ERROR: " << e.what() << std::endl;
   return 1;
 }
 catch(...) {
   std::cerr << "ERROR: unknown\n";
   return 1;
 }

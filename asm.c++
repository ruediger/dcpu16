#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef std::uint16_t word;

int main() try {
  /*
    name := [:alpha:] [:alnum:]*
    ops := 'set' | 'add' | ...
    lit := hex | dec | oct | bin
    label := name ':'
    reg := 'A' | 'B' | 'C' | 'X' | 'Y' | 'Z' | 'I' | 'J'
    specreg := 'SP' | 'PC' | 'O'
    stack := 'POP' | 'PUSH' | 'PEEK'
    var := reg | lit | specreg | stack | '[' (reg | lit | lit '+' reg) ']'
    op := ops var, var
    jsr := 'jsr' (name | lit)
    prepro := '#' preproinstr
    preproinstr := pdefine | pifdef | pelse | pendif
    pdefine := 'define' name (value)?
    pifdef := 'ifdef' name
    pelse := 'else'
    pendif := 'endif'

    expr := op | jsr | label | prepro | special | comment
    comment := ';' .*
    
    special := '.' specialinstr
    specialinstr := section | endsection | var
    var := 'var' name lit
    section := 'section' name lit
    endsection := 'end'
   */

  auto const name = qi::alpha >> *qi::alnum;
  auto const lit = qi::bin | qi::oct | qi::ushort_ | qi::hex;

  qi::symbols<char, word> ops;
  ops.add
    ("set", 1)
    ("add", 2); // ...
  qi::symbols<char, word> reg;
  reg.add
    ("A", 0)
    ("B", 1)
    ("C", 2)
    ("X", 3)
    ("Y", 4)
    ("Z", 5)
    ("I", 6)
    ("J", 7);
  qi::symbols<char, word> specreg;
  specreg.add
    ("SP", 0x1b)
    ("PC", 0x1c)
    ("O", 0x1d);
  qi::symbols<char, word> stack;
  stack.add
    ("POP", 0x18)
    ("PEEK", 0x19)
    ("PUSH", 0x1a);
  auto const var = reg | lit | specreg | stack | ( '[' >> (reg | lit | (lit >> '+' >> reg) ) >> ']' );
  auto const op = ops >> var >> ',' >> var;
  
  auto const label = name << ':';
  
  auto const jsr = qi::lit("jsr") >> ( name | lit );

  auto const section = "section" >> name >> lit;
  auto const endsection = qi::lit("end") >> -qi::lit("section");
  auto const defvar = "var" >> name >> lit;
  auto const specialinstr = section | endsection | defvar;
  auto const special = qi::lit('.') >> specialinstr;

  // prepro
  auto const pvalue = *qi::char_;
  auto const pdefine = "define" >> name >> -(pvalue);
  auto const pifdef = "ifdef" >> name;
  auto const pelse = "else";
  auto const pendif = "endif";
  auto const preproinstr = pdefine | pifdef | pelse | pendif;
  auto const prepro = qi::lit('#') >> preproinstr;

  auto const comment = qi::lit(';') >> *qi::char_;

  auto const expr = op | jsr | label | prepro | special | comment;
}
 catch(std::exception &e) {
   std::cerr << "ERROR: " << e.what() << std::endl;
   return 1;
 }
 catch(...) {
   std::cerr << "ERROR: unknown\n";
   return 1;
 }

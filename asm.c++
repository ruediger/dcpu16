#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/spirit/home/phoenix/statement/if.hpp>
#include <boost/spirit/home/phoenix/statement/sequence.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/qi.hpp>
namespace px = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef std::uint16_t word;

std::vector<std::string> code;
std::vector<word> binary;

void assemble() {
  typedef std::string::const_iterator iterator;
  typedef qi::rule<iterator, word(), ascii::space_type> rule;

  auto name = qi::alpha >> *qi::alnum;

  rule hexlit = qi::lit('0') >> (qi::lit('x') | 'X') > qi::hex [qi::_val = qi::_1];
  rule binlit = qi::lit('0') >> (qi::lit('b') | 'B') > qi::bin [qi::_val = qi::_1];
  rule octlit = qi::lit('0') > qi::oct                         [qi::_val = qi::_1];
  rule lit = (binlit | hexlit | octlit | qi::ushort_)          [qi::_val = qi::_1];

  qi::symbols<char, word> ops_sym;
  ops_sym.add
    ("set", 1)
    ("add", 2); // ...
  auto ops = qi::no_case[ ops_sym ];
  qi::symbols<char, word> reg_sym;
  reg_sym.add
    ("a", 0u)
    ("b", 1u)
    ("c", 2u)
    ("x", 3u)
    ("y", 4u)
    ("z", 5u)
    ("i", 6u)
    ("j", 7u);
  auto reg = qi::no_case[ reg_sym ];
  qi::symbols<char, word> specreg_sym;
  specreg_sym.add
    ("sp", 0x1bu)
    ("pc", 0x1cu)
    ("o", 0x1du);
  auto specreg = qi::no_case[ specreg_sym ];
  qi::symbols<char, word> stack_sym;
  stack_sym.add
    ("pop", 0x18u)
    ("peek", 0x19u)
    ("push", 0x1au);
  auto stack = qi::no_case[ stack_sym ];
  std::uint32_t next_word = 0;
  rule var =
      reg       [qi::_val = qi::_1]
    | lit       [
                 px::if_(qi::_1 <= px::val(0x1Fu))
                 [
                  qi::_val = qi::_1 + px::val(0x20u)
                 ]
                 .else_
                 [
                  qi::_val = px::val(0x1F),
                  px::ref(next_word) = px::val(~std::uint32_t(0)) & qi::_1
                 ]
                ]
    | specreg   [qi::_val = qi::_1]
    | stack     [qi::_val = qi::_1]
    | ( '[' > (reg [qi::_val = qi::_1 + px::val(0x08)]
            | (lit [px::ref(next_word) = px::val(~std::uint32_t(0)) & qi::_1] >> '+' >> reg[qi::_val = qi::_1 + px::val(0x10)])
            | (lit [px::ref(next_word) = px::val(~std::uint32_t(0)) & qi::_1]) [qi::_val = px::val(0x1e)]
      ) > ']' );

  auto op =
    ops     [qi::_val = qi::_1]
    > var   [qi::_val |= qi::_1 << px::val(4u) ]
    > ','
    > var   [qi::_val |= qi::_1 << px::val(10u) ];

  auto label = name >> ':';

  auto jsr = qi::lit("jsr") > qi::ushort_;

#if 0
  auto section = "section" >> name >> lit;
  auto endsection = qi::lit("end") >> -qi::lit("section");
  auto defvar = "var" >> name >> lit;
  auto specialinstr = section | endsection | defvar;
  auto special = qi::lit('.') >> specialinstr;
#endif

#if 0
  // prepro
  auto pvalue = *qi::char_;
  auto pdefine = "define" >> name >> -(pvalue);
  auto pifdef = "ifdef" >> name;
  auto pelse = "else";
  auto pendif = "endif";
  auto preproinstr = pdefine | pifdef | pelse | pendif;
  auto prepro = qi::lit('#') >> preproinstr;
#endif

  auto comment = qi::lit(';') >> *qi::char_;

//op | jsr | label | prepro | special | comment;

  rule expr = op | jsr | label | comment;

  unsigned lineno = 0;

  /*
  auto x = [lineno](iterator first, iterator last, iterator error_pos, std::string const &what) {
       std::cerr << "ERROR:" << lineno << ':' << (error_pos-first) << ": " << what << '\n';
       std::cerr << std::string(first, last) << '\n';
       for(unsigned i = 0; i < error_pos-first; ++i) {
         std::cerr << ' ';
       }
       std::cerr << "^\n";
       };*/

  qi::on_error<qi::fail>
    (
     expr,
     std::cout
     << px::val("Error! Expecting ")
     << boost::spirit::_4                               // what failed?
     << px::val(" here: \"")
     << px::construct<std::string>(boost::spirit::_3, boost::spirit::_2)   // iterators to error-pos, end
     << px::val("\"")
     << std::endl
     );

  for(auto const &l : code) {
    ++lineno;
    auto i = l.begin();
    word instr = 0;
    bool const r = qi::phrase_parse(i, l.end(), expr, ascii::space, instr);
    if(not r) {
      std::cerr << "ERROR! " << std::string(i, l.end()) << std::endl;
    }
    else {
      binary.push_back(instr);
      std::cerr << "OK: " << std::hex << std::setfill('0') << std::setw(4) << instr << "\n";
      if(next_word) {
        binary.push_back(next_word);
        std::cerr << "NW: " << std::hex << std::setfill('0') << std::setw(4) << word(next_word) << "\n";
        next_word = 0;
      }
    }
  }
}

int main() try {
  std::string line;
  bool attach_last = false; // set to true if line should be attacked to previous
  while(std::getline(std::cin, line)) {
    if(attach_last) {
      assert(not code.empty());
      code.back() += line;
      attach_last = false;
    }
    else {
      code.push_back(line);
    }
    if(not line.empty() and line.back() == '\\') {
      attach_last = true;
      code.back().back() = ' ';
    }
  }

  assemble();
  std::ofstream out("dcpu.out");
  if(not out) {
    std::cerr << "ERROR: Couldn't open 'dcpu.out'\n";
    return 1;
  }
  out.write(reinterpret_cast<char const*>(&binary[0]), binary.size()*sizeof(binary[0]));
}
 catch(std::exception &e) {
   std::cerr << "ERROR: " << e.what() << std::endl;
   return 1;
 }
 catch(...) {
   std::cerr << "ERROR: unknown\n";
   return 1;
 }

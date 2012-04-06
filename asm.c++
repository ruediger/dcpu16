#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef std::uint16_t word;

std::vector<std::string> code;
std::vector<word> binary;

void grammar() {
  auto name = qi::alpha >> *qi::alnum;
  auto hexlit = qi::lit('0') >> (qi::lit('x') | 'X') > qi::hex;
  auto binlit = qi::lit('0') >> (qi::lit('b') | 'B') > qi::bin;
  auto octlit = qi::lit('0') > qi::oct;
  auto lit = binlit | hexlit | octlit | qi::ushort_;

  qi::symbols<char, word> ops_sym;
  ops_sym.add
    ("set", 1)
    ("add", 2); // ...
  auto ops = qi::no_case[ ops_sym ];
  qi::symbols<char, word> reg_sym;
  reg_sym.add
    ("a", 0)
    ("b", 1)
    ("c", 2)
    ("x", 3)
    ("y", 4)
    ("z", 5)
    ("i", 6)
    ("j", 7);
  auto reg = qi::no_case[ reg_sym ];
  qi::symbols<char, word> specreg_sym;
  specreg_sym.add
    ("sp", 0x1b)
    ("pc", 0x1c)
    ("o", 0x1d);
  auto specreg = qi::no_case[ specreg_sym ];
  qi::symbols<char, word> stack_sym;
  stack_sym.add
    ("pop", 0x18)
    ("peek", 0x19)
    ("push", 0x1a);
  auto stack = qi::no_case[ stack_sym ];
  auto var = reg | lit | specreg | stack | ( '[' > (reg | lit | (lit >> '+' >> reg) ) > ']' );
  auto op = ops > var > ',' > var;
  
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

  qi::rule<std::string::const_iterator, ascii::space_type> expr = op | jsr | label | comment;

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
     << boost::phoenix::val("Error! Expecting ")
     << boost::spirit::_4                               // what failed?
     << boost::phoenix::val(" here: \"")
     << boost::phoenix::construct<std::string>(boost::spirit::_3, boost::spirit::_2)   // iterators to error-pos, end
     << boost::phoenix::val("\"")
     << std::endl
     );

  for(auto const &l : code) {
    ++lineno;
    auto i = l.begin();
    bool const r = qi::phrase_parse(i, l.end(), expr, ascii::space);
    if(not r) {
      std::cerr << "ERROR! " << std::string(i, l.end()) << std::endl;
    }
    else {
      std::cerr << "OK\n";
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

  binary.resize(0x10000);
  grammar();
}
 catch(std::exception &e) {
   std::cerr << "ERROR: " << e.what() << std::endl;
   return 1;
 }
 catch(...) {
   std::cerr << "ERROR: unknown\n";
   return 1;
 }

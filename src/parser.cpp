#include "parser.h"
#include "color.h"
#include "construct.h"
#include "scanner.h"
#include "stream.h"

#include <iostream>

#ifdef __DEBUG__
#define DEBUG(X) X
#else
#define DEBUG(X)
#endif

static std::string make_expected(char expected) {
  std::string s(1, '\''); s.push_back(expected); s.push_back('\'');
  return s;
}

static std::string make_expected(std::string expected) {
  std::string s(1, '\"'); s.append(expected); s.push_back('\"');
  return s;
}

parser::error::error(char expected, char got, stream::location loc)
  : expected_(make_expected(expected)), got_(got), loc_(loc) {
}
parser::error::error(scanner expected, char got, stream::location loc)
  : expected_(expected.get_id()), got_(got), loc_(loc) {
}
parser::error::error(std::string expected, char got, stream::location loc)
  : expected_(make_expected(expected)), got_(got), loc_(loc) {
}

typedef std::pair<parser, construct*> parser_pair_ty;

template <typename T>
parser& operator>>(parser &prs, T run) {
  // Stop parsing if the stream is invalid.
  if (!prs)
    return prs;

  stream& s = prs.get_stream();
  stream::location before = s.get_loc();

  parser_pair_ty pair = run(prs);
  if ((prs = pair.first) && before != s.get_loc())
    // Clear all the errors located before whatever we just parsed.
    prs.clear_errors(s.get_loc());
  if (pair.second)
    prs.add_construct(pair.second);
  return prs;
}

parser& operator<<(parser &prs, parser new_prs) {
  return prs = new_prs;
}

static std::function<parser_pair_ty(parser)> parse_char(char c) {
  return [c](parser prs) {
    DEBUG(std::cout << "parse_char: " << c << "\n");
    stream &s = prs.get_stream();
    char peek = s.peek();
    if (peek == c) {
      s.next();
    } else {
      prs.add_error(parser::error(c, peek, s.get_loc()));
      prs.set_valid(false);
    }
    return parser_pair_ty(prs, nullptr);
  };
}

static std::function<parser_pair_ty(parser)> parse_string(const char *str) {
  return [str](parser prs) {
    DEBUG(std::cout << "parse_string: " << str << "\n");
    stream &s = prs.get_stream();
    const char *ostr = str;
    while (*ostr && s.peek() == *ostr) { ++ostr; s.next(); }
    if (*ostr) {
      prs.add_error(parser::error(str, s.peek(), s.get_loc()));
      prs.set_valid(false);
    }
    return parser_pair_ty(prs, nullptr);
  };
}

static std::string parse_many(parser& prs, scanner scn) {
  DEBUG(std::cout << "parse_many: " << scn.get_id() << "\n");
  stream &s = prs.get_stream();
  std::string lexeme;
  while (scn(s.peek()))
    lexeme.push_back(s.next());
  if (lexeme.empty()) {
    prs.add_error(parser::error(scn, s.peek(), s.get_loc()));
    prs.set_valid(false);
  }
  return lexeme;
}


template<typename T>
static std::function<parser_pair_ty(parser)> maybe(T run) {
  return [run](parser prs) {
    auto copy_prs = prs;
    if (prs >> run)
      return parser_pair_ty(prs, nullptr);
     return parser_pair_ty(copy_prs, nullptr);
  };
}

template<typename T, typename U>
static std::function<parser_pair_ty(parser)> compose(T first, U second) {
  return [first, second](parser prs) {
    prs >> first >> second;
    return parser_pair_ty(prs, nullptr);
  };
}

template<typename T>
static bool do_try(parser &prs, T run) {
  auto copy_prs = prs;
  if (prs >> run)
    return true;
  copy_prs.merge_errors(prs);
  prs = copy_prs;
  return false;
}

static parser_pair_ty parse_spaces(parser prs) {
  DEBUG(std::cout << "parse_spaces\n");
  auto spaces = parse_many(prs, scanner::is_space);
  return parser_pair_ty(prs, nullptr);
}

static parser_pair_ty parse_maybe_spaces(parser prs) {
  DEBUG(std::cout << "parse_maybe_spaces\n");
  stream &s = prs.get_stream();
  while (scanner::is_space(s.peek())) s.next();
  return parser_pair_ty(prs, nullptr);
}

static parser_pair_ty parse_eof(parser prs) {
  DEBUG(std::cout << "parse_eof\n");
  stream &s = prs.get_stream();
  prs.set_valid(s.peek() == '\0');
  return parser_pair_ty(prs, nullptr);
}

static parser parse_comma_sep(parser &prs,
                              parser_pair_ty (*run)(parser)) {
  DEBUG(std::cout << "parse_comma_sep\n");
  if (prs >> run)
    while (do_try(prs, compose(parse_maybe_spaces, parse_char(','))))
      if (!do_try(prs, compose(parse_maybe_spaces, run)))
        break;
  return prs;
}

static parser parse_space_sep(parser &prs,
                              parser_pair_ty (*run)(parser)) {
  DEBUG(std::cout << "parse_space_sep\n");
  if (prs >> run)
    while (do_try(prs, compose(parse_spaces, run)));
  return prs;
}

static parser_pair_ty parse_id(parser prs) {
  DEBUG(std::cout << "parse_id\n");
  auto id = parse_many(prs, scanner::is_ident);
  return parser_pair_ty(prs, !prs ? nullptr : new construct_id(id));
}

static parser_pair_ty parse_word(parser prs) {
  DEBUG(std::cout << "parse_word\n");
  auto word = parse_many(prs, scanner::is_word);
  prs.set_valid(prs.is_valid() && word != ";");
  return parser_pair_ty(prs, !prs ? nullptr : new construct_word(word));
}

static parser_pair_ty parse_type_id(parser prs) {
  DEBUG(std::cout << "parse_type_id\n");
  auto id = parse_many(prs, scanner::is_ident);
  return parser_pair_ty(prs, !prs ? nullptr : new construct_type_id(id));
}

static parser_pair_ty parse_type_compound(parser prs) {
  DEBUG(std::cout << "parse_type_compound\n");
  auto ret_prs = parse_space_sep(prs, parse_type_id);
  if (ret_prs) {
    auto list = ret_prs.gather_constructs<construct_type_id>();
    return parser_pair_ty(ret_prs, new construct_type_compound(list));
  }
  return parser_pair_ty(ret_prs, nullptr);
}

static parser_pair_ty parse_type_list(parser prs) {
  DEBUG(std::cout << "parse_type_list\n");
  auto ret_prs = parse_comma_sep(prs, parse_type_compound);
  if (ret_prs) {
    auto list = ret_prs.gather_constructs<construct_type_compound>();
    return parser_pair_ty(ret_prs, new construct_type_list(list));
  }
  return parser_pair_ty(ret_prs, nullptr);
}

static parser_pair_ty parse_type_fn(parser prs) {
  if (prs >> parse_char('(')
          >> parse_maybe_spaces >> parse_type_list
          >> parse_maybe_spaces >> parse_string("->")
          >> parse_maybe_spaces >> maybe(parse_type_compound)
          >> parse_maybe_spaces >> parse_char(')')) {
    construct_type_compound *out = prs.get_construct<construct_type_compound>();
    if (!out) out = new construct_type_compound();
    construct_type_list *inp = prs.get_construct<construct_type_list>();
    return parser_pair_ty(prs, new construct_type_fn(inp, out));
  }
  return parser_pair_ty(prs, nullptr);
}

static parser_pair_ty parse_arg_id(parser prs) {
  DEBUG(std::cout << "parse_arg_id\n");
  if (prs >> parse_id) {
    construct_id *cid = prs.get_construct<construct_id>();
    construct_arg_id *ctid = new construct_arg_id(cid->get_str());
    return parser_pair_ty(prs, ctid);
  }
  return parser_pair_ty(prs, nullptr);
}

static parser_pair_ty parse_arg_compound(parser prs) {
  DEBUG(std::cout << "parse_arg_compound\n");
  auto ret_prs = parse_space_sep(prs, parse_arg_id);
  if (ret_prs) {
    auto list = ret_prs.gather_constructs<construct_arg_id>();
    return parser_pair_ty(ret_prs, new construct_arg_compound(list));
  }
  return parser_pair_ty(ret_prs, nullptr);
}

static parser_pair_ty parse_arg_list(parser prs) {
  DEBUG(std::cout << "parse_arg_list\n");
  auto ret_prs = parse_comma_sep(prs, parse_arg_compound);
  if (ret_prs) {
    auto list = ret_prs.gather_constructs<construct_arg_compound>();
    return parser_pair_ty(ret_prs, new construct_arg_list(list));
  }
  return parser_pair_ty(ret_prs, nullptr);
}

static parser_pair_ty parse_args(parser prs) {
  DEBUG(std::cout << "parse_args\n");
  if (prs >> parse_char('(') >> parse_maybe_spaces >> parse_arg_list
          >> parse_maybe_spaces >> parse_char(')')) {
    return parser_pair_ty(prs, prs.get_construct<construct_arg_list>());
  }
  return parser_pair_ty(prs, nullptr);
}

static parser_pair_ty parse_body(parser prs) {
  DEBUG(std::cout << "parse_body\n");
  auto ret_prs = parse_space_sep(prs, parse_word);
  if (ret_prs) {
    auto list = ret_prs.gather_constructs<construct_word>();
    return parser_pair_ty(ret_prs, new construct_body(list));
  }
  return parser_pair_ty(ret_prs, nullptr);
}

static parser_pair_ty parse_def(parser prs) {
  DEBUG(std::cout << "parse_def\n");
  if (!(prs >> parse_word
            >> parse_spaces       >> parse_char(':')
            >> parse_spaces       >> parse_type_fn
            >> maybe(compose(parse_spaces, parse_args))
            >> parse_maybe_spaces >> parse_string("->")
            >> maybe(compose(parse_maybe_spaces, parse_body))))
    return parser_pair_ty(prs, nullptr);

  // With this, unterminated definitions are not incorrectly shown as being
  // on the line after the actual definition. This also avoids unterminated
  // definitions generating two errors each.
  auto errors = prs.get_errors();
  if (!do_try(prs, compose(parse_spaces, parse_char(';')))) {
    prs.set_errors(errors);
    prs.set_valid(false);
  }

  if (prs) {
    auto body = prs.get_construct<construct_body>();
    if (!body) body = new construct_body();
    auto args = prs.get_construct<construct_arg_list>();
    if (!args) args = new construct_arg_list();
    auto type = prs.get_construct<construct_type_fn>();
    return parser_pair_ty(prs, new construct_def(type, args, body));
  }
  return parser_pair_ty(prs, nullptr);
}

void parser::add_error(error err) {
  errors_.insert(err);
}

void parser::merge_errors(parser &prs) {
  auto errors = prs.errors_;
  errors_.insert(errors.begin(), errors.end());
}

void parser::clear_errors() {
  errors_.clear();
}

void parser::clear_errors(stream::location loc) {
  auto it = errors_.begin(), e = errors_.end();
  while (it != e)
    if (it->get_loc() < loc)
      it = errors_.erase(it);
    else
      ++it;
}

void parser::print_error_loc(std::ostream &os, stream::location loc) const {
  os << stream_.get_line(loc.get_line()) << "\n"
     << std::string(loc.get_col() > 1 ? loc.get_col() - 1 : 0, ' ')
     << "^" << "\n";
}

void parser::print_errors(std::ostream &os) const {
  auto it = errors_.begin(), e = errors_.end();
  stream::location loc = it->get_loc();
  os << stream_.get_filename() << ":" << loc.get_line() << ":"
     << loc.get_col() << ": " << color::code::red << "error:"
     << color::code::reset << " expected ";
  if (errors_.size() == 1) {
    os << it->get_expected();
  } else if (errors_.size() == 2) {
    os << it->get_expected() << " or " << (++it)->get_expected();
  } else {
    for (; it != std::prev(e); ++it)
      os << it->get_expected() << ", ";
    os << "or " << it->get_expected();
  }
  os << ", got '" << it->get_got() << "'" << "\n";
  print_error_loc(os, loc);
}

void parser::print_error_unterminated(std::ostream &os,
                                      stream::location loc) const {
  os << stream_.get_filename() << ":" << loc.get_line() << ":"
     << loc.get_col() << ": " << color::code::red << "error:"
     << color::code::reset << " unterminated definition" << "\n";
  print_error_loc(os, loc);
}

void parser::add_construct(construct *c) {
  cons_.push(c);
}

void parser::advance() {
  stream::location loc = stream_.get_loc();
  for (; !(*this << parser(stream_) >> parse_maybe_spaces >> parse_eof);
       stream_.set_loc(loc), stream_.next(), loc = stream_.get_loc()) {

    stream_.set_loc(loc);
    if (*this << parser(stream_) >> parse_spaces >> parse_char(';')
              >> parse_spaces)
      return;

    stream_.set_loc(loc);
    if (*this << parser(stream_) >> parse_spaces >> parse_word
              >> parse_spaces >> parse_char(':') >> parse_spaces) {
      stream_.set_loc(loc);
      print_error_unterminated(std::cerr, loc);
      return;
    }
  }
  print_error_unterminated(std::cerr, loc);
}

bool parser::parse() {
  bool has_error = false;
  while (*this >> parse_maybe_spaces && stream_.peek() != '\0') {
    if (*this >> parse_def) {
      DEBUG(std::cout << "parse_def: " << *get_construct<construct_def>()
                      << "\n");
      continue;
    }
    has_error = true;
    if (!errors_.empty()) {
      print_errors(std::cerr);
      clear_errors();
    }
    advance();
  }
  return !has_error;
}


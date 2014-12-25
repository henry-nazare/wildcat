#include "parser.h"
#include "color.h"
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

template <typename T>
parser& operator>>(parser &prs, T run) {
  // Stop parsing if the stream is invalid.
  if (!prs)
    return prs;

  stream& s = prs.get_stream();
  stream::location before = s.get_loc();

  if ((prs = run(prs)) && before != s.get_loc())
    // Clear all the errors located before whatever we just parsed.
    prs.clear_errors(s.get_loc());
  return prs;
}

parser& operator<<(parser &prs, parser new_prs) {
  return prs = new_prs;
}

static std::function<parser(parser)> parse_char(char c) {
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
    return prs;
  };
}

static std::function<parser(parser)> parse_string(const char *str) {
  return [str](parser prs) {
    DEBUG(std::cout << "parse_string: " << str << "\n");
    stream &s = prs.get_stream();
    const char *ostr = str;
    while (*ostr && s.peek() == *ostr) { ++ostr; s.next(); }
    if (*ostr) {
      prs.add_error(parser::error(str, s.peek(), s.get_loc()));
      prs.set_valid(false);
    }
    return prs;
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
static std::function<parser(parser)> maybe(T run) {
  return [run](parser prs) {
    auto copy_prs = prs;
    if (prs >> run)
      return prs;
    return copy_prs;
  };
}

template<typename T, typename U>
static std::function<parser(parser)> compose(T first, U second) {
  return [first, second](parser prs) {
    prs >> first >> second;
    return prs;
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

static parser parse_spaces(parser prs) {
  DEBUG(std::cout << "parse_spaces\n");
  auto spaces = parse_many(prs, scanner::is_space);
  return prs;
}

static parser parse_maybe_spaces(parser prs) {
  DEBUG(std::cout << "parse_maybe_spaces\n");
  stream &s = prs.get_stream();
  while (scanner::is_space(s.peek())) s.next();
  return prs;
}

static parser parse_eof(parser prs) {
  DEBUG(std::cout << "parse_eof\n");
  stream &s = prs.get_stream();
  prs.set_valid(s.peek() == '\0');
  return prs;
}

static parser parse_comma_sep(parser &prs, parser (*run)(parser)) {
  DEBUG(std::cout << "parse_comma_sep\n");
  if (prs >> run)
    while (do_try(prs, compose(parse_maybe_spaces, parse_char(','))))
      if (!do_try(prs, compose(parse_maybe_spaces, run)))
        break;
  return prs;
}

static parser parse_space_sep(parser &prs, parser (*run)(parser)) {
  DEBUG(std::cout << "parse_space_sep\n");
  if (prs >> run)
    while (do_try(prs, compose(parse_spaces, run)));
  return prs;
}

static parser parse_id(parser prs) {
  DEBUG(std::cout << "parse_id\n");
  auto id = parse_many(prs, scanner::is_ident);
  DEBUG(std::cout << "parse_id: " << id << "\n");
  return prs;
}

static parser parse_word(parser prs) {
  DEBUG(std::cout << "parse_word\n");
  auto word = parse_many(prs, scanner::is_word);
  DEBUG(std::cout << "parse_word: " << word << "\n");
  prs.set_valid(prs.is_valid() && word != ";");
  return prs;
}

static parser parse_type_id(parser prs) {
  DEBUG(std::cout << "parse_type_id\n");
  return prs >> parse_id;
}

static parser parse_type_compound(parser prs) {
  DEBUG(std::cout << "parse_type_compound\n");
  return parse_space_sep(prs, parse_type_id);
}

static parser parse_type_list(parser prs) {
  DEBUG(std::cout << "parse_type_list\n");
  return parse_comma_sep(prs, parse_type_compound);
}

static parser parse_type_fn(parser prs) {
  return prs >> parse_char('(')
             >> parse_maybe_spaces >> parse_type_list
             >> parse_maybe_spaces >> parse_string("->")
             >> parse_maybe_spaces >> maybe(parse_type_compound)
             >> parse_maybe_spaces >> parse_char(')');
}

static parser parse_arg(parser prs) {
  DEBUG(std::cout << "parse_arg\n");
  auto arg = parse_many(prs, scanner::is_ident);
  DEBUG(std::cout << "parse_arg: " << arg << "\n");
  return prs;
}

static parser parse_arg_compound(parser prs) {
  DEBUG(std::cout << "parse_arg_compound\n");
  return parse_space_sep(prs, parse_arg);
}

static parser parse_arg_list(parser prs) {
  DEBUG(std::cout << "parse_arg_list\n");
  return parse_comma_sep(prs, parse_arg_compound);
}

static parser parse_args(parser prs) {
  DEBUG(std::cout << "parse_args\n");
  return prs >> parse_char('(') >> parse_maybe_spaces >> parse_arg_list
             >> parse_maybe_spaces >> parse_char(')');
}

static parser parse_body(parser prs) {
  DEBUG(std::cout << "parse_body\n");
  return parse_space_sep(prs, parse_word);
}

static parser parse_def(parser prs) {
  DEBUG(std::cout << "parse_def\n");
  prs >> parse_word
      >> parse_spaces       >> parse_char(':')
      >> parse_spaces       >> parse_type_fn
      >> parse_spaces       >> parse_args
      >> parse_maybe_spaces >> parse_string("->")
      >> maybe(compose(parse_maybe_spaces, parse_body));

  // With this, unterminated definitions are not incorrectly shown as being
  // on the line after the actual definition. This also avoids unterminated
  // definitions generating two errors each.
  auto errors = prs.get_errors();
  if (!do_try(prs, compose(parse_spaces, parse_char(';')))) {
    prs.set_errors(errors);
    prs.set_valid(false);
  }
  return prs;
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
    if (*this >> parse_def)
      continue;
    has_error = true;
    if (!errors_.empty()) {
      print_errors(std::cerr);
      clear_errors();
    }
    advance();
  }
  return !has_error;
}


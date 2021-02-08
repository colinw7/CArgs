#include <CArgs.h>
#include <CStrUtil.h>
#include <CThrow.h>

//
// The format of the options definition is a space separated list
// of option definitions of the form:
//
//   '-'<name>':'<type>[<count>][<flags>][=<value>]
//
// where
//
//   <name>  - Option name
//   <type>  - Option type
//   <count> - Value Count
//   <flags> - Option flags
//   <value> - Default value
//
// The count if supplied specifies how many values are associated
// with the option (defaults to 1 if not supplied). This is only
// used for the 'i', 'I', 'r', 'R', 's' and 'S' options.
//
// Option types are :-
//
//   'f' - flag               (no arg required)
//   'i' - unattached integer (integer arg required)
//   'I' - attached integer   (integer arg required)
//   'r' - unattached real    (real arg required)
//   'R' - attached real      (real arg required)
//   's' - unattached string  (string arg required)
//   'S' - attached string    (string arg required)
//   'c' - unattached choice  (integer arg required)
//   'C' - attached choice    (integer arg required)
//
// For 'c' or 'C' a square bracked space or comma separated list of
// allowable values must follow. If the name contains an '=' then
// then the characters before it are taken as the value string and
// the characters after it as the string representation of the
// resultant integer value.
//
// Option flags are :-
//
//   'n' - Not case sensitive when matching argument name.
//   'r' - Argument required, error is output if no supplied.
//   's' - Skip Argument (No return argument required, associated
//         arguments are not removed from the argument list).
//   'm' - Argument can appear multiple times
//
// If '=<value>' is supplied then this is used as the default value
// when the option is not specified. If it is not supplied then 0
// is used for integers and reals, NULL for strings and -1 for choices.
//
// Flag, integer and choice return 'int' (choice returns index into
// list 0,1,...). Real returns 'double'. String returns 'char *'.
//
// ------------
//
// e.g.
//
//  CArgs *cargs = new CArgs("-f -i:i -r:r -s:s -c:c[a,b,c]");
//
//  int      f;
//  int      i;
//  double   r;
//  char    *s;
//  int      c;
//
//  cargs.vparse(&argc, argv, &f, &i, &r, &s, &c);
//
// returns
//
//   f = 1, i = 5, r = 3.4, s = "Fred", c = 2
//
// when the following arguments are supplied
//
//   <app_name> -f -i 5 -r 3.4 -s Fred -c c
//
// ------------
//
// e.g.
//
//  CArgs *cargs = new CArgs("-f -i:I -r:R -s:S -c:C[a,b,c]");
//
//  int      f;
//  int      i;
//  double   r;
//  char    *s;
//  int      c;
//
//  cargs.vparse(&argc, argv, &f, &i, &r, &s, &c);
//
// returns
//
//   f = 1, i = 5, r = 3.4, s = "Fred", c = 2
//
// when the following arguments are supplied
//
//   <app_name> -f -i5 -r3.4 -sFred -cc
//
// ------------
//
// Note: all return values are initialised before the argument
//       values are extracted. Integers -> 0, Reals -> 0.0,
//       Strings -> NULL.

CArgs::
CArgs(const std::string &def)
{
  setFormat(def);
}

void
CArgs::
setFormat(const std::string &def)
{
  def_ = def;

  args_.clear();

  uint i = 0;

  while (i < def.size()) {
    while (i < def.size() && isspace(def[i]))
      ++i;

    if (i >= def.size())
      break;

    if (def[i] != '-') {
      CTHROW(std::string("Invalid Character ") + def[i]);
      return;
    }

    //------

    uint j = i;

    ++i;

    while (i < def.size() && def[i] == '-')
      ++i;

    if (i < def.size() && isalnum(def[i]))
      ++i;
    else {
      CTHROW(std::string("Invalid Character ") + def[i]);
      return;
    }

    while (i < def.size() && (isalnum(def[i]) || def[i] == '_'))
      ++i;

    std::string name = def.substr(j, i - j);

    //------

    CArgType   type     = CARG_TYPE_BOOLEAN;
    int        count    = 1;
    int        flags    = CARG_FLAG_NONE;
    bool       attached = false;
    StringList choices;

    //------

    if (i < def.size() && def[i] == ':') {
      ++i;

      if      (def[i] == 'f')
        type = CARG_TYPE_BOOLEAN;
      else if (def[i] == 'i' || def[i] == 'I')
        type = CARG_TYPE_INTEGER;
      else if (def[i] == 'r' || def[i] == 'R')
        type = CARG_TYPE_REAL;
      else if (def[i] == 's' || def[i] == 'S')
        type = CARG_TYPE_STRING;
      else if (def[i] == 'c' || def[i] == 'C')
        type = CARG_TYPE_CHOICE;

      if (def[i] == 'I' || def[i] == 'R' ||
          def[i] == 'S' || def[i] == 'C')
        attached = true;

      if (type == CARG_TYPE_CHOICE) {
        ++i;

        if (i >= def.size() || def[i] != '[') {
          CTHROW("Missing Choices for -c/C");
          return;
        }

        ++i;

        int jj = i;

        while (i < def.size() && def[i] != ']')
          ++i;

        std::string opts = def.substr(jj, i - jj);

        std::vector<std::string> words;

        CStrUtil::addFields(opts, words, " ,");

        for (uint k = 0; k < words.size(); ++k)
          choices.push_back(words[k]);
      }

      ++i;

      if (isdigit(def[i])) {
        int jj = i;

        while (i < def.size() && isdigit(def[i]))
          ++i;

        std::string istr = def.substr(jj, i - jj);

        if (! CStrUtil::isInteger(istr)) {
          CTHROW("Invalid Integer for Count");
          return;
        }

        count = CStrUtil::toInteger(istr);

        if (count <= 0) {
          CTHROW(std::string("Invalid Value for Count ") + istr);
          return;
        }
      }

      while (def[i] == 'n' || def[i] == 'r' ||
             def[i] == 's' || def[i] == 'm') {
        if      (def[i] == 'n')
          flags |= CARG_FLAG_NO_CASE;
        else if (def[i] == 'r')
          flags |= CARG_FLAG_REQUIRED;
        else if (def[i] == 's')
          flags |= CARG_FLAG_SKIP;
        else if (def[i] == 'm')
          flags |= CARG_FLAG_MULTIPLE;

        ++i;
      }
    }

    //------

    std::string defval = "";

    if (i < def.size() && def[i] == '=') {
      ++i;

      int jj = i;

      while (i < def.size() && ! isspace(def[i])) {
        if (def[i] == '\\')
          ++i;

        ++i;
      }

      defval = def.substr(jj, i - jj);
    }

    //------

    std::string desc = "";

    j = i;

    while (j < def.size() && isspace(def[j]))
      ++j;

    if (j < def.size() && def[j] == '(') {
      i = j;

      ++i;

      int jj = i;

      while (i < def.size() && def[i] != ')') {
        if (def[i] == '\\')
          ++i;

        ++i;
      }

      desc = def.substr(jj, i - jj);

      if (i < def.size() && def[i] == ')')
        ++i;
    }

    if (i < def.size() && ! isspace(def[i])) {
      CTHROW(std::string("Invalid Character ") + def[i]);
      return;
    }

    //------

    CArg *arg;

    if      (type == CARG_TYPE_BOOLEAN) {
      bool defval1 = false;

      if (defval != "") {
        if (! CStrUtil::isBool(defval)) {
          CTHROW("Invalid Boolean");
          return;
        }

        defval1 = CStrUtil::toBool(defval);
      }

      if (count == 1)
        arg = new CArgBoolean(name, flags, defval1, desc);
      else {
        CTHROW("Multiple values not supported");
        return;
      }
    }
    else if (type == CARG_TYPE_INTEGER) {
      long defval1 = 0;

      if (defval != "") {
        if (! CStrUtil::isInteger(defval)) {
          CTHROW("Invalid Integer");
          return;
        }

        defval1 = CStrUtil::toInteger(defval);
      }

      if (count == 1)
        arg = new CArgInteger(name, flags, defval1, attached, desc);
      else {
        CTHROW("Multiple values not supported");
        return;
      }
    }
    else if (type == CARG_TYPE_REAL) {
      double defval1 = 0;

      if (defval != "") {
        if (! CStrUtil::isReal(defval)) {
          CTHROW("Invalid Real");
          return;
        }

        defval1 = CStrUtil::toReal(defval);
      }

      if (count == 1)
        arg = new CArgReal(name, flags, defval1, attached, desc);
      else {
        CTHROW("Multiple values not supported");
        return;
      }
    }
    else if (type == CARG_TYPE_STRING) {
      if      (flags & CARG_FLAG_MULTIPLE)
        arg = new CArgStringList(name, flags, defval, attached, desc);
      else if (count == 1)
        arg = new CArgString(name, flags, defval, attached, desc);
      else {
        CTHROW("Multiple values not supported");
        return;
      }
    }
    else if (type == CARG_TYPE_CHOICE) {
      long defval1 = 0;

      if (defval != "") {
        if (! CStrUtil::isInteger(defval)) {
          CTHROW("Invalid Integer");
          return;
        }

        defval1 = CStrUtil::toInteger(defval);
      }

      if (count == 1)
        arg = new CArgChoice(name, flags, choices, defval1, attached, desc);
      else {
        CTHROW("Multiple values not supported");
        return;
      }
    }

    args_.push_back(arg);
  }
}

CArgs::
~CArgs()
{
  for (auto &arg : args_)
    delete arg;
}

bool
CArgs::
vparse(int argc, char **argv, ...)
{
  bool flag = parse(argc, argv);

  va_list vargs;

  va_start(vargs, argv);

  for (auto &arg : args_)
    arg->setArg(&vargs);

  va_end(vargs);

  return flag;
}

bool
CArgs::
vparse(int *argc, char **argv, ...)
{
  bool flag = parse(argc, argv);

  va_list vargs;

  va_start(vargs, argv);

  for (auto &arg : args_)
    arg->setArg(&vargs);

  va_end(vargs);

  return flag;
}

bool
CArgs::
parse(int argc, char **argv)
{
  return parse1(&argc, argv, false);
}

bool
CArgs::
parse(int *argc, char **argv)
{
  return parse1(argc, argv, true);
}

bool
CArgs::
parse1(int *argc, char **argv, bool update)
{
  skip_remaining_ = false;

  std::vector<char *> new_argv;

  int i = 0;

  if (i < *argc) {
    if (update)
      new_argv.push_back(argv[i]);

    ++i;
  }

  while (i < *argc) {
    if (argv[i][0] != '-' || skip_remaining_) {
      if (update)
        new_argv.push_back(argv[i]);

      ++i;

      continue;
    }

    if (strcmp(argv[i], "--") == 0) {
      ++i;

      skip_remaining_ = true;

      continue;
    }

    if (strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);

      ++i;

      help_ = true;

      continue;
    }

    auto parg = args_.begin();

    for ( ; parg != args_.end(); ++parg)
      if ((*parg)->optionCmp(argv[i]))
        break;

    if (parg == args_.end()) {
      bool single_letter_flag = false;

      parg = args_.begin();

      for ( ; parg != args_.end(); ++parg) {
        if ((*parg)->getType() == CARG_TYPE_BOOLEAN &&
            (*parg)->getName().size() == 2) {
          single_letter_flag = true;
          break;
        }
      }

      if (! single_letter_flag) {
        std::cerr << "Warning: Unrecognised argument " << argv[i] << std::endl;

        if (update)
          new_argv.push_back(argv[i]);

        ++i;

        continue;
      }

      bool found = false;

      for (int j = 1; argv[i][j] != '\0'; ++j) {
        found = false;

        parg = args_.begin();

        for ( ; parg != args_.end(); ++parg) {
          if ((*parg)->getType() != CARG_TYPE_BOOLEAN ||
              (*parg)->getName().size() != 2)
            continue;

          if (argv[i][j] == (*parg)->getName()[1]) {
            found = true;
            break;
          }
        }

        if (! found) {
          std::cerr << "Warning: Unrecognised argument -" << argv[i][j] << std::endl;
          break;
        }
      }

      if (! found) {
        if (update)
          new_argv.push_back(argv[i]);

        ++i;

        continue;
      }

      for (int j = 1; argv[i][j] != '\0'; ++j) {
        parg = args_.begin();

        for ( ; parg != args_.end(); ++parg) {
          if ((*parg)->getType() != CARG_TYPE_BOOLEAN ||
              (*parg)->getName().size() != 2)
            continue;

          if (argv[i][j] == (*parg)->getName()[1]) {
            (*parg)->setValue("", nullptr, 0);

            if (update) {
              if ((*parg)->getSkip()) {
                char *argv1 = new char [3];

                argv1[0] = '-';
                argv1[1] = argv[i][j];
                argv1[2] = '\0';

                new_argv.push_back(argv1);
              }
            }
          }
        }
      }

      ++i;
    }
    else {
      int num_args = (*parg)->getNumArgs();

      if (i + num_args >= *argc) {
        std::cerr << "Error: Missing Value for " << argv[i] << std::endl;
        break;
      }

      ++i;

      bool flag = (*parg)->setValue(argv[i - 1], (const char **) &argv[i], *argc - i);

      if (! flag)
        std::cerr << "Error: Invalid Value " << argv[i] << " for " << argv[i - 1] << std::endl;

      if (update) {
        if ((*parg)->getSkip()) {
          new_argv.push_back(argv[i - 1]);

          for (int j = 0; j < num_args; ++j)
            new_argv.push_back(argv[i + j]);
        }
      }

      i += num_args;
    }
  }

  if (update) {
    *argc = new_argv.size();

    for (i = 0; i < *argc; ++i)
      argv[i] = new_argv[i];
  }

  if (! checkRequired())
    return false;

  return true;
}

bool
CArgs::
parse(const std::vector<std::string> &args)
{
  std::vector<std::string> args1 = args;

  return parse1(args1, false);
}

bool
CArgs::
parse(std::vector<std::string> &args)
{
  return parse1(args, true);
}

bool
CArgs::
parse1(std::vector<std::string> &args, bool update)
{
  int num_args = args.size();

  std::vector<std::string> new_args;

  int i = 0;

  if (i < num_args) {
    if (update)
      new_args.push_back(args[i]);

    ++i;
  }

  while (i < num_args) {
    int len = args[i].size();

    if (len == 0 || args[i][0] != '-') {
      if (update)
        new_args.push_back(args[i]);

      ++i;

      continue;
    }

    if (args[i] == "--help") {
      usage(args[0]);

      ++i;

      help_ = true;

      continue;
    }

    auto parg = args_.begin();

    for ( ; parg != args_.end(); ++parg)
      if ((*parg)->optionCmp(args[i]))
        break;

    if (parg == args_.end()) {
      bool single_letter_flag = false;

      parg = args_.begin();

      for ( ; parg != args_.end(); ++parg) {
        if ((*parg)->getType() == CARG_TYPE_BOOLEAN &&
            (*parg)->getName().size() == 2) {
          single_letter_flag = true;
          break;
        }
      }

      if (! single_letter_flag) {
        std::cerr << "Warning: Unrecognised argument " << args[i] << std::endl;

        if (update)
          new_args.push_back(args[i]);

        ++i;

        continue;
      }

      bool found = false;

      for (int j = 1; j < len; ++j) {
        found = false;

        parg = args_.begin();

        for ( ; parg != args_.end(); ++parg) {
          if ((*parg)->getType() != CARG_TYPE_BOOLEAN ||
              (*parg)->getName().size() != 2)
            continue;

          if (args[i][j] == (*parg)->getName()[1]) {
            found = true;
            break;
          }
        }

        if (! found) {
          std::cerr << "Warning: Unrecognised argument -" << args[i][j] << std::endl;

          break;
        }
      }

      if (! found) {
        if (update)
          new_args.push_back(args[i]);

        ++i;

        continue;
      }

      for (int j = 1; j < len; ++j) {
        parg = args_.begin();

        for ( ; parg != args_.end(); ++parg) {
          if ((*parg)->getType() != CARG_TYPE_BOOLEAN ||
              (*parg)->getName().size() != 2)
            continue;

          if (args[i][j] == (*parg)->getName()[1]) {
            (*parg)->setValue("", nullptr, 0);

            if (update) {
              if ((*parg)->getSkip()) {
                std::string args1 = "-x";

                args1[1] = args[i][j];

                new_args.push_back(args1);
              }
            }
          }
        }
      }

      ++i;
    }
    else {
      int num_args1 = (*parg)->getNumArgs();

      if (i + num_args1 >= num_args) {
        std::cerr << "Error: Missing Value for " << args[i] << std::endl;
        break;
      }

      ++i;

      std::vector<std::string> args1;

      for (int j = i; j < num_args; ++j)
        args1.push_back(args[i]);

      bool flag = (*parg)->setValue(args[i - 1], args1);

      if (! flag)
        std::cerr << "Error: Invalid Value " << args[i] << " for " << args[i - 1] << std::endl;

      if (update) {
        if ((*parg)->getSkip()) {
          new_args.push_back(args[i - 1]);

          for (int j = 0; j < num_args1; ++j)
            new_args.push_back(args[i + j]);
        }
      }

      i += num_args1;
    }
  }

  if (update)
    args = new_args;

  if (! checkRequired())
    return false;

  return true;
}

bool
CArgs::
getBooleanArg(const std::string &name) const
{
  CArgBoolean *arg = lookupBooleanArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not Boolean"));
    return false;
  }

  return arg->getValue();
}

bool
CArgs::
getBooleanArg(int i) const
{
  CArg *arg = getArg(i);

  CArgBoolean *arg1 = dynamic_cast<CArgBoolean *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not Boolean"));
    return false;
  }

  return arg1->getValue();
}

long
CArgs::
getIntegerArg(const std::string &name) const
{
  CArgInteger *arg = lookupIntegerArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not Integer"));
    return 0;
  }

  return arg->getValue();
}

long
CArgs::
getIntegerArg(int i) const
{
  CArg *arg = getArg(i);

  CArgInteger *arg1 = dynamic_cast<CArgInteger *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not Integer"));
    return 0;
  }

  return arg1->getValue();
}

double
CArgs::
getRealArg(const std::string &name) const
{
  CArgReal *arg = lookupRealArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not Real"));
    return 0.0;
  }

  return arg->getValue();
}

double
CArgs::
getRealArg(int i) const
{
  CArg *arg = getArg(i);

  CArgReal *arg1 = dynamic_cast<CArgReal *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not Real"));
    return 0.0;
  }

  return arg1->getValue();
}

std::string
CArgs::
getStringArg(const std::string &name) const
{
  CArgString *arg = lookupStringArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not String"));
    return "";
  }

  return arg->getValue();
}

std::string
CArgs::
getStringArg(int i) const
{
  CArg *arg = getArg(i);

  CArgString *arg1 = dynamic_cast<CArgString *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not String"));
    return "";
  }

  return arg1->getValue();
}

CArgs::StringList
CArgs::
getStringListArg(const std::string &name) const
{
  CArgStringList *arg = lookupStringListArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not String List"));
    StringList t;
    return t;
  }

  return arg->getValue();
}

CArgs::StringList
CArgs::
getStringListArg(int i) const
{
  CArg *arg = getArg(i);

  CArgStringList *arg1 = dynamic_cast<CArgStringList *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not String List"));
    StringList t;
    return t;
  }

  return arg1->getValue();
}

long
CArgs::
getChoiceArg(const std::string &name) const
{
  CArgChoice *arg = lookupChoiceArg(name);

  if (! arg) {
    CTHROW(std::string("Option ") + name + std::string(" is not Choice"));
    return -1;
  }

  return arg->getValue();
}

long
CArgs::
getChoiceArg(int i) const
{
  CArg *arg = getArg(i);

  CArgChoice *arg1 = dynamic_cast<CArgChoice *>(arg);

  if (! arg1) {
    CTHROW(std::string("Option ") + arg->getName() + std::string(" is not Choice"));
    return -1;
  }

  return arg1->getValue();
}

bool
CArgs::
isBooleanArg(const std::string &name) const
{
  CArgBoolean *arg = lookupBooleanArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isBooleanArg(int i) const
{
  CArg *arg = getArg(i);

  CArgBoolean *arg1 = dynamic_cast<CArgBoolean *>(arg);

  if (! arg1)
    return false;

  return true;
}

bool
CArgs::
isIntegerArg(const std::string &name) const
{
  CArgInteger *arg = lookupIntegerArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isIntegerArg(int i) const
{
  CArg *arg = getArg(i);

  CArgInteger *arg1 = dynamic_cast<CArgInteger *>(arg);

  if (! arg1)
    return false;

  return true;
}

bool
CArgs::
isBooleanArgSet(const std::string &name) const
{
  CArgBoolean *arg = lookupBooleanArg(name);

  if (! arg)
    return false;

  return arg->getSet();
}

bool
CArgs::
isIntegerArgSet(const std::string &name) const
{
  CArgInteger *arg = lookupIntegerArg(name);

  if (! arg)
    return false;

  return arg->getSet();
}

bool
CArgs::
isRealArg(const std::string &name) const
{
  CArgReal *arg = lookupRealArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isRealArg(int i) const
{
  CArg *arg = getArg(i);

  CArgReal *arg1 = dynamic_cast<CArgReal *>(arg);

  if (! arg1)
    return false;

  return true;
}

bool
CArgs::
isRealArgSet(const std::string &name) const
{
  CArgReal *arg = lookupRealArg(name);

  if (! arg)
    return false;

  return arg->getSet();
}

bool
CArgs::
isStringArg(const std::string &name) const
{
  CArgString *arg = lookupStringArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isStringArg(int i) const
{
  CArg *arg = getArg(i);

  CArgString *arg1 = dynamic_cast<CArgString *>(arg);

  if (! arg1)
    return false;

  return true;
}

bool
CArgs::
isStringArgSet(const std::string &name) const
{
  CArgString *arg = lookupStringArg(name);

  if (! arg)
    return false;

  return arg->getSet();
}

bool
CArgs::
isStringListArg(const std::string &name) const
{
  CArgStringList *arg = lookupStringListArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isStringListArg(int i) const
{
  CArg *arg = getArg(i);

  CArgStringList *arg1 = dynamic_cast<CArgStringList *>(arg);

  if (! arg1)
    return false;

  return true;
}

bool
CArgs::
isStringListArgSet(const std::string &name) const
{
  CArgStringList *arg = lookupStringListArg(name);

  if (! arg)
    return false;

  return arg->getSet();
}

bool
CArgs::
isChoiceArg(const std::string &name) const
{
  CArgChoice *arg = lookupChoiceArg(name);

  if (! arg)
    return false;

  return true;
}

bool
CArgs::
isChoiceArg(int i) const
{
  CArg *arg = getArg(i);

  CArgChoice *arg1 = dynamic_cast<CArgChoice *>(arg);

  if (! arg1)
    return false;

  return true;
}

CArgBoolean *
CArgs::
lookupBooleanArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgBoolean *arg1 = dynamic_cast<CArgBoolean*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArgInteger *
CArgs::
lookupIntegerArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgInteger *arg1 = dynamic_cast<CArgInteger*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArgReal *
CArgs::
lookupRealArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgReal *arg1 = dynamic_cast<CArgReal*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArgString *
CArgs::
lookupStringArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgString *arg1 = dynamic_cast<CArgString*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArgStringList *
CArgs::
lookupStringListArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgStringList *arg1 = dynamic_cast<CArgStringList*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArgChoice *
CArgs::
lookupChoiceArg(const std::string &name) const
{
  CArg *arg = lookupArg(name);

  if (! arg)
    return nullptr;

  CArgChoice *arg1 = dynamic_cast<CArgChoice*>(arg);

  if (! arg1)
    return nullptr;

  return arg1;
}

CArg *
CArgs::
lookupArg(const std::string &name) const
{
  for (auto &arg : args_)
    if (arg->nameCmp(name))
      return arg;

  return nullptr;
}

void
CArgs::
resetSet()
{
  for (auto &arg : args_)
    arg->setSet(false);
}

bool
CArgs::
checkRequired()
{
  bool all_found = true;

  for (auto &arg : args_) {
    if (arg->getRequired() && ! arg->getSet()) {
      std::cerr << "Required argument " << arg->getName() << " not supplied" << std::endl;
      all_found = false;
    }
  }

  return all_found;
}

bool
CArgs::
checkOption(const char *arg, std::string &opt)
{
  opt = "";

  if (strcmp(arg, "--") == 0) {
    skip_remaining_ = true;

    return true;
  }

  if (skip_remaining_ || arg[0] != '-')
    return false;

  opt = &arg[1];

  return true;
}

void
CArgs::
unhandledOpt(const std::string &opt)
{
  if (opt != "")
    std::cerr << "Unhandled option: -" << opt << std::endl;
}

void
CArgs::
usage(const std::string &cmd) const
{
  int max_name_len = 0;

  std::cerr << cmd << " ";

  for (auto &arg : args_) {
    if (! arg->getRequired())
      std::cerr << "[";

    std::cerr << arg->getName();

    max_name_len = std::max((int) arg->getName().size(), max_name_len);

    CArgType type = arg->getType();

    if (type != CARG_TYPE_BOOLEAN && ! arg->getAttached())
      std::cerr << " ";

    if      (type == CARG_TYPE_INTEGER)
      std::cerr << "<integer>";
    else if (type == CARG_TYPE_REAL)
      std::cerr << "<real>";
    else if (type == CARG_TYPE_STRING)
      std::cerr << "<string>";
    else if (type == CARG_TYPE_CHOICE)
      std::cerr << "<choice>";

    if (! arg->getRequired())
      std::cerr << "]";

    std::cerr << " ";
  }

  std::cerr << std::endl;

  for (auto &arg : args_) {
    std::cerr << " ";

    std::cerr << arg->getName();

    for (uint i = 0; i < max_name_len - arg->getName().size(); ++i)
      std::cerr << " ";

    std::cerr << " : ";

    std::cerr << arg->getDesc();

    std::cerr << std::endl;
  }
}

void
CArgs::
print() const
{
  for (auto &arg : args_)
    arg->print();
}

//-------

CArg::
CArg(const std::string &name, CArgType type, int flags, bool attached, const std::string &desc) :
 name_(name), type_(type), flags_(flags), attached_(attached), desc_(desc)
{
}

bool
CArg::
optionCmp(const std::string &opt)
{
  if (! attached_) {
    if (flags_ & CARG_FLAG_NO_CASE)
      return (CStrUtil::casecmp(opt, name_) == 0);
    else
      return (opt == name_);
  }
  else {
    if (opt.size() <= name_.size())
      return false;

    std::string opt1 = opt.substr(0, name_.size());

    if (flags_ & CARG_FLAG_NO_CASE)
      return (CStrUtil::casecmp(opt1, name_) == 0);
    else
      return (opt1 == name_);
  }
}

bool
CArg::
nameCmp(const std::string &name)
{
  if (flags_ & CARG_FLAG_NO_CASE)
    return (CStrUtil::casecmp(name, name_) == 0);
  else
    return (name == name_);
}

int
CArg::
getNumArgs() const
{
  if (attached_)
    return 0;
  else
    return getNumArgs1();
}

bool
CArg::
setValue(const char *opt, const char **args, int num_args)
{
  if (! attached_)
    set_ = setValue1(args, num_args);
  else {
    char *arg0 = strndup_m(&opt[name_.size()], strlen(opt) - name_.size());

    char **args1 = new char * [num_args + 1];

    args1[0] = arg0;

    for (int i = 1; i <= num_args; i++)
      args1[i] = (char *) args[i - 1];

    set_ = setValue1((const char **) args1, num_args + 1);

    delete [] args1;

    free(arg0);
  }

  return set_;
}

bool
CArg::
setValue(const std::string &opt, const std::vector<std::string> &args)
{
  int    num_args1 = args.size();
  char **args1     = new char * [num_args1];

  for (int i = 0; i < num_args1; ++i)
    args1[i] = (char *) args[i].c_str();

  bool rc = setValue(opt.c_str(), (const char **) args1, num_args1);

  delete [] args1;

  return rc;
}

bool
CArg::
setArg(va_list *vargs)
{
  if (flags_ & CARG_FLAG_SKIP)
    return true;

  return setArg1(vargs);
}

void
CArg::
print() const
{
  std::cout << "Name     " << name_                           << std::endl;
  std::cout << "Type     " << typeToString(type_)             << std::endl;
  std::cout << "Flags    " << flagsToString(flags_)           << std::endl;
  std::cout << "Attached " << (attached_  ? "true" : "false") << std::endl;
}

std::string
CArg::
typeToString(CArgType type) const
{
  switch (type) {
    case CARG_TYPE_BOOLEAN:
      return "Boolean";
    case CARG_TYPE_INTEGER:
      return "Integer";
    case CARG_TYPE_REAL:
      return "Real";
    case CARG_TYPE_STRING:
      return "String";
    case CARG_TYPE_CHOICE:
      return "Choice";
    default:
      return "????";
  }
}

std::string
CArg::
flagsToString(int flags) const
{
  std::string desc;

  if (flags == CARG_FLAG_NONE)
    return "None";

  if (flags & CARG_FLAG_NO_CASE)
    desc += "No Case ";

  if (flags & CARG_FLAG_REQUIRED)
    desc += "Required ";

  if (flags & CARG_FLAG_SKIP)
    desc += "Skip ";

  return desc;
}

//------

CArgBoolean::
CArgBoolean(const std::string &name, int flags, bool defval, const std::string &desc) :
 CArg(name, CARG_TYPE_BOOLEAN, flags, false, desc), value_(defval), defval_(defval)
{
}

bool
CArgBoolean::
setValue1(const char **, int)
{
  value_ = true;

  return true;
}

bool
CArgBoolean::
setArg1(va_list *vargs)
{
  bool *value = va_arg(*vargs, bool *);

  if (! value)
    return false;

  *value = value_;

  return true;
}

void
CArgBoolean::
print() const
{
  CArg::print();

  std::cout << "Value    " << (value_  ? "true" : "false") << std::endl;
  std::cout << "Default  " << (defval_ ? "true" : "false") << std::endl;
}

//-------

CArgInteger::
CArgInteger(const std::string &name, int flags, long defval, bool attached,
            const std::string &desc) :
 CArg(name, CARG_TYPE_INTEGER, flags, attached, desc), value_(defval), defval_(defval) {
}

bool
CArgInteger::
setValue1(const char **args, int)
{
  if (! CStrUtil::isInteger(args[0]))
    return false;

  value_ = CStrUtil::toInteger(args[0]);

  return true;
}

bool
CArgInteger::
setArg1(va_list *vargs)
{
  long *value = va_arg(*vargs, long *);

  if (! value)
    return false;

  *value = value_;

  return true;
}

void
CArgInteger::
print() const
{
  CArg::print();

  std::cout << "Value    " << value_  << std::endl;
  std::cout << "Default  " << defval_ << std::endl;
}

//-------

CArgReal::
CArgReal(const std::string &name, int flags, double defval, bool attached,
         const std::string &desc) :
 CArg(name, CARG_TYPE_REAL, flags, attached, desc), value_(defval), defval_(defval)
{
}

bool
CArgReal::
setValue1(const char **args, int)
{
  if (! CStrUtil::isReal(args[0]))
    return false;

  value_ = CStrUtil::toReal(args[0]);

  return true;
}

bool
CArgReal::
setArg1(va_list *vargs)
{
  double *value = va_arg(*vargs, double *);

  if (! value)
    return false;

  *value = value_;

  return true;
}

void
CArgReal::
print() const
{
  CArg::print();

  std::cout << "Value    " << value_  << std::endl;
  std::cout << "Default  " << defval_ << std::endl;
}

//------

CArgString::
CArgString(const std::string &name, int flags, const std::string &defval, bool attached,
           const std::string &desc) :
 CArg(name, CARG_TYPE_STRING, flags, attached, desc), value_(defval), defval_(defval)
{
}

bool
CArgString::
setValue1(const char **args, int)
{
  value_ = args[0];

  return true;
}

bool
CArgString::
setArg1(va_list *vargs)
{
  std::string *value = va_arg(*vargs, std::string *);

  if (! value)
    return false;

  *value = value_;

  return true;
}

void
CArgString::
print() const
{
  CArg::print();

  std::cout << "Value    " << value_  << std::endl;
  std::cout << "Default  " << defval_ << std::endl;
}

//------

CArgStringList::
CArgStringList(const std::string &name, int flags, const std::string &defval, bool attached,
               const std::string &desc) :
 CArg(name, CARG_TYPE_STRING, flags, attached, desc), defval_(defval)
{
}

bool
CArgStringList::
setValue1(const char **args, int)
{
  values_.push_back(args[0]);

  return true;
}

bool
CArgStringList::
setArg1(va_list *vargs)
{
  std::string *value = va_arg(*vargs, std::string *);

  if (! value)
    return false;

  if (values_.empty())
    return false;

  *value = values_[0];

  return true;
}

void
CArgStringList::
print() const
{
  CArg::print();

  int num_values = values_.size();

  std::cout << "Values   ";

  for (int i = 0; i < num_values; ++i) {
    if (i > 0)
      std::cout << ", ";

    std::cout << values_[i];
  }

  std::cout << std::endl;

  std::cout << "Default  " << defval_ << std::endl;
}

//------

CArgChoice::
CArgChoice(const std::string &name, int flags, const ChoiceList &choices, long defval,
           bool attached, const std::string &desc) :
 CArg(name, CARG_TYPE_CHOICE, flags,  attached, desc), value_(defval),
 choices_(choices), defval_(defval)
{
}

bool
CArgChoice::
setValue1(const char **args, int)
{
  long value = 0;

  std::string choice = args[0];

  ChoiceList::const_iterator pstring1 = choices_.begin();
  ChoiceList::const_iterator pstring2 = choices_.end  ();

  for ( ; pstring1 != pstring2; ++pstring1) {
    if (choice == *pstring1) {
      value_ = value;

      return true;
    }

    value++;
  }

  return false;
}

bool
CArgChoice::
setArg1(va_list *vargs)
{
  long *value = va_arg(*vargs, long *);

  if (! value)
    return false;

  *value = value_;

  return true;
}

void
CArgChoice::
print() const
{
  CArg::print();

  std::cout << "Value    " << value_  << std::endl;
  std::cout << "Default  " << defval_ << std::endl;

  std::cout << "Choices ";

  ChoiceList::const_iterator pstring1 = choices_.begin();
  ChoiceList::const_iterator pstring2 = choices_.end  ();

  for ( ; pstring1 != pstring2; ++pstring1)
    std::cout << " " << *pstring1;

  std::cout << std::endl;
}

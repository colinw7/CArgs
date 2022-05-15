#ifndef CARGS_H
#define CARGS_H

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <iostream>

enum CArgType {
  CARG_TYPE_NONE,
  CARG_TYPE_BOOLEAN,
  CARG_TYPE_INTEGER,
  CARG_TYPE_REAL,
  CARG_TYPE_STRING,
  CARG_TYPE_CHOICE
};

enum CArgFlag {
  CARG_FLAG_NONE     = 0,
  CARG_FLAG_NO_CASE  = (1<<0),
  CARG_FLAG_REQUIRED = (1<<1),
  CARG_FLAG_SKIP     = (1<<2),
  CARG_FLAG_MULTIPLE = (1<<3)
};

//---

class CArg {
 public:
  CArg(const std::string &name, CArgType type, int flags, bool attached, const std::string &desc);

  virtual ~CArg() { }

  bool optionCmp(const std::string &opt);

  bool nameCmp(const std::string &name);

  virtual int getNumArgs() const;

  virtual int getNumArgs1() const = 0;

  bool setValue(const char *opt, const char **args, int num_args);

  bool setValue(const std::string &opt, const std::vector<std::string> &args);

  virtual bool setValue1(const char **args, int num_args) = 0;

  bool setArg(va_list *vargs);

  virtual bool setArg1(va_list *vargs) = 0;

  std::string getName() const { return name_; }

  CArgType getType() const { return type_; }

  bool getRequired() const { return flags_ & CARG_FLAG_REQUIRED; }

  bool getSkip() const { return flags_ & CARG_FLAG_SKIP; }

  bool getAttached() const { return attached_; }

  bool getSet() const { return set_; }
  void setSet(bool set) { set_ = set; }

  const std::string &getDesc() const { return desc_; }

  virtual void print() const;

 private:
  std::string typeToString(CArgType type) const;

  std::string flagsToString(int flags) const;

 private:
  std::string name_;
  CArgType    type_     { CARG_TYPE_NONE };
  int         flags_    { 0 };
  bool        attached_ { false };
  bool        set_      { false };
  std::string desc_;
};

//---

class CArgBoolean : public CArg {
 public:
  CArgBoolean(const std::string &name, int flags, bool defval, const std::string &desc);

  virtual int getNumArgs1() const { return 0; }

  virtual bool setValue1(const char **, int);

  virtual bool setArg1(va_list *vargs);

  bool getValue() const { return value_; }

  virtual void print() const;

 private:
  bool value_  { false };
  bool defval_ { false };
};

//---

class CArgInteger : public CArg {
 public:
  CArgInteger(const std::string &name, int flags, long defval, bool attached,
              const std::string &desc);

  virtual int getNumArgs1() const { return 1; }

  virtual bool setValue1(const char **args, int);

  virtual bool setArg1(va_list *vargs);

  long getValue() const { return value_; }

  virtual void print() const;

 private:
  long value_  { 0 };
  long defval_ { 0 };
};

//---

class CArgReal : public CArg {
 public:
  CArgReal(const std::string &name, int flags, double defval, bool attached,
           const std::string &desc);

  virtual int getNumArgs1() const { return 1; }

  virtual bool setValue1(const char **args, int);

  virtual bool setArg1(va_list *vargs);

  double getValue() const { return value_; }

  virtual void print() const;

 private:
  double value_  { 0.0 };
  double defval_ { 0.0 };
};

//---

class CArgString : public CArg {
 public:
  CArgString(const std::string &name, int flags, const std::string &defval,
             bool attached, const std::string &desc);

  virtual int getNumArgs1() const { return 1; }

  virtual bool setValue1(const char **args, int);

  virtual bool setArg1(va_list *vargs);

  const std::string &getValue() const { return value_; }

  virtual void print() const;

 private:
  std::string value_;
  std::string defval_;
};

//---

class CArgStringList : public CArg {
 public:
  typedef std::vector<std::string> ValueList;

 public:
  CArgStringList(const std::string &name, int flags, const std::string &defval,
                 bool attached, const std::string &desc);

  virtual int getNumArgs1() const { return int(values_.size()); }

  virtual bool setValue1(const char **args, int);

  virtual bool setArg1(va_list *vargs);

  const ValueList &getValue() const { return values_; }

  virtual void print() const;

 private:
  ValueList   values_;
  std::string defval_;
};

//---

class CArgChoice : public CArg {
 public:
  typedef std::vector<std::string> ChoiceList;

 public:
  CArgChoice(const std::string &name, int flags, const ChoiceList &choices,
             long defval, bool attached, const std::string &desc);

  virtual int getNumArgs1() const { return 1; }

  virtual bool setValue1(const char **args, int);

  virtual bool setArg1(va_list *vargs);

  long getValue() const { return value_; }

  virtual void print() const;

 private:
  long       value_ { 0 };
  ChoiceList choices_;
  long       defval_ { 0 };
};

//---

class CArgs {
 public:
  typedef std::vector<CArg *>      ArgList;
  typedef std::vector<std::string> StringList;

 public:
  CArgs(const std::string &def="");
 ~CArgs();

  void setFormat(const std::string &def);

  bool isHelp() const { return help_; }

  //---

  bool vparse(int  argc, char **argv, ...);
  bool vparse(int *argc, char **argv, ...);

  bool parse(int  argc, char **argv);
  bool parse(int *argc, char **argv);
  bool parse(const std::vector<std::string> &args);
  bool parse(std::vector<std::string> &args);

  //---

  bool isBooleanArg   (const std::string &name) const;
  bool isIntegerArg   (const std::string &name) const;
  bool isRealArg      (const std::string &name) const;
  bool isStringArg    (const std::string &name) const;
  bool isStringListArg(const std::string &name) const;
  bool isChoiceArg    (const std::string &name) const;

  bool isBooleanArg   (int i) const;
  bool isIntegerArg   (int i) const;
  bool isRealArg      (int i) const;
  bool isStringArg    (int i) const;
  bool isStringListArg(int i) const;
  bool isChoiceArg    (int i) const;

  bool isBooleanArgSet   (const std::string &name) const;
  bool isIntegerArgSet   (const std::string &name) const;
  bool isRealArgSet      (const std::string &name) const;
  bool isStringArgSet    (const std::string &name) const;
  bool isStringListArgSet(const std::string &name) const;

  //---

  bool        getBooleanArg   (const std::string &name) const;
  long        getIntegerArg   (const std::string &name) const;
  double      getRealArg      (const std::string &name) const;
  std::string getStringArg    (const std::string &name) const;
  StringList  getStringListArg(const std::string &name) const;
  long        getChoiceArg    (const std::string &name) const;

  bool        getBooleanArg   (int i) const;
  long        getIntegerArg   (int i) const;
  double      getRealArg      (int i) const;
  std::string getStringArg    (int i) const;
  StringList  getStringListArg(int i) const;
  long        getChoiceArg    (int i) const;

  template<typename T> T getArg(const std::string &name) const {
    T dummy;

    return getArgHelper(name, dummy);
  }

 private:
  bool        getArgHelper(const std::string &name, const bool &) const {
    return getBooleanArg   (name); }
  long        getArgHelper(const std::string &name, const long &) const {
    return getIntegerArg   (name); }
  double      getArgHelper(const std::string &name, const double &) const {
    return getRealArg      (name); }
  std::string getArgHelper(const std::string &name, const std::string &) const {
    return getStringArg    (name); }
  StringList  getArgHelper(const std::string &name, const StringList &) const {
    return getStringListArg(name); }

  //---

 public:
  void resetSet();
  bool checkRequired();

  int   getNumArgs() const { return int(args_.size()); }
  CArg *getArg(int i) const { return args_[size_t(i)]; }

  bool checkOption(const char *arg, std::string &opt);

  void unhandledOpt(const std::string &opt);

  void usage(const std::string &cmd) const;

  void print() const;

 private:
  CArgBoolean    *lookupBooleanArg   (const std::string &name) const;
  CArgInteger    *lookupIntegerArg   (const std::string &name) const;
  CArgReal       *lookupRealArg      (const std::string &name) const;
  CArgString     *lookupStringArg    (const std::string &name) const;
  CArgStringList *lookupStringListArg(const std::string &name) const;
  CArgChoice     *lookupChoiceArg    (const std::string &name) const;

  bool parse1(int *argc, char **argv, bool update);
  bool parse1(std::vector<std::string> &args, bool update);

  CArg *lookupArg(const std::string &name) const;

 private:
  std::string def_;
  ArgList     args_;
  bool        skip_remaining_ { false };
  bool        help_ { false };
};

#endif

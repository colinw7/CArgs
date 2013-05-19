#include <CArgs.h>

static std::string opts = "\
-1:f (one) \
-2:f (two) \
-3:f (three) \
-f:fr=1 \
-i:ir=1 \
-I:Ir=3 \
-r:rr=4.5 \
-R:Rr=8.3 \
-s:sr=Fred \
-S:Sr=Bill \
-c:c[a,b,c]r \
-C:C[d,e,f]r";

int
main(int argc, char **argv)
{
  CArgs cargs(opts);

  // cargs->print();

  cargs.usage(argv[0]);

  cargs.parse(&argc, argv);

  std::cout << "-1 " << cargs.getBooleanArg("-1") << std::endl;
  std::cout << "-2 " << cargs.getBooleanArg("-2") << std::endl;
  std::cout << "-3 " << cargs.getBooleanArg("-3") << std::endl;
  std::cout << "-f " << cargs.getBooleanArg("-f") << std::endl;
  std::cout << "-i " << cargs.getIntegerArg("-i") << std::endl;
  std::cout << "-I " << cargs.getIntegerArg("-I") << std::endl;
  std::cout << "-r " << cargs.getRealArg   ("-r") << std::endl;
  std::cout << "-R " << cargs.getRealArg   ("-R") << std::endl;
  std::cout << "-s " << cargs.getStringArg ("-s") << std::endl;
  std::cout << "-S " << cargs.getStringArg ("-S") << std::endl;
  std::cout << "-c " << cargs.getChoiceArg ("-c") << std::endl;
  std::cout << "-C " << cargs.getChoiceArg ("-C") << std::endl;

  for (int i = 1; i < argc; i++)
    std::cout << argv[i] << std::endl;

  return 0;
}

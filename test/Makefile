CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = .
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CArgsTest

SRC = \
CArgsTest.cpp

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-I$(INC_DIR) \
-I.

LFLAGS = \
-L$(LIB_DIR) \
-L../../CArgs/lib \
-L../../CStrUtil/lib \

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CArgsTest

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CArgsTest: $(OBJS) $(LIB_DIR)/libCArgs.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CArgsTest $(OBJS) $(LFLAGS) -lCArgs -lCStrUtil

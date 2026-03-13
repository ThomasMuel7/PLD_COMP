# config.mk contains the paths to antlr4 etc.
# Each student should have a config.mk corresponding to her system.
# Examples are  ubuntu.mk, DI.mk, fedora.mk
# Then config.mk should be in the .gitignore of your project
include config.mk

CC=g++
CCFLAGS=-g -c -std=c++17 -I$(ANTLRINC) -Wno-attributes # -Wno-defaulted-function-deleted -Wno-unknown-warning-option
LDFLAGS=-g
SRC_DIR = src
GENERATED_DIR = generated
MAIN_FILE = main.cpp

default: all
all: ifcc

##########################################
# link together all pieces of our compiler
OBJECTS=build/ifccBaseVisitor.o \
	build/ifccLexer.o \
	build/ifccVisitor.o \
	build/ifccParser.o \
	build/IR.o \
	build/BasicBlock.o \
	build/CFG.o \
	build/IRVisitor.o \
	build/SymbolVisitor.o \
	build/backend.o 

ifcc: $(OBJECTS) build/main.o
	@mkdir -p build
	$(CC) $(LDFLAGS) build/*.o $(ANTLRLIB) -o ifcc

##########################################
# compile main file
build/main.o: $(MAIN_FILE) $(GENERATED_DIR)/ifccParser.cpp 
	@mkdir -p build
	$(CC) $(CCFLAGS) -MMD -o $@ $<

##########################################
# compile our hand-writen C++ code, CodeGenVisitor, etc.
build/%.o: $(SRC_DIR)/%.cpp $(GENERATED_DIR)/ifccParser.cpp
	@mkdir -p build
	$(CC) $(CCFLAGS) -MMD -o $@ $<


##########################################
# compile all the antlr-generated C++
build/%.o: $(GENERATED_DIR)/%.cpp
	@mkdir -p build
	$(CC) $(CCFLAGS) -MMD -o $@ $<

# automagic dependency management: `gcc -MMD` generates all the .d files for us
-include build/*.d
build/%.d:

##########################################
# generate the C++ implementation of our Lexer/Parser/Visitor
$(GENERATED_DIR)/ifccLexer.cpp: $(GENERATED_DIR)/ifccParser.cpp
$(GENERATED_DIR)/ifccVisitor.cpp: $(GENERATED_DIR)/ifccParser.cpp
$(GENERATED_DIR)/ifccBaseVisitor.cpp: $(GENERATED_DIR)/ifccParser.cpp
$(GENERATED_DIR)/ifccParser.cpp: ifcc.g4
	@mkdir -p $(GENERATED_DIR)
	java -jar $(ANTLRJAR) -visitor -no-listener -Dlanguage=Cpp -o $(GENERATED_DIR) ifcc.g4

# prevent automatic cleanup of "intermediate" files like ifccLexer.cpp etc
.PRECIOUS: $(GENERATED_DIR)/ifcc%.cpp

##########################################
# view the parse tree in a graphical window

# Usage: `make gui FILE=path/to/your/file.c`
FILE ?= ../tests/testfiles/1_return42.c

gui:
	@mkdir -p $(GENERATED_DIR) build
	java -jar $(ANTLRJAR) -Dlanguage=Java -o $(GENERATED_DIR) ifcc.g4
	javac -cp $(ANTLRJAR) -d build $(GENERATED_DIR)/*.java
	java -cp $(ANTLRJAR):build org.antlr.v4.gui.TestRig ifcc axiom -gui $(FILE)

tests:
	python3 ../testing_wrapper.py

##########################################
# delete all machine-$(GENERATED_DIR) files
clean:
	rm -rf build $(GENERATED_DIR)
	rm -f ifcc

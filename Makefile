CXX = g++
CXXFLAGS = -std=c++17 -Wall -Isrc

SRCDIR = src/
TESTDIR = tests/
UTILSDIR = utils/
BUILDDIR = build/
TMPDIR = tmp/

# Source files
SERVER_SRCS = $(SRCDIR)server.cpp $(SRCDIR)database.cpp
SERVER_OBJS = $(TMPDIR)server.o $(TMPDIR)database.o
MAIN_SRC = $(SRCDIR)main.cpp
MAIN_OBJ = $(TMPDIR)main.o

# Test files
TEST_DATABASE_SRC = $(TESTDIR)test_database.cpp
TEST_SERVER_SRC = $(TESTDIR)test_server.cpp
PERFORMANCE_TEST_SRC = $(TESTDIR)performance_test.cpp

TEST_DATABASE_OBJ = $(TMPDIR)test_database.o
TEST_SERVER_OBJ = $(TMPDIR)test_server.o
PERFORMANCE_TEST_OBJ = $(TMPDIR)performance_test.o

# Utility files
SST_TOOLS_SRC = $(UTILSDIR)sst_tools.cpp
SST_TOOLS_OBJ = $(TMPDIR)sst_tools.o
DB_CLI_SRC = $(UTILSDIR)db_cli.cpp
DB_CLI_OBJ = $(TMPDIR)db_cli.o

# All object files that might be generated in the root directory
ALL_OBJS = $(SERVER_OBJS) $(MAIN_OBJ) $(TEST_DATABASE_OBJ) $(TEST_SERVER_OBJ) $(PERFORMANCE_TEST_OBJ) $(SST_TOOLS_OBJ) $(DB_CLI_OBJ)

# Executables
TEST_DATABASE_EXEC = $(BUILDDIR)test_database
TEST_SERVER_EXEC = $(BUILDDIR)test_server
PERFORMANCE_TEST_EXEC = $(BUILDDIR)performance_test
SST_TOOLS_EXEC = $(BUILDDIR)sst_tools
DB_CLI_EXEC = $(BUILDDIR)db_cli
SERVER_EXEC = $(BUILDDIR)server

ALL_EXECS = $(TEST_DATABASE_EXEC) $(TEST_SERVER_EXEC) $(PERFORMANCE_TEST_EXEC) $(SST_TOOLS_EXEC) $(DB_CLI_EXEC) $(SERVER_EXEC)

.PHONY: all test sst_tools clean server db_cli

all: $(ALL_EXECS)

$(BUILDDIR)libserver.a: $(SERVER_OBJS) | $(BUILDDIR)
	ar rcs $@ $^

# Ensure build and tmp directories exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TMPDIR):
	mkdir -p $(TMPDIR)

# General rule for compiling source files in src/ to object files in tmp/
$(TMPDIR)%.o: $(SRCDIR)%.cpp | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# General rule for compiling test files in tests/ to object files in tmp/
$(TMPDIR)%.o: $(TESTDIR)%.cpp | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# General rule for compiling utility files in utils/ to object files in tmp/
$(TMPDIR)%.o: $(UTILSDIR)%.cpp | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_DATABASE_EXEC) $(TEST_SERVER_EXEC) $(PERFORMANCE_TEST_EXEC)
	./$(TEST_DATABASE_EXEC)
	./$(TEST_SERVER_EXEC)
	./$(PERFORMANCE_TEST_EXEC)

$(TEST_DATABASE_EXEC): $(TEST_DATABASE_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_SERVER_EXEC): $(TEST_SERVER_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

$(PERFORMANCE_TEST_EXEC): $(PERFORMANCE_TEST_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

sst_tools: $(SST_TOOLS_EXEC)

$(SST_TOOLS_EXEC): $(SST_TOOLS_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

server: $(SERVER_EXEC)

$(SERVER_EXEC): $(MAIN_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

db_cli: $(DB_CLI_EXEC)

$(DB_CLI_EXEC): $(DB_CLI_OBJ) $(BUILDDIR)libserver.a
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(BUILDDIR) $(TMPDIR) *.sst

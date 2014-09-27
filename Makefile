# Macro definitions
CXX = /usr/bin/g++
DOXYGEN = /usr/bin/doxygen

BASE_CXX_FLAGS = -ansi -pedantic -Wall -Wextra -Weffc++
DEBUG_CXX_FLAGS = -DDEBUG -g3
RELEASE_CXX_FLAGS = -O3
CXX_FLAGS = $(BASE_CXX_FLAGS) $(RELEASE_CXX_FLAGS)

DOC_DIR = doc
RM = /bin/rm -f

# make targets
.PHONY:  all doxygen

all: chat_server.exe chat_coordinator.exe chat_client.exe

chat_server.exe: chat_server.cc socket_utils.o
	$(CXX) $(CXX_FLAGS) -o chat_server.exe chat_server.cc socket_utils.o

chat_coordinator.exe: chat_coordinator.cc socket_utils.o
	$(CXX) $(CXX_FLAGS) -o chat_coordinator.exe chat_coordinator.cc socket_utils.o

chat_client.exe: chat_client.cc socket_utils.o
	$(CXX) $(CXX_FLAGS) -o chat_client.exe chat_client.cc socket_utils.o

socket_utils.o: socket_utils.h socket_utils.cc
	$(CXX) $(CXX_FLAGS) -c -o socket_utils.o socket_utils.cc

doxygen:
	@$(RM) -fr $(DOC_DIR)/doxygen
	@$(DOXYGEN) $(DOC_DIR)/Doxyfile

clean:
	@$(RM) chat_server.exe
	@$(RM) chat_coordinator.exe
	@$(RM) chat_client.exe
	@$(RM) socket_utils.o
	@$(RM) -fr $(DOC_DIR)/doxygen


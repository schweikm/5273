# Macro definitions
CXX = /usr/bin/g++
CXXFLAGS = -DDEBUG -ansi -pedantic -Wall -Wextra -Weffc++ -g3
RM = /bin/rm -f

# make targets
all: chat_server.exe chat_coordinator.exe chat_client.exe

chat_server.exe: chat_server.cc socket_utils.o
	$(CXX) $(CXXFLAGS) -o chat_server.exe chat_server.cc socket_utils.o

chat_coordinator.exe: chat_coordinator.cc socket_utils.o
	$(CXX) $(CXXFLAGS) -o chat_coordinator.exe chat_coordinator.cc socket_utils.o

chat_client.exe: chat_client.cc socket_utils.o
	$(CXX) $(CXXFLAGS) -o chat_client.exe chat_client.cc socket_utils.o

socket_utils.o: socket_utils.h socket_utils.cc
	$(CXX) $(CXXFLAGS) -c -o socket_utils.o socket_utils.cc

clean:
	@$(RM) chat_server.exe
	@$(RM) chat_coordinator.exe
	@$(RM) chat_client.exe
	@$(RM) socket_utils.o


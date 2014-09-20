# Macro definitions
CXX = /usr/bin/g++
CXXFLAGS = -DDEBUG -ansi -pedantic -Wall -Wextra -Weffc++ -g3
RM = /bin/rm -f

# make targets
all: chat_server chat_coordinator chat_client

chat_server: chat_server.cc
	$(CXX) $(CXXFLAGS) -o chat_server.exe chat_server.cc

chat_coordinator: chat_coordinator.cc
	$(CXX) $(CXXFLAGS) -o chat_coordinator.exe chat_coordinator.cc

chat_client: chat_client.cc
	$(CXX) $(CXXFLAGS) -o chat_client.exe chat_client.cc

clean:
	@$(RM) chat_server.exe
	@$(RM) chat_coordinator.exe
	@$(RM) chat_client.exe


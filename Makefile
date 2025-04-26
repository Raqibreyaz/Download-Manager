CXX = g++

CXXFLAGS = -Wall -Wextra -g -std=c++20

SSL_FLAGS = -lssl -lcrypto

SRCS = main.cpp \
		src/socket-lib/tcp-socket/tcp-socket.cpp \
		src/socket-lib/ssl-socket/ssl-socket.cpp \
		src/socket-lib/isocket/isocket.cpp \
		src/http-request/http-request.cpp \
		src/http-response/http-response.cpp \
		src/utils/utils.cpp 

# names of all the object files
BUILD_SRCS = $(SRCS:.cpp=.o)

EXEC = client

ARGS ?= http://example.com

$(EXEC): $(BUILD_SRCS)
			$(CXX) $(CXXFLAGS) -o $@ $^ $(SSL_FLAGS)

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c $< -o $@

clean: 
		rm -rf $(BUILD_SRCS) $(EXEC)

run:	$(EXEC)
		./$(EXEC) $(ARGS)
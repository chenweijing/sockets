TARGET := app
CXX := g++

#SRCS := $(wildcard *.cc)
SRCS := sockets.cc main.cc
OBJS := $(patsubst %.cc, %.o, $(SRCS))

LDFLAGS := -I. -pthread -std=c++11 -lglog

CXXFLAGS := -g -DNDEBUG -O2 


$(TARGET): $(OBJS) $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	$(RM) -f $(OBJS)
%.o:%.cc
	$(CXX) $(CXXFLAGS) -c $<  -o $@ $(LDFLAGS)

clean:
	$(RM) -rf $(TARGET)
	$(RM) -rf $(OBJS)



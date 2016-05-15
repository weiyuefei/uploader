#
# Copyright (C) SDMC Co. ltd.
# Author: feeman
#

CXX=g++

CXXFLAGS=

INCS:=-Isrc \
      -Ilibs/jsoncpp/include \
      -Ilibs/libcurl/include \
      -Ilibs/libev/include
CXXFLAGS += $(INCS)

LDFLAGS=-lpthread -lm
LIBS:= -Llibs/libev -lev \
      -Llibs/libcurl -lcurl \
      -Llibs/jsoncpp -ljson
LDFLAGS+=$(LIBS)

SRCDIRS=src

CPPFILES=$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

CXXOBJS=$(CPPFILES:%.cpp=%.o)
OBJS=$(CXXOBJS)
DEPS=$(OBJS:%.o=%.d)

TARGET=MediaFileUploader

all: $(TARGET)

%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM $< | sed -e 's/\(.*\)\.o\(.*\)/\1\.d \1\.o\2/g' >$@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o sbin/$@

-include $(DEPS)

clean:
	rm -rf $(SRCDIRS)/*.d $(SRCDIRS)/*.o sbin/$(TARGET)

.PHONY: all clean

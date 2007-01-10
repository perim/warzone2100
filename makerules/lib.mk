include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.cpp=%.o))

all: ../lib$(LIB).a
../lib$(LIB).a: $(DEPS) ; $(AR) rcv ../lib$(LIB).a $(DEPS)
clean: ; $(RMF) ..$(DIRSEP)lib$(LIB).a *.o


TARGET = EBOOT

OBJS = src/main.o

INCDIR =

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

LIBS = -lpspgu -lpspgum -lpspdebug -lpspdisplay -lpspkernel -lm

BUILD_PRX = 1
EXTRA_TARGETS = EBOOT.PBP

PSPSDK = $(shell psp-config --pspsdk-path)

include $(PSPSDK)/lib/build.mak

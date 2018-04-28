THIRD_PARTY_DIR = ../../cmocka
CMOCKA_INCLUDE_DIR = $(THIRD_PARTY_DIR)/build-Debug/include
CMOCKA_LIBRARY = $(THIRD_PARTY_DIR)/build-Debug/lib/libcmocka.a
CMOCKA_BUILD_DIR = ../../cmocka/build-Debug

#
# library
#
LIB_SRCS = message.c util.c
LIB_OBJS = $(LIB_SRCS:%.c=%.o)
LIB_INCLUDE_DIR = ../include
LIB_HEADERS = $(LIB_SRCS:%.c=%.h)
LIB = libutils.a

#
# main executable
#
EXE_SRCS = task_Command.c task_Logger.c task_MsgRouter.c task_RxSocket.c task_Tx.c main.c 
EXE_OBJS = $(EXE_SRCS:%.c=%.o)
EXE = main.out

#
# unit tests
#
UNITTEST_SRCS = unittest.c
UNITTEST_OBJS = $(UNITTEST_SRCS:%.c=%.o)
UNITTEST_EXE = test.out

#
# Compile options
#
CC = gcc
CFLAGS = -std=gnu11 -Wall -g -O0
CMOCKA_CFLAGS = $(CFLAGS) -I $(CMOCKA_INCLUDE_DIR)
UNITTEST_CFLAGS = $(CFLAGS) -L $(CMOCKA_LIBRARY) -I $(CMOCKA_INCLUDE_DIR) -I $(LIB_INCLUDE_DIR)
MAIN_CFLAGS = $(CFLAGS) -I $(LIB_INCLUDE_DIR)
UNITTEST_LFLAGS = -lcmocka -lrt
MAIN_LFLAGS = -pthread -lrt -lm
#
# Generic rule to generate objects
#
%.o : %.c
	$(CC) $(MAIN_CFLAGS) -c -o $@ $<

$(LIB) : $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(EXE) : $(EXE_OBJS) $(LIB)
	$(CC) $(MAIN_CFLAGS) -o $@ $^ $(MAIN_LFLAGS)

$(UNITTEST_EXE) : $(UNITTEST_SRCS) $(LIB)
	$(CC) $(UNITTEST_CFLAGS) -o $@ $^ $(UNITTEST_LFLAGS)
#
# build cmocka libraries
#
.PHONY : cmocka
cmocka :
		$(MAKE) --no-print-directory --directory=$(THIRD_PARTY_DIR) \
			CC=$(CC) CFLAGS="$(CMOCKA_CFLAGS)" $(MAKECMDGOALS);

.PHONY : all
all : $(LIB) $(EXE)
	# gcc -g socket_client.c api.c -I$(LIB_INCLUDE_DIR) -o client.out $(MAIN_LFLAGS)

.PHONY : test
test : $(UNITTEST_EXE)
	sudo ./$(UNITTEST_EXE)

.PHONY : clean
clean : FORCE
	-rm -rf *~ *.o  $(LIB) $(EXE) $(UNITTEST_EXE) *.dSYM/ *.out

.PHONY : cmocka_uninstall
cmocka_uninstall :
	-rm -rf $(CMOCKA_BUILD_DIR)

FORCE :

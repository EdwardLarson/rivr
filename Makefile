SRCDIR=src
CC=gcc
CFLAGS=-Wall

DEBUG=no
ifeq ($(DEBUG), yes)
CFLAGS += -D DEBUG -g
endif

PROFILER=no
ifeq ($(PROFILER), yes)
CFLAGS += -pg
endif

OPTIMIZE=no
ifeq ($(OPTIMIZE), yes)
CFLAGS += -O2
endif

AGGRESSIVE_OPT=no
ifeq ($(AGGRESSIVE_OPT), yes)
CFLAGS += -O3
endif

ODIR=obj

_DEPS = keywords.h opcodes.h parser.h rv_functions.h rv_strings.h rv_types.h vm.h version.h
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJ = keywords.o parser.o rivr.o rv_functions.o rv_strings.o vm.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_PW_DEPS = keywords.h opcodes.h rv_functions.h rv_strings.h rv_types.h vm.h version.h
PW_DEPS = $(patsubst %,$(SRCDIR)/%,$(_PW_DEPS))

_PW_OBJ = pw_prog_writer.o pw_vm.o pw_rv_strings.o pw_rv_functions.o
PW_OBJ = $(patsubst %,$(ODIR)/%,$(_PW_OBJ))


$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(ODIR)/pw_%.o: $(SRCDIR)/%.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

rivr: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -rf $(ODIR) *~ core $(INCDIR)/*~ 
	
rivr_d: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
prog_writer: $(PW_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
SRCDIR = src
OBJDIR = obj
BINDIR = bin

DIRS = $(OBJDIR) $(BINDIR)

LIBS = -lSDL -lSDL_draw -lSDL_image -lm -ljpeg

CC    = g++
CWARN = -W -Wall -Wshadow -Wreturn-type -Wformat -Wparentheses -Wpointer-arith -Wuninitialized -O
CDBG  = -g $(CWARN) -fno-inline

CFLAGS = -O3 #-fpack-struct
DFLAGS = -g $(CWARN)

TGT    = client
DBGTGT = dclient


OBJS = $(OBJDIR)/Surface.o \
       $(OBJDIR)/INI.o \
       $(OBJDIR)/main.o

DBGOBJS = $(OBJDIR)/Surface.dbg.o \
          $(OBJDIR)/INI.dbg.o \
	  $(OBJDIR)/main.dbg.o


all: $(TGT) $(SRCDIR)/*.h
dbg: $(DBGTGT)

 
$(TGT): $(DIRS) $(BINDIR)/$(TGT)
	cp $(BINDIR)/$(TGT) $(TGT)
	@echo "$@ up to date"

$(DBGTGT): $(DIRS) $(BINDIR)/$(DBGTGT)
	cp $(BINDIR)/$(DBGTGT) $(DBGTGT)
	@echo "$@ up to date"


$(BINDIR)/$(TGT): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $?


$(BINDIR)/$(DBGTGT): $(DBGOBJS)
	$(CC) $(CFLAGS) -o $@ $(DBGOBJS) $(LIBS)

$(OBJDIR)/%.dbg.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -DDEBUG=1 -c -o $@ $?

$(BINDIR):
	@mkdir $(BINDIR)

$(OBJDIR):
	@mkdir $(OBJDIR)

clean:
	rm -rf $(BINDIR)/*
	rm -rf $(OBJDIR)/*
	rm -rf $(TGT) $(DBGTGT)

fresh: clean all

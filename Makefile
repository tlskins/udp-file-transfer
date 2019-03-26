TARGETS = ncp rcv
LIBS =
CC = g++
CFLAGS = -g -Wall
OBJDIR = obj
CMNOBJ = $(OBJDIR)/rudp.o
NCPOBJS = $(OBJDIR)/ncp.o $(CMNOBJ)
RCVOBJS = $(OBJDIR)/rcv.o $(CMNOBJ)
.PHONY: default all clean

default: $(TARGETS)

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET)

ncp: $(NCPOBJS)
	$(CC) $(NCPOBJS) -g $(LIBS) -o $@

rcv: $(RCVOBJS)
	$(CC) $(RCVOBJS) -g $(LIBS) -o $@

clean:
	rm -rf $(OBJDIR)/*.o
	rm -f $(TARGETS)

run:
	@echo "run './rcv' in one terminal and run './ncp atestfile a @ u2' in another terminal"

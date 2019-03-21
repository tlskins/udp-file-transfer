TARGET = UDPServer UDPClient
LIBS =
CC = g++
CFLAGS = -g -Wall
OBJDIR = obj
.PHONY: default all clean

default: $(TARGET)

OBJECTS = $(OBJDIR)/UDPServer.o $(OBJDIR)/UDPClient.o $(OBJDIR)/rudp.o

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

#.PRECIOUS: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -g $(LIBS) -o $@

UDPServer: obj/UDPServer.o obj/rudp.o
	$(CC) $(OBJDIR)/UDPServer.o $(OBJDIR)/rudp.o -g $(LIBS) -o $@

UDPClient: obj/UDPClient.o obj/rudp.o
	$(CC) $(OBJDIR)/UDPClient.o $(OBJDIR)/rudp.o -g $(LIBS) -o $@

clean:
	rm -rf $(OBJDIR)/*.o
	rm -f $(TARGET)

TARGET = UDPServer UDPClient
LIBS =
CC = g++
CFLAGS = -g -Wall
OBJDIR = obj
.PHONY: default all clean

default: $(TARGET)

OBJECTS = $(OBJDIR)/UDPServer.o $(OBJDIR)/UDPClient.o

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

#.PRECIOUS: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -g $(LIBS) -o $@

UDPServer: obj/UDPServer.o
	$(CC) $(OBJDIR)/UDPServer.o -g $(LIBS) -o $@

UDPClient: obj/UDPClient.o
	$(CC) $(OBJDIR)/UDPClient.o -g $(LIBS) -o $@

clean:
	rm -rf $(OBJDIR)/*.o
	rm -f $(TARGET)

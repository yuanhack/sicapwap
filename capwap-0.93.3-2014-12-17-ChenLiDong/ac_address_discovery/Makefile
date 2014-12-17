TARGET=ads
OBJS=ac_address_server.o

all:$(TARGET)
.c.o:
	$(CC) -c -g $< -o $@
$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS)
clean:
	rm -rf $(OBJS) $(TARGET)

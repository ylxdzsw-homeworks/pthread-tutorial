DEBUG=1
CC=gcc
CFLAGS= -pthread -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O3
endif 

TARGET=hello return_stack_ptr show_stack show_tid detach
ALL: $(TARGET)

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET) *.s

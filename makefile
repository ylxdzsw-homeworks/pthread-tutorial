DEBUG=1
CC=gcc
CFLAGS= -pthread -Wall -lm

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O3
endif 

TARGET=hello return_stack_ptr show_stack show_tid detach kway_merge_sort 
ALL: $(TARGET)

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET) *.s

DEBUG=1
CC=gcc
CFLAGS= -pthread -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O3
endif 

TARGET=hello shared_data
ALL: $(TARGET)

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET) *.s

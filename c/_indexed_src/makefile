#DEBUG=0
CC=gcc
CFLAGS= -pthread -Wall -lm 

ifeq ($(DEBUG), 1)
	CFLAGS += -g 
else
	CFLAGS += -O3
endif 

TARGET=p13_show_stack p14_show_tid p24_hello p30_return_stack_ptr p36_kway_merge_sort p47_bind_affinity p48_vec_sum p51_shared_data p65_shared_data_mutex p69_bank p73_deadlock detach
ALL: $(TARGET)

$(TARGET): %: %.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o $(TARGET) *.s

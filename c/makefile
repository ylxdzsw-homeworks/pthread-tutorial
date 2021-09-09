DEBUG=1
CC=gcc
CFLAGS= -pthread -Wall -lm 

ifeq ($(DEBUG), 1)
	CFLAGS += -g 
else
	CFLAGS += -O3
endif 

# TARGET=hello return_stack_ptr show_stack show_tid detach kway_merge_sort bind_affinity vec_sum shared_data shared_data_mutex deadlock bank
TARGET=p13_show_stack p14_show_tid p24_hello p30_return_stack_ptr p36_kway_merge_sort p47_bind_affinity p48_vec_sum p51_shared_data p65_shared_data_mutex p69_bank p73_deadlock detach
ALL: $(TARGET)

$(TARGET): %: %.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o $(TARGET) *.s
	$(MAKE) -C exercise clean

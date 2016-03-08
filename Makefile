ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m += mp2.o
mp2-objs := ./src/mp2.o ./src/task.o ./src/dispatch.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

obj-m += KboardModule.o mod_proc.o

all : module app

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app:
	gcc SyncTest.c -o SyncTest -pthread

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean; rm SyncTest

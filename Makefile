objname = dhms_daemon
objects = dhms_daemon.o dhms_process.o parse.o cJSON.o
CC=/home/jess/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
CFLAG=-I/home/jess/cross-compile/zlog/include
LDFLAG=-L/home/jess/cross-compile/zlog/lib -lzlog
STRIP=/home/jess/arm-linux-gnueabi/bin/arm-linux-gnueabi-strip
define dhms_log
	@echo "\033[33m\033[1m\033[4m$(1)\033[0m"
endef
all:$(objects)
	$(call dhms_log,"start compile $(objname)")
	@$(CC) $(LDFLAG) $(CFLAG) -o $(objname) $(objects)
	@$(STRIP) $(objname)
	$(call dhms_log,"compile $(objname) sucess")
%.o:%.c
	@$(CC) -c $(CFLAG) $<

clean: 
	@rm *.o $(objname)

.PHONY:clean all

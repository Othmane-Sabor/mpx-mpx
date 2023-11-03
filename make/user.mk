.POSIX:

user/core.o: user/core.c include/string.h include/mpx/serial.h \
  include/mpx/device.h include/processes.h include/sys_req.h

user/interface.o: user/interface.c include/sys_req.h include/mpx/io.h include/string.h

user/pcb.o: user/pcb.c include/string.h include/pcb.h include/memory.h include/sys_req.h

USER_OBJECTS=\
	user/core.o \
	user/interface.o \
	user/pcb.o
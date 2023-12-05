CFLAGS:=-m32

CFLAGS+=-Qn
#CFLAGS+=-fno-builtin # no build in funcs
#CFLAGS+=-nostdinc  # no std include
CFLAGS+=-fno-pic    # no position independent code
#CFLAGS+=-fno-pie   # no position independent exec
#CFLAGS+=-nostdlib
#CFLAGS+=-fno-stack-protector
CFLAGS+=-mpreferred-stack-boundary=2 # 不需要栈对齐
CFLAGS+=-fomit-frame-pointer # 不需要栈对齐
CFLAGS+=-fno-asynchronous-unwind-tables
CFLAGS:=$(strip ${CFLAGS})

.PHONY: hello.s
hello.s: hello.c
	gcc $(CFLAGS) -S $< -o $@
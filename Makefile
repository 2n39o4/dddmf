#
# +) declare the path to the cross-compiler
#
#PATH=/home/neon/etc/Sourcery-G++/2010q1-188/bin
PATH=/home/neon/etc/CROSS/2010q1-188/bin

# +) define tools
#
AS=$(PATH)/arm-none-eabi-as
AR=$(PATH)/arm-none-eabi-ar
CC=$(PATH)/arm-none-eabi-gcc
OD=$(PATH)/arm-none-eabi-objdump
OC=$(PATH)/arm-none-eabi-objcopy
LD=$(PATH)/arm-none-eabi-ld


# +) list of objects needed to build target
#
OBJS  = main.o
OBJS += isr.o
OBJS += syslog.o
OBJS += init-uC.o
OBJS += dddmf.o
OBJS += lm3s6965-uart.o
OBJS += lm3s6965-clock.o
OBJS += lm3s6965-systick.o
OBJS += lm3s6965-gpio.o
OBJS += startup.o
OBJS += syscalls.o

#
# +) declare target (voodoo.axf)
#
TARGET = voodoo

#
# +) where do I keep StellarisWare ???
#
#STELLARISWARE=/home/neon/etc/TI/Luminary/StellarisWare/9453
STELLARISWARE=/home/neon/etc/StellarisWare/9453


#
# I have tried to implement a library to my liking, which is
# located at $(STELLARISWARE)/lm3s6965-API/liblm3s6965.c
#                                          -------------
# There is a Makefile in that directory to use if I change
# my library !!!!
#
# The idea behind my "library" is to hide all of the Stellaris 
# driver lib calls
#
#
#
#
# LIB_LM3S6965  = $(STELLARISWARE)/lm3s6965-API/liblm3s6965.o
LIB_LM3S6965 = $(STELLARISWARE)/driverlib/gcc-cm3/libdriver-cm3.a

#LINKER_SCRIPT = linker-script.ld
LINKER_SCRIPT = hello.ld

# +) declare compiler flags
#
CFLAGS += -mcpu=cortex-m3 -mthumb -g -O0
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -MD
CFLAGS += -std=c99
CFLAGS += -Wall
CFLAGS += -pedantic
CFLAGS += -DPART_LM3S6965


# +) declare linker flags
#
LDFLAGS  = -T $(LINKER_SCRIPT)
LDFLAGS += --entry ResetISR 
LDFLAGS += --gc-sections
LDFLAGS += -g

LIBC = /home/neon/etc/Sourcery-G++/2010q1-188/arm-none-eabi/lib/thumb

#LDFLAGS += -Wl,-nostdlib
#LDFLAGS += -Wl,--cref
#LDFLAGS += -Wl,--stats

$(TARGET).axf : $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIB_LM3S6965) -o $@
	$(OC) -O binary $(TARGET).axf $(TARGET).bin


bob.axf : $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIB_LM3S6965) -o $@ 

### -L$(LIBC) -lc <= this seems to find libc BUT setjmp crashes my uC

## $(TARGET).axf : $(OBJS)
## 	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIB_LM3S6965) -o $@
## 	$(OC) -O binary $(TARGET).axf $(TARGET).bin

startup.o : startup.c
	$(CC) $(CFLAGS) -o $@ -c $<

isr.o : isr.c
	$(CC) $(CFLAGS) -o $@ -c $<

main.o :  main.c
	$(CC) $(CFLAGS) -o $@ -c $<

syslog.o : syslog.c
	$(CC) $(CFLAGS) -o $@ -c $<

liblm3s6965.o : liblm3s6965.c
	$(CC) $(CFLAGS) -o $@ -c $< -I/home/neon/etc/TI/Luminary/StellarisWare/9453

lm3s6965-uart.o : lm3s6965-uart.c lm3s6965-uart.h
	$(CC) $(CFLAGS) -o $@ -c $< -I/home/neon/etc/TI/Luminary/StellarisWare/9453

lm3s6965-clock.o : lm3s6965-clock.c
	$(CC) $(CFLAGS) -o $@ -c $< -I/home/neon/etc/TI/Luminary/StellarisWare/9453

lm3s6965-systick.o : lm3s6965-systick.c
	$(CC) $(CFLAGS) -o $@ -c $< -I/home/neon/etc/TI/Luminary/StellarisWare/9453

lm3s6965-gpio.o : lm3s6965-gpio.c
	$(CC) $(CFLAGS) -o $@ -c $< -I/home/neon/etc/TI/Luminary/StellarisWare/9453

init-uC.o : init-uC.c
	$(CC) $(CFLAGS) -o $@ -c $< 

#-I $(STELLARISWARE)/lm3s6965-API
#                                      -----------------------------
#                                                  |
#   this should be a directory, not a file --------+

tasks.o : tasks.c
	$(CC) $(CFLAGS) -o $@ -c $< -I $(DDD_COOP_TASKS) 

console-task.o : console-task.c
	$(CC) $(CFLAGS) -o $@ -c $<

console-idle-task.o : console-idle-task.c
	$(CC) $(CFLAGS) -o $@ -c $<

forth.o : forth.s
	$(CC) $(CFLAGS) -o $@ -c $<

arm-forth.o : arm-forth.s
	$(CC) $(CFLAGS) -o $@ -c $<

uptime.o : uptime.c
	$(CC) $(CFLAGS) -o $@ -c $<

syscalls.o : syscalls.c
	$(CC) $(CFLAGS) -o $@ -c $<

help:
	@echo "OBJS = $(OBJS)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LINKER_SCRIPT = $(LINKER_SCRIPT)"


# NOTES:

#DDD_COOP_TASKS = /home/neon/etc/DDD-COOP-TASKS/
# by commenting out DDD_COOP_TASKS this make file 
# looks for `coop-tasks.o' in the project tree

# !!! I prefer to have ALL the source files in the
#     project tree !!!

#OBJS += liblm3s6965.o

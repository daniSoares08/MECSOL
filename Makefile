NAME = MECSOL
DESCRIPTION = "Viga+Secao (demo CE C)"
COMPRESSED = YES
ARCHIVED = YES

SRC = src/main.c

CFLAGS = -Wall -Wextra -Oz
LDFLAGS = -lgraphx -lkeypadc -ltice -lm

include $(shell cedev-config --makefile)
NAME = MECSOL
DESCRIPTION = "Viga+Secao (demo CE C)"
COMPRESSED = YES
ARCHIVED = YES

# All source files shipped with the project.
SRC = src/main.c src/centroid.c src/viga.c src/beam.c src/tensoes.c

CFLAGS = -Wall -Wextra -Oz
LDFLAGS = -lgraphx -lkeypadc -ltice -lm

include $(shell cedev-config --makefile)
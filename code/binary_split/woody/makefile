LCRCC=gcc
LCRCFLAGS=-O3 -Wunused-variable -Wno-unused-result -Wno-discarded-qualifiers 
GDAL_INCLUDE_PATH=${EBROOTGDAL}/include
GDAL_LIB_PATH=${EBROOTGDAL}/lib

CFLAGS = -I$(GDAL_INCLUDE_PATH) $(LCRCFLAGS)
GDAL   = -L$(GDAL_LIB_PATH) -lgdal -lm
CC     = $(LCRCC)

OBJS = woody.o

TARGETS = woody

#
# Rules for making object files from .c files.
#
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

all: woody

woody.o: woody.c makefile

woody: $(OBJS)
	$(CC) $(OBJS) $(GDAL_LINK_FLAGS) $(GDAL) -o $@

clean:
	rm -f *.o woody

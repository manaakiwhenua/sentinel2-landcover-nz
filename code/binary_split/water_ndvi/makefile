LCRCC=gcc
LCRCFLAGS=-O3 -Wunused-variable -Wno-unused-result -Wno-discarded-qualifiers 
GDAL_INCLUDE_PATH=${EBROOTGDAL}/include
GDAL_LIB_PATH=${EBROOTGDAL}/lib

CFLAGS = -I$(GDAL_INCLUDE_PATH) $(LCRCFLAGS)
GDAL   = -L$(GDAL_LIB_PATH) -lgdal -lm
CC     = $(LCRCC)

OBJS = water_ndvi.o

TARGETS = water_ndvi

#
# Rules for making object files from .c files.
#
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

all: water_ndvi

water_ndvi.o: water_ndvi.c makefile

water_ndvi: $(OBJS)
	$(CC) $(OBJS) $(GDAL_LINK_FLAGS) $(GDAL) -o $@

clean:
	rm -f *.o water_ndvi

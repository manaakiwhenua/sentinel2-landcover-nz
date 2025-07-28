#!/usr/bin/env python

import sys
import numpy
from rios import rat

# Constants for coding cloud, shadow, snow and null in the masks. 
UNDEFINED = 0
WATER = 1
BARE_GROUND= 2
WOODY = 3
HERBACEOUS = 4
CLOUD = 5
PRIMARILY_BARE = 6
SNOW = 7
GLACIAL = 8
UNSPECIFIED_WOODY = 9
NARROWLEAVED_SCRUB = 10
EXOTIC_FOREST = 11
DECIDUOUS_HARDWOODS = 12
BROADLEAVED_SHRUB = 13
CROPLANDS = 14
WETLANDS = 15
ORCHARDS_VINEYARDS = 16

if len(sys.argv) != 2:
    print("usage: infile")
    sys.exit()

clrTbl = numpy.array([
    [UNDEFINED, 0, 0, 0, 255],
    [WATER, 53, 158, 201, 255],
    [BARE_GROUND, 211, 211, 211, 255],
    [WOODY, 0, 100, 0, 255],
    [HERBACEOUS, 246, 249, 158, 255],
    [CLOUD, 255, 0, 0, 255],
    [PRIMARILY_BARE, 209, 179, 140, 255],
    [SNOW, 255, 0, 255, 255],
    [GLACIAL, 90, 178, 255, 255],
    [UNSPECIFIED_WOODY, 102, 102, 102, 255],
    [NARROWLEAVED_SCRUB, 112, 156, 99, 255],    
    [EXOTIC_FOREST, 171, 41, 31, 255],    
    [DECIDUOUS_HARDWOODS, 255, 166, 0, 255],    
    [BROADLEAVED_SHRUB, 255, 128, 161, 255],    
    [CROPLANDS, 130, 74, 179, 255],
    [WETLANDS, 145, 212, 145, 255],
    [ORCHARDS_VINEYARDS, 180, 74, 179, 255]    
])
    
rat.setColorTable(sys.argv[1], clrTbl)

className = numpy.empty([256], dtype=numpy.dtype('a255'))
className[...] = ""
className[UNDEFINED] = "Undefined"
className[WATER] = "Water"
className[BARE_GROUND] = "Bare Ground"
className[WOODY] = "Woody Vegetation"
className[HERBACEOUS] = "Herbaceous Vegetation"        
className[CLOUD] = "Cloud"        
className[PRIMARILY_BARE] = "Primarily Bare Ground"        
className[SNOW] = "Snow"        
className[GLACIAL] = "Glacial Lakes, Wet Rock, Water/Sediment"        
className[UNSPECIFIED_WOODY] = "Unspecified Woody Vegetation"        
className[NARROWLEAVED_SCRUB] = "Narrow-leaved Scrub"        
className[EXOTIC_FOREST] = "Exotic Forest"        
className[DECIDUOUS_HARDWOODS] = "Deciduous Hardwoods"        
className[BROADLEAVED_SHRUB] = "Broadleaved Shrub"        
className[CROPLANDS] = "Croplands"        
className[WETLANDS] = "Wetlands"        
className[ORCHARDS_VINEYARDS] = "Orchards and Vineyards"        

rat.writeColumn(sys.argv[1], "Class", className)

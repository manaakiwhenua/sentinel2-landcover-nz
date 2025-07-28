#!/usr/bin/env python
"""
Classify snow from surace reflectance data using DEM to deal with breaking surf
"""
import sys
import numpy
from rios import applier
from rios import rat
from rios import cuiprogress
from osgeo import gdal

UNDEFINED = 0
SNOW = 1
BRIGHT_COASTAL_WATER = 2
OTHER = 3

if len(sys.argv) != 5:
    print("usage: flats dem mask snow")
    sys.exit()

# Set up input filename 
infiles = applier.FilenameAssociations()
infiles.flats = sys.argv[1]
infiles.dem   = sys.argv[2]
infiles.mask  = sys.argv[3]

# Set up output filename
outfiles = applier.FilenameAssociations()
outfiles.snow = sys.argv[4]

controls = applier.ApplierControls()
controls.referenceImage = infiles.flats
controls.thematic = True
controls.drivername='KEA'
controls.calcStats = True
controls.progress = cuiprogress.CUIProgressBar()

# Define the applier function
def recodeValid(info, inputs, outputs):

    snow = numpy.zeros_like(inputs.mask[0],dtype=numpy.uint8)

    blue  = inputs.flats[0].astype(numpy.float)/1000.0
    green = inputs.flats[1].astype(numpy.float)/1000.0
    red   = inputs.flats[2].astype(numpy.float)/1000.0
    nir   = inputs.flats[6].astype(numpy.float)/1000.0
    swir  = inputs.flats[8].astype(numpy.float)/1000.0
    
    vsbar = (blue + green + red)/3;

    vdem = inputs.dem[0]

    snow[(inputs.mask[0] > 0) & (vsbar > 0)] = OTHER

    vsbar[vsbar <= 0] = 0.0001
    
    # snow test
    snow[(snow == OTHER) & (red > 0.25) & ((nir - swir) > 0.2) & (swir < 0.10)] = SNOW

    # unlikely to ever have snow below 150 metres - probably heavy surf
    snow[(snow == SNOW) & (vdem < 150)] = BRIGHT_COASTAL_WATER

    # turbid estuary water and surf (low alt, bright visible with a large drop from vis to swir)
    snow[(snow == OTHER) & (vdem < 50) & (vsbar > 0.10) & ((vsbar - swir)/vsbar > 0.6)] = BRIGHT_COASTAL_WATER
    
    # Expand the output array to include a single
    # image band and set as the output dataset.
    outputs.snow = numpy.expand_dims(snow, axis=0)


# A function to add a colour table.
def addColourTable(imageFile):
    # Create a colour table (n,5) where
    # n is the number of classes to be 
    # coloured. The data type must be
    # of type integer.
    ct = numpy.zeros([4,5], dtype=numpy.int)
    
    # Set 0 to be black.
    ct[0][0] = 0   # Pixel Val
    ct[0][1] = 0   # Red
    ct[0][2] = 0   # Green
    ct[0][3] = 0   # Blue
    ct[0][4] = 255 # Opacity
    
    # Set 1 to be magenta
    ct[1][0] = 1   # Pixel Val
    ct[1][1] = 255 # Red
    ct[1][2] = 0   # Green
    ct[1][3] = 255 # Blue
    ct[1][4] = 255 # Opacity
    
    # Set 2 to be purple
    ct[2][0] = 2   # Pixel Val
    ct[2][1] = 160 # Red
    ct[2][2] = 32  # Green
    ct[2][3] = 240 # Blue
    ct[2][4] = 255 # Opacity
    
    # Set 3 to be grey
    ct[3][0] = 3   # Pixel Val
    ct[3][1] = 211 # Red
    ct[3][2] = 211 # Green
    ct[3][3] = 211 # Blue
    ct[3][4] = 255 # Opacity
        
    rat.setColorTable(imageFile, ct)

def addClassNames(imageFile):
    histo = rat.readColumn(imageFile, "Histogram")
    className = numpy.empty_like(histo, dtype=numpy.dtype('a255'))
    className[...] = ""
    className[0] = "Undefined"
    className[1] = "Snow"
    className[2] = "Bright Coastal Water"
    className[3] = "Other"
    # Write the output column to the file.
    rat.writeColumn(imageFile, "Class", className)


# Apply the recodeRules function.
applier.apply(recodeValid, infiles, outfiles, controls=controls)

# Run the add colour table function
addColourTable(outfiles.snow)

# Run the add colour table function
addClassNames(outfiles.snow)

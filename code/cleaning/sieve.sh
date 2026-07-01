#!/bin/bash
# Iterative sieve filtering to remove small isolated patches from a classified raster.
#
# Sieve thresholds escalate across passes (2→3→5→7 pixels) using 8-connectivity.
# A mask is applied throughout to prevent land-cover classes from bleeding into
# water/nodata areas. Each pass is followed by LZW compression before the next.
#
# The final pass (st=5, no mask) intentionally omits the mask so that small
# isolated water patches — which were protected in all prior passes — can now
# be removed. This avoids land-cover bleed in the intermediate steps while
# still cleaning up fragmented water at the end.
#
# Input:  nz_2324_blc.tif          (classified raster)
#         nz_2324_blc-mask.tif     (mask: nodata/water areas to protect)
# Output: nz_2324_blc_st2-st3-st5-st7-water_st5-compressed.tif

BASE=/media/lawr/blue-transcend
INPUT=$BASE/nz_2324_blc.tif
MASK=$BASE/nz_2324_blc-mask.tif

set -euo pipefail

sieve_and_compress() {
    local input=$1 output_raw=$2 output_compressed=$3 threshold=$4
    shift 4
    gdal_sieve -st "$threshold" -8 "$input" "$@" -of GTiff "$output_raw"
    gdal_translate -of GTiff -co "COMPRESS=LZW" -co "TILED=YES" "$output_raw" "$output_compressed"
    rm "$output_raw"
}

# Pass 1: remove patches < 2px (with mask)
sieve_and_compress \
    "$INPUT" \
    "$BASE/nz_2324_blc_st2.tif" \
    "$BASE/nz_2324_blc_st2-compressed.tif" \
    2 -mask "$MASK"

# Pass 2: remove patches < 3px (with mask)
sieve_and_compress \
    "$BASE/nz_2324_blc_st2-compressed.tif" \
    "$BASE/nz_2324_blc_st2-st3.tif" \
    "$BASE/nz_2324_blc_st2-st3-compressed.tif" \
    3 -mask "$MASK"

# Pass 3: remove patches < 5px (with mask)
sieve_and_compress \
    "$BASE/nz_2324_blc_st2-st3-compressed.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5-compressed.tif" \
    5 -mask "$MASK"

# Pass 4: remove patches < 7px (with mask)
sieve_and_compress \
    "$BASE/nz_2324_blc_st2-st3-st5.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5-st7.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5-st7-compressed.tif" \
    7 -mask "$MASK"

# Pass 5: remove small water patches < 5px (NO mask — intentional)
# Water was protected in all prior passes to prevent land-cover bleed.
# This final unmasked pass cleans up residual fragmented water.
sieve_and_compress \
    "$BASE/nz_2324_blc_st2-st3-st5-st7-compressed.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5-st7-water_st5.tif" \
    "$BASE/nz_2324_blc_st2-st3-st5-st7-water_st5-compressed.tif" \
    5

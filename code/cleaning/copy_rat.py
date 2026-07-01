from osgeo import gdal

gdal.UseExceptions()

tif = "./output/nz_2324_blc_16bit.tif"   # <-- edit
ds = gdal.Open(tif, gdal.GA_Update)
b = ds.GetRasterBand(1)

# Build a thematic RAT with your fields
rat = gdal.RasterAttributeTable()
rat.CreateColumn("Histogram", gdal.GFT_Integer, gdal.GFU_PixelCount)
rat.CreateColumn("Red",       gdal.GFT_Integer, gdal.GFU_Red)
rat.CreateColumn("Green",     gdal.GFT_Integer, gdal.GFU_Green)
rat.CreateColumn("Blue",      gdal.GFT_Integer, gdal.GFU_Blue)
rat.CreateColumn("Alpha",     gdal.GFT_Integer, gdal.GFU_Alpha)
rat.CreateColumn("Class",     gdal.GFT_String,  gdal.GFU_Name)

# Define rows 0..16 only (this is what drops the unused values)
rat.SetRowCount(17)

# value: (hist, R, G, B, A, class_name)
rows = {
  0:  (0,          0,   0,   0, 255, "Undefined"),
  1:  (122826829, 53, 158, 201, 255, "Water"),
  2:  (71363335,  211,211,211, 255, "Bare Ground"),
  3:  (691858296, 0,  100,  0, 255, "Indigenous Forest"),
  4:  (1078408060,246,249,158,255, "Herbaceous Vegetation"),
  5:  (0,         255,  0,  0, 255, "Cloud (N/A)"),
  6:  (294271194, 209,179,140,255, "Primarily Bare Ground"),
  7:  (9696198,   255,  0,255,255, "Snow"),
  8:  (39195,      90,178,255,255, "Glacial Lakes, Wet Rock, Water/Sediment"),
  9:  (31317295,  102,102,102,255, "Unspecified Woody Vegetation"),
  10: (148578011, 112,156, 99,255, "Narrow-leaved Scrub"),
  11: (164926579, 171, 41, 31,255, "Exotic Forest (DL)"),
  12: (4727640,   255,166,  0,255, "Deciduous Hardwoods (DL)"),
  13: (94208125,  255,128,161,255, "Broadleaved Shrub"),
  14: (19337359,  130, 74,179,255, "Cropland (Temporal NDVI 2020-2024)"),
  15: (14767565,  145,212,145,255, "Wetland (LCDB)"),
  16: (10509375,  180, 74,179,255, "Orchards and Vineyards (LCDB)"),
}

# Fill rows
for i in range(17):
    hist, r, g, bb, a, name = rows[i]
    rat.SetValueAsInt(i, 0, int(hist))
    rat.SetValueAsInt(i, 1, int(r))
    rat.SetValueAsInt(i, 2, int(g))
    rat.SetValueAsInt(i, 3, int(bb))
    rat.SetValueAsInt(i, 4, int(a))
    rat.SetValueAsString(i, 5, name)

b.SetDefaultRAT(rat)
b.FlushCache()
ds.FlushCache()

print("Wrote RAT rows 0..16")


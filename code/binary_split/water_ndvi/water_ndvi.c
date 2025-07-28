#include "gdal.h"

#define outerr stderr, __FILE__, __LINE__

typedef enum
{
    EIMG_THEMATIC_LAYER = 0,
    EIMG_ATHEMATIC_LAYER,
    EIMG_REALFFT_LAYER
}
Eimg_LayerType;

typedef struct
{
  double minimum; /* Minimum value */
  double maximum; /* Maximum value */
  double mean;    /* Mean value */
  double median;  /* Median value */
  double mode;    /* Mode value */
  double stddev;  /* Standard deviation */
}
Esta_Statistics;

typedef struct
{
  double x; /* coordinate x-value */
  double y; /* coordinate y-value */
}
Eprj_Coordinate;

typedef struct
{
  double width;  /* pixelsize width */
  double height; /* pixelsize height */
}
Eprj_Size;

typedef struct {
  char *proName;                      /* projection name */
  Eprj_Coordinate *upperLeftCenter;   /* map coordinates of center of upper left pixel */
  Eprj_Coordinate *lowerRightCenter;  /* map coordinates of center of lower right pixel */
  Eprj_Size *pixelSize;               /* pixel size in map units */
  char *units;                        /* units of the map */
}
Eprj_MapInfo;

#define pGlayer(glayer,x,y) ((char*)glayer->line[y] + (x)*glayer->nbpp)
#define Glayer(glayer,type,x,y) (*(type *)pGlayer(glayer,x,y))

typedef struct     /* imagery layer */
{
  GDALDatasetH    hDataset;
  GDALRasterBandH hBand;

  Eprj_MapInfo *info;

  char *pszProjection;
  char **papszMetadata;

  GDALDataType pixeltype;

  Eimg_LayerType layertype;

  Esta_Statistics *stats; 

  void *data;   /* pointer to actual image data */

  void **line;  /* pointers to start of image lines */

  long width;   /* image width */
  long height;  /* image height */

  int blockwidth;
  int blockheight;

  /* Raster Colortable */

  GDALColorTableH hCT;

  /* Raster Attribute Table */

  GDALRasterAttributeTableH hRAT;
  
  /* Map information */

  double GeoTransform[6];

  /* GeoTransform - coefficients for transforming between pixel/line (i,j) raster space,
   * and projection coordinates (E,N) space
   *
   * GeoTransform[0]  top left x 
   * GeoTransform[1]  w-e pixel resolution nfo->proName.data
   * GeoTransform[2]  rotation, 0 if image is "north up" 
   * GeoTransform[3]  top left y 
   * GeoTransform[4]  rotation, 0 if image is "north up" 
   * GeoTransform[5]  n-s pixel resolution 
   *
   * E = GeoTransform[0] + i*GeoTransform[1] + j*GeoTransform[2];
   * N = GeoTransform[3] + i*GeoTransform[4] + j*GeoTransform[5];
   * 
   * In a north up image, GeoTransform[1] is the pixel width, and GeoTransform[5] is the pixel height.
   * The upper left corner of the upper left pixel is at position (GeoTransform[0],GeoTransform[3]).
   */

  int nbpp;     /* number of bytes per pixel  */
  int nbpl;     /* number of bytes per line */

  double pw;    /* pixel width  (map units) */
  double ph;    /* pixel height             */
  double pa;    /* pixel area               */
}
Glayer;

#define UNDEFINED  (0)
#define WATER      (1)
#define BARE       (2)
#define WOODY      (3)
#define HERBACEOUS (4)
#define CLOUD      (5)
#define PRIM_BARE  (6)
#define SNOW       (7)
#define GLACIAL    (8) /* Glacial lakes, wet rock, water with high sediment */
#define LAND       (9)

#define LANDSAT     (1) /* LANDSAT 4/5/7 imagery (6 bands) */
#define SPOT        (2) /* SPOT 4/5 imagery (4 bands)      */
#define LANDSAT8    (3) /* LANDSAT 8 imagery (7 bands)     */
#define SENTINEL2   (4) /* SENTINEL 2 imagery (10 bands)   */

#define HRG_TO_ETM_GREEN  (1.086)
#define HRG_TO_ETM_RED    (0.926)
#define HRG_TO_ETM_NIR    (0.998)
#define HRG_TO_ETM_SWIR   (0.983) 

/* Values taken from Flood 2017 */

/* ETM */

#define MSI_TO_ETM_BLUE_C0   (-0.0022)
#define MSI_TO_ETM_BLUE_C1   (0.9551)

#define MSI_TO_ETM_GREEN_C0  (0.0031)
#define MSI_TO_ETM_GREEN_C1  (1.0582)

#define MSI_TO_ETM_RED_C0    (0.0064)
#define MSI_TO_ETM_RED_C1    (0.9871)

#define MSI_TO_ETM_NIR_C0    (0.0120)
#define MSI_TO_ETM_NIR_C1    (1.0187)

#define MSI_TO_ETM_SWIR_C0   (0.0079)
#define MSI_TO_ETM_SWIR_C1   (0.9528)

#define MSI_TO_ETM_SWIR2_C0  (-0.0042)
#define MSI_TO_ETM_SWIR2_C1  (0.9688)

/* OLI */

#define MSI_TO_OLI_BLUE_C0   (-0.0012)
#define MSI_TO_OLI_BLUE_C1   (0.9630)

#define MSI_TO_OLI_GREEN_C0  (0.0013)
#define MSI_TO_OLI_GREEN_C1  (1.0473)

#define MSI_TO_OLI_RED_C0    (0.0027)
#define MSI_TO_OLI_RED_C1    (0.9895)

#define MSI_TO_OLI_NIR_C0    (0.0147)
#define MSI_TO_OLI_NIR_C1    (1.0129)

#define MSI_TO_OLI_SWIR_C0   (0.0025)
#define MSI_TO_OLI_SWIR_C1   (0.9626)

#define MSI_TO_OLI_SWIR2_C0  (-0.0011)
#define MSI_TO_OLI_SWIR2_C1  (0.9392)

/* basic premise raw NDVI good at finding water or bare ground, water if dark */

int main(int argc, char **argv);

Glayer *
GlayerReadWholeLayer(char *fname, int band);

void CreateRasterFile(char *fname, int bands, int width, int height, GDALDataType pixeltype, double *GeoTransform, char *pszProjection);
void GlayerWriteWholeLayer(Glayer *glayer, char *fname, int band);
void SetThematic(char *fname);
void AddClassColumn(char *fname);
void BuildOverviews(char *fname, char *aggregationType);

int GetRATColumnIndex(GDALRasterAttributeTableH hRAT, char *cname);

void errexit(FILE *fp, char *file, int line, char *errstr, ...);
void jobstate(char *fmt, ...);
void jobprog(int percent);

int GdalGetNumberOfBands(char *fname);
void GlayerCheckType(Glayer *glayer, GDALDataType pixeltype);

char *
fextn(char *fname);

Glayer *
GlayerCreate(long width, long height, GDALDataType pixeltype, Eprj_MapInfo *info, char *pszProjection);
int setnbpp(GDALDataType pixeltype);

Eprj_MapInfo *
GeoTransformToMapInfo(Eprj_MapInfo *info, double *GeoTransform, long width, long height);

double *
MapInfoToGeoTransform(double *GeoTransform, Eprj_MapInfo *info);

Eprj_MapInfo *
CreateMapInfo();

Eprj_MapInfo *
CopyMapInfo(Eprj_MapInfo *dest, Eprj_MapInfo *src);

int
main(int argc, char **argv)
{
  Glayer *sr_blue;  /* standardised reflectance - blue  band */
  Glayer *sr_green; /* standardised reflectance - green band */
  Glayer *sr_red;   /* standardised reflectance - red   band */
  Glayer *sr_nir;   /* standardised reflectance - NIR   band */
  Glayer *sr_swir;  /* standardised reflectance - SWIR  band */
  Glayer *sr_swir2; /* standardised reflectance - SWIR2 band */

  Glayer *cloud;  /* cloud mask */
  Glayer *snow;   /* snow bright water mask */
  Glayer *water;
  int nbands;

  int i,j;
  double percent_per_line;
  double percent;

  double blue,green,red,nir,swir,swir2;
  int    vcloud;
  int    vsnow;
  double vsbar;
  double irbar;
  double ndvi;
  int sensor;
  
  if(argc != 5)
    errexit(outerr,"expecting [flats] [cloud] [snow] [water]");

   /* Register GDAL */
  GDALAllRegister();
 
  jobstate("Opening %s...", argv[1]);

  nbands = GdalGetNumberOfBands(argv[1]);

  if(nbands == 6)
    {
      sensor = LANDSAT;
      
      jobstate("Sensor: LANDSAT");

      sr_blue  = GlayerReadWholeLayer(argv[1], 1);
      GlayerCheckType(sr_blue, GDT_UInt16);
      
      sr_green = GlayerReadWholeLayer(argv[1], 2);
      GlayerCheckType(sr_green, GDT_UInt16);
      
      sr_red   = GlayerReadWholeLayer(argv[1], 3);
      GlayerCheckType(sr_red, GDT_UInt16);
      
      sr_nir   = GlayerReadWholeLayer(argv[1], 4);
      GlayerCheckType(sr_nir, GDT_UInt16);

      sr_swir  = GlayerReadWholeLayer(argv[1], 5);
      GlayerCheckType(sr_swir, GDT_UInt16);

      sr_swir2 = GlayerReadWholeLayer(argv[1], 6);
      GlayerCheckType(sr_swir2, GDT_UInt16);
    }
  else if(nbands == 4)
    {
      sensor = SPOT;

      jobstate("Sensor: SPOT");

      sr_green = GlayerReadWholeLayer(argv[1], 1);
      GlayerCheckType(sr_green, GDT_UInt16);
      
      sr_red   = GlayerReadWholeLayer(argv[1], 2);
      GlayerCheckType(sr_red, GDT_UInt16);
      
      sr_nir   = GlayerReadWholeLayer(argv[1], 3);
      GlayerCheckType(sr_nir, GDT_UInt16);

      sr_swir  = GlayerReadWholeLayer(argv[1], 4);
      GlayerCheckType(sr_swir, GDT_UInt16);

      sr_swir2 = NULL;
    }
  else if(nbands == 7)
    {
      sensor = LANDSAT8;

      jobstate("Sensor: LANDSAT8");

      sr_blue  = GlayerReadWholeLayer(argv[1], 2);
      GlayerCheckType(sr_blue, GDT_UInt16);
      
      sr_green = GlayerReadWholeLayer(argv[1], 3);
      GlayerCheckType(sr_green, GDT_UInt16);
      
      sr_red   = GlayerReadWholeLayer(argv[1], 4);
      GlayerCheckType(sr_red, GDT_UInt16);
      
      sr_nir   = GlayerReadWholeLayer(argv[1], 5);
      GlayerCheckType(sr_nir, GDT_UInt16);

      sr_swir  = GlayerReadWholeLayer(argv[1], 6);
      GlayerCheckType(sr_swir, GDT_UInt16);

      sr_swir2 = GlayerReadWholeLayer(argv[1], 7);
      GlayerCheckType(sr_swir2, GDT_UInt16);
    }
  else if(nbands == 10)
    {
      sensor = SENTINEL2;

      jobstate("Sensor: SENTINEL2");

      sr_blue  = GlayerReadWholeLayer(argv[1], 1);
      GlayerCheckType(sr_blue, GDT_UInt16);
      
      sr_green = GlayerReadWholeLayer(argv[1], 2);
      GlayerCheckType(sr_green, GDT_UInt16);
      
      sr_red   = GlayerReadWholeLayer(argv[1], 3);
      GlayerCheckType(sr_red, GDT_UInt16);
      
      sr_nir   = GlayerReadWholeLayer(argv[1], 7);
      GlayerCheckType(sr_nir, GDT_UInt16);

      sr_swir  = GlayerReadWholeLayer(argv[1], 9);
      GlayerCheckType(sr_swir, GDT_UInt16);

      sr_swir2 = GlayerReadWholeLayer(argv[1], 10);
      GlayerCheckType(sr_swir2, GDT_UInt16);
    }
  else
    errexit(outerr,"expecting 4,6,7 or 10 band imagery (SPOT/LANDSAT/SENTINEL)");

  jobstate("Opening %s...", argv[2]);
  
  cloud = GlayerReadWholeLayer(argv[2],1);

  GlayerCheckType(cloud, GDT_Byte);
    
  jobstate("Opening %s...", argv[3]);
  
  snow = GlayerReadWholeLayer(argv[3],1);

  GlayerCheckType(snow, GDT_Byte);

  water = GlayerCreate(snow->width, snow->height, GDT_Byte, snow->info, snow->pszProjection);

  percent_per_line = 100.0/water->height;
  percent = 0.0;

  jobstate("Classifying Water..."); 
  jobprog(0);

  for(j = 0; j < water->height; j++)
    {
      if(floor(j*percent_per_line) > percent)
	{
	  percent = j*percent_per_line;
	  jobprog(percent);
	}

      for(i = 0; i < water->width; i++)
	{
	  vcloud = Glayer(cloud,GByte,i,j);
	  vsnow  = Glayer(snow,GByte,i,j);

	  if(vcloud == UNDEFINED)
	    {
	      Glayer(water,GByte,i,j) = UNDEFINED;
	    }
	  else if(vcloud == CLOUD)
	    {
	      Glayer(water,GByte,i,j) = CLOUD;
	    }
	  else if(vsnow < 3)
	    {
	      switch(vsnow)
		{
		case 0:   /* error */
		  //errexit(outerr,"inconsistent background between cloud and snow masks");
		  Glayer(water,GByte,i,j) = UNDEFINED;
		  break;
		case 1:   /* snow */
		  Glayer(water,GByte,i,j) = SNOW;
		  break;
		case 2:   /* bright coastal water */
		  Glayer(water,GByte,i,j) = WATER;
		  break;
		}
	    }
	  else if(sensor == LANDSAT) /* Landsat 4,5,7 */
	    {
	      blue  = Glayer(sr_blue, GUInt16,i,j);
	      green = Glayer(sr_green,GUInt16,i,j);
	      red   = Glayer(sr_red,  GUInt16,i,j);
	      nir   = Glayer(sr_nir,  GUInt16,i,j);
	      swir  = Glayer(sr_swir, GUInt16,i,j);
	      swir2 = Glayer(sr_swir2,GUInt16,i,j);

	      if(nir + red > 0)
		ndvi = (nir - red)/(nir + red);
	      else
		ndvi = 0;

	      vsbar = (blue + green + red)/3;
	      irbar = (nir + swir + swir2)/3;
	      
	      Glayer(water,GByte,i,j) = LAND;

	      if((blue + green)/(swir + swir2) > 1.0)
		{
		  /* spectral shape downward sloping (unlike soil or rock) */

		  if(nir < 150 && swir < 80)
		    {
		      /* dark in infrared (allowing for some veg or glacial contamination in NIR) */
		      Glayer(water,GByte,i,j) = WATER;
		    }		  
		  else if(vsbar > 100 && (swir + swir2)/2 < 30)
		    {
		      /* glacial lakes, water with high sediment content */
		      Glayer(water,GByte,i,j) = GLACIAL;
		    }
		}
	      
	      if(Glayer(water,GByte,i,j) == LAND && ndvi < -0.2)  /* not vegetated */
		{
		  if((vsbar + irbar)/2 < 50)  /* mean less than 5% probably water rather than bare */
		     Glayer(water,GByte,i,j) = WATER;
		  else
		     Glayer(water,GByte,i,j) = BARE;
		}
	    }
	  else if(sensor == SPOT) /* SPOT 5 */
	    {
	      green = Glayer(sr_green,GUInt16,i,j) * HRG_TO_ETM_GREEN;
	      red   = Glayer(sr_red,  GUInt16,i,j) * HRG_TO_ETM_RED;
	      nir   = Glayer(sr_nir,  GUInt16,i,j) * HRG_TO_ETM_NIR;
	      swir  = Glayer(sr_swir, GUInt16,i,j) * HRG_TO_ETM_SWIR;

	      if(nir + red > 0)
		ndvi = (nir - red)/(nir + red);
	      else
		ndvi = 0;

	      vsbar = (green + red)/2;
	      irbar = (nir + swir)/2;
	      
	      Glayer(water,GByte,i,j) = LAND;

	      if(green/swir > 1.0)
		{
		  /* spectral shape downward sloping (unlike soil or rock) */

		  if(nir < 150 && swir < 80)
		    {
		      /* dark in infrared (allowing for some veg or glacial contamination in NIR) */
		      Glayer(water,GByte,i,j) = WATER;
		    }		  
		  else if(vsbar > 100 && swir < 30)
		    {
		      /* glacial lakes, water with high sediment content */
		      Glayer(water,GByte,i,j) = GLACIAL;
		    }
		}
	      
	      if(Glayer(water,GByte,i,j) == LAND && ndvi < -0.2)  /* not vegetated */
		{
		  if((vsbar + irbar)/2 < 50)  /* mean less than 5% probably water rather than bare */
		    Glayer(water,GByte,i,j) = WATER;
		  else
		    if(green/swir <= 1.0 && vsbar > 40) /* ensure upward sloping and at least 4% in visible */
		      Glayer(water,GByte,i,j) = BARE;
		}
	    }
	  else if(sensor == LANDSAT8) /* Landsat 8 */
	    {
	      blue  = Glayer(sr_blue, GUInt16,i,j);
	      green = Glayer(sr_green,GUInt16,i,j);
	      red   = Glayer(sr_red,  GUInt16,i,j);
	      nir   = Glayer(sr_nir,  GUInt16,i,j);
	      swir  = Glayer(sr_swir, GUInt16,i,j);
	      swir2 = Glayer(sr_swir2,GUInt16,i,j);

	      if(nir + red > 0)
		ndvi = (nir - red)/(nir + red);
	      else
		ndvi = 0;

	      vsbar = (blue + green + red)/3;
	      irbar = (nir + swir + swir2)/3;
	      
	      Glayer(water,GByte,i,j) = LAND;

	      if((blue + green)/(swir + swir2) > 1.0)
		{
		  /* spectral shape downward sloping (unlike soil or rock) */

		  if(nir < 150 && swir < 80)
		    {
		      /* dark in infrared (allowing for some veg or glacial contamination in NIR) */
		      Glayer(water,GByte,i,j) = WATER;
		    }		  
		  else if(vsbar > 100 && (swir + swir2)/2 < 30)
		    {
		      /* glacial lakes, water with high sediment content */
		      Glayer(water,GByte,i,j) = GLACIAL;
		    }
		}
	      
	      if(Glayer(water,GByte,i,j) == LAND && ndvi < -0.2)  /* not vegetated */
		{
		  if((vsbar + irbar)/2 < 50)  /* mean less than 5% probably water rather than bare */
		    Glayer(water,GByte,i,j) = WATER;
		  else
		    Glayer(water,GByte,i,j) = BARE;
		}
	    }
	  else if(sensor == SENTINEL2) /* Sentinel 2 */
	    {
	      blue  = Glayer(sr_blue, GUInt16,i,j);
	      green = Glayer(sr_green,GUInt16,i,j);
	      red   = Glayer(sr_red,  GUInt16,i,j);
	      nir   = Glayer(sr_nir,  GUInt16,i,j);
	      swir  = Glayer(sr_swir, GUInt16,i,j);
	      swir2 = Glayer(sr_swir2,GUInt16,i,j);

	      if((blue + green + red + nir + swir + swir2) == 0)
		Glayer(water,GByte,i,j) = UNDEFINED;
	      else
		{
		  blue  = blue  * MSI_TO_ETM_BLUE_C1  + MSI_TO_ETM_BLUE_C0;
		  green = green * MSI_TO_ETM_GREEN_C1 + MSI_TO_ETM_GREEN_C0;
		  red   = red   * MSI_TO_ETM_RED_C1   + MSI_TO_ETM_RED_C0;
		  nir   = nir   * MSI_TO_ETM_NIR_C1   + MSI_TO_ETM_NIR_C0;
		  swir  = swir  * MSI_TO_ETM_SWIR_C1  + MSI_TO_ETM_SWIR_C0;
		  swir2 = swir2 * MSI_TO_ETM_SWIR2_C1 + MSI_TO_ETM_SWIR2_C0;

		  if(nir + red > 0)
		    ndvi = (nir - red)/(nir + red);
		  else
		    ndvi = 0;

		  vsbar = (blue + green + red)/3;
		  irbar = (nir + swir + swir2)/3;
	      
		  Glayer(water,GByte,i,j) = LAND;

		  if(vsbar <= 30 && irbar <= 50)
		    {
		      /* average reflectance very low (even if slight upwards curve) */
		      Glayer(water,GByte,i,j) = WATER;
		    }
		  else if((blue + green)/(swir + swir2) > 1.2)
		    {
		      /* spectral shape downward sloping (unlike soil or rock) */
			
		      if(nir < 150 && swir < 80 && swir2 < 60)
			{
			  /* dark in infrared (allowing for some veg or glacial contamination in NIR) */
			  Glayer(water,GByte,i,j) = WATER;
			}		  
		      else if(vsbar > 100 && (swir + swir2)/2 < 30)
			{
			  /* glacial lakes, wet rock, water with high sediment content */
			  Glayer(water,GByte,i,j) = GLACIAL;
			}
		    }
	      
		  if(Glayer(water,GByte,i,j) == LAND && ndvi < -0.2)  /* not vegetated */
		    {
		      if(vsbar < 50 && irbar < 50)  /* less than 5% probably water rather than bare */
			Glayer(water,GByte,i,j) = WATER;
		      else
			Glayer(water,GByte,i,j) = BARE;
		    }
		}
	    }
	  else
	    errexit(outerr,"expecting 4,6,7 or 10 band imagery (SPOT/LANDSAT/SENTINEL)");
	}
    }

  jobprog(100);
  
  CreateRasterFile(argv[4], 1, water->width, water->height, GDT_Byte, water->GeoTransform, water->pszProjection);

  GlayerWriteWholeLayer(water, argv[4], 1);

  /* set thematic (categorical) layer and do statistics */
  SetThematic(argv[4]);

  /* add class names and colours */
  AddClassColumn(argv[4]);

  /* add pyramid layers for thematic data */
  BuildOverviews(argv[4],"NEAREST");
}

Glayer *
GlayerReadWholeLayer(char *fname, int band)
{
  Glayer *glayer = (Glayer *)calloc(1,sizeof(Glayer));
  long j;
  
  glayer->hDataset = GDALOpen(fname, GA_ReadOnly);

  if(glayer->hDataset == NULL)
    errexit(outerr, "error opening %s", fname);

  glayer->hBand = GDALGetRasterBand(glayer->hDataset, band);
  
  glayer->pixeltype = GDALGetRasterDataType(glayer->hBand);  

  glayer->width  = GDALGetRasterBandXSize(glayer->hBand);
  glayer->height = GDALGetRasterBandYSize(glayer->hBand);

  GDALGetBlockSize(glayer->hBand, &glayer->blockwidth, &glayer->blockheight);

  glayer->nbpp = setnbpp(glayer->pixeltype);
  glayer->nbpl = glayer->width*glayer->nbpp;

  if(GDALGetGeoTransform(glayer->hDataset, glayer->GeoTransform) == CE_None)
    {
      glayer->pszProjection = strdup(GDALGetProjectionRef(glayer->hDataset));

      glayer->info = GeoTransformToMapInfo(glayer->info, glayer->GeoTransform, glayer->width, glayer->height);
 
      glayer->pw = glayer->info->pixelSize->width;
      glayer->ph = glayer->info->pixelSize->height;
      glayer->pa = glayer->pw*glayer->ph;
    }

  glayer->hCT = GDALGetRasterColorTable(glayer->hBand);

  glayer->papszMetadata = GDALGetMetadata(glayer->hBand, NULL);
  glayer->hRAT = GDALGetDefaultRAT(glayer->hBand);

  jobstate("allocating layer memory (%ld x %ld)", glayer->width, glayer->height);

  glayer->nbpp = setnbpp(glayer->pixeltype);
  glayer->nbpl = glayer->width*glayer->nbpp;

  /* allocate data memory */

  glayer->data = malloc((size_t)(glayer->height*glayer->nbpl));
      
  if(!glayer->data)
    errexit(outerr,"error allocating image memory");
  
  glayer->data = memset(glayer->data, 0, (size_t)(glayer->height*glayer->nbpl));
  
  glayer->line = (void **)calloc(glayer->height, sizeof(void *));

  for(j = 0; j < glayer->height; j++)
    glayer->line[j] = glayer->data + (size_t)(j*glayer->nbpl);

  jobstate("Reading %s [Layer %d]...", fname, band);

  GDALRasterIO(glayer->hBand, GF_Read, 0, 0, glayer->width, glayer->height, glayer->data, glayer->width, glayer->height, glayer->pixeltype, 0, 0);
    
  return(glayer);
}

void SetThematic(char *fname)
{
  GDALDatasetH hDataset;
  GDALRasterBandH hBand;
  GDALRasterAttributeTableH hRAT;
  int bApproxOK = FALSE;
  int bIncludeOutOfRange = FALSE;
  
  double pdfMin;
  double pdfMax;
  double pdfMean;
  double pdfStdDev;
  CPLErr error;
  char   value[80];
  GUIntBig panHistogram[256];
  double histmin;
  double histmax;
  double histstep;
  double histCalcMin;
  double histCalcMax;
  double histogram[256];
  int i, histnbins, bin;
  double max, sum, middle;
  
  hDataset = GDALOpen(fname, GA_Update);

  hBand = GDALGetRasterBand(hDataset, 1);

  GDALSetMetadataItem(hBand, "LAYER_TYPE", "thematic", NULL);

  error = GDALComputeRasterStatistics(hBand, bApproxOK,	&pdfMin, &pdfMax, &pdfMean, &pdfStdDev, NULL, NULL);

  if(error == CE_Failure)
    pdfMin = pdfMax = pdfMean = pdfStdDev = 0;

  sprintf(value,"%G", pdfMin);
  GDALSetMetadataItem(hBand, "STATISTICS_MINIMUM", value, NULL);

  sprintf(value,"%G", pdfMax);
  GDALSetMetadataItem(hBand, "STATISTICS_MAXIMUM", value, NULL);

  sprintf(value,"%G", pdfMean);
  GDALSetMetadataItem(hBand, "STATISTICS_MEAN", value, NULL);

  sprintf(value,"%G", pdfStdDev);
  GDALSetMetadataItem(hBand, "STATISTICS_STDDEV", value, NULL);

  /* because we did at full res - these are the default anyway */
  GDALSetMetadataItem(hBand, "STATISTICS_SKIPFACTORX", "1", NULL);
  GDALSetMetadataItem(hBand, "STATISTICS_SKIPFACTORY", "1", NULL);

  /* create a histogram so we can do the mode and median */

  /* if byte data use 256 bins and the whole range */
  
  histmin = 0;
  histmax = 255;
  histstep = 1.0;
  histCalcMin = -0.5;
  histCalcMax = 255.5;
  histnbins = 256;

  GDALSetMetadataItem(hBand, "STATISTICS_HISTOBINFUNCTION", "direct", NULL);
   
  /* get histogram and force GDAL to recalculate it */

  GDALGetRasterHistogramEx(hBand, histCalcMin, histCalcMax, histnbins, panHistogram, bApproxOK, bIncludeOutOfRange, NULL, NULL);
  
  /* do the mode - bin with the highest count */

  max = sum = 0;

  for(i = 0; i < 256; i++)
    {
      if(panHistogram[i] > max)
	{
	  max = panHistogram[i];
	  bin = i;
	}
      sum += panHistogram[i];
      histogram[i] = panHistogram[i];  /* copy over to doubles for writing to RAT */
    }
  
  sprintf(value,"%d", bin);
  GDALSetMetadataItem(hBand, "STATISTICS_MODE", value, NULL);

  /* estimate the median - bin with the middle number */
  
  middle = sum/2;
  sum = 0;
  
  for(i = 0; i < 256; i++)
    {
      sum += panHistogram[i];
      
      if(sum > middle)
	{
	  bin = i;
	  break;
	}
    }

  sprintf(value,"%d", bin);
  GDALSetMetadataItem(hBand, "STATISTICS_MEDIAN", value, NULL);

  hRAT = GDALGetDefaultRAT(hBand);

  GDALRATSetRowCount(hRAT, histnbins);

  GDALRATCreateColumn(hRAT, "Histogram", GFT_Real, GFU_PixelCount);

  GDALRATValuesIOAsDouble(hRAT, GF_Write, GetRATColumnIndex(hRAT, "Histogram"), 0, 256,	histogram);
    
  GDALClose(hDataset);
}

void AddClassColumn(char *fname)
{
  GDALDatasetH hDataset;
  GDALRasterBandH hBand;
  GDALRasterAttributeTableH hRAT;
  int class;
  int red;
  int green;
  int blue;
  int alpha;

  hDataset = GDALOpen(fname, GA_Update);

  hBand = GDALGetRasterBand(hDataset, 1);
  hRAT = GDALGetDefaultRAT(hBand);

  GDALRATCreateColumn(hRAT, "Class", GFT_String, GFU_Generic);
  jobstate("Adding Class Column");

  GDALRATSetRowCount(hRAT,10);

  class = GetRATColumnIndex(hRAT, "Class");

  GDALRATSetValueAsString(hRAT,0,class,"Undefined");
  GDALRATSetValueAsString(hRAT,1,class,"Water");
  GDALRATSetValueAsString(hRAT,2,class,"Bare Ground");
  GDALRATSetValueAsString(hRAT,3,class,"Woody");
  GDALRATSetValueAsString(hRAT,4,class,"Herbaceous");
  GDALRATSetValueAsString(hRAT,5,class,"Cloud");
  GDALRATSetValueAsString(hRAT,6,class,"Primarily Bare Ground");
  GDALRATSetValueAsString(hRAT,7,class,"Snow");
  GDALRATSetValueAsString(hRAT,8,class,"Glacial Lakes, Wet Rock, Water/Sediment");
  GDALRATSetValueAsString(hRAT,9,class,"Other Land");

  GDALRATCreateColumn(hRAT, "Red", GFT_Integer, GFU_Red);
  jobstate("Adding Red Column");

  red = GetRATColumnIndex(hRAT, "Red");

  GDALRATSetValueAsInt(hRAT, 0, red,   0);
  GDALRATSetValueAsInt(hRAT, 1, red,  43);
  GDALRATSetValueAsInt(hRAT, 2, red, 211);
  GDALRATSetValueAsInt(hRAT, 3, red,   0);
  GDALRATSetValueAsInt(hRAT, 4, red, 246);
  GDALRATSetValueAsInt(hRAT, 5, red, 255);
  GDALRATSetValueAsInt(hRAT, 6, red, 209);
  GDALRATSetValueAsInt(hRAT, 7, red, 255);
  GDALRATSetValueAsInt(hRAT, 8, red,  90);
  GDALRATSetValueAsInt(hRAT, 9, red, 170);

  GDALRATCreateColumn(hRAT, "Green", GFT_Integer, GFU_Green);
  jobstate("Adding Green Column");

  green = GetRATColumnIndex(hRAT, "Green");

  GDALRATSetValueAsInt(hRAT, 0, green,   0);
  GDALRATSetValueAsInt(hRAT, 1, green, 148);
  GDALRATSetValueAsInt(hRAT, 2, green, 211);
  GDALRATSetValueAsInt(hRAT, 3, green, 100);
  GDALRATSetValueAsInt(hRAT, 4, green, 249);
  GDALRATSetValueAsInt(hRAT, 5, green,   0);
  GDALRATSetValueAsInt(hRAT, 6, green, 179);
  GDALRATSetValueAsInt(hRAT, 7, green,   0);
  GDALRATSetValueAsInt(hRAT, 8, green, 178);
  GDALRATSetValueAsInt(hRAT, 9, green, 170);

  GDALRATCreateColumn(hRAT, "Blue", GFT_Integer, GFU_Blue);
  jobstate("Adding Blue Column");

  blue = GetRATColumnIndex(hRAT, "Blue");

  GDALRATSetValueAsInt(hRAT, 0, blue,   0);
  GDALRATSetValueAsInt(hRAT, 1, blue, 190);
  GDALRATSetValueAsInt(hRAT, 2, blue, 211);
  GDALRATSetValueAsInt(hRAT, 3, blue,   0);
  GDALRATSetValueAsInt(hRAT, 4, blue, 158);
  GDALRATSetValueAsInt(hRAT, 5, blue,   0);
  GDALRATSetValueAsInt(hRAT, 6, blue, 140);
  GDALRATSetValueAsInt(hRAT, 7, blue, 255);
  GDALRATSetValueAsInt(hRAT, 8, blue, 255);
  GDALRATSetValueAsInt(hRAT, 9, blue, 170);

  GDALRATCreateColumn(hRAT, "Alpha", GFT_Integer, GFU_Alpha);
  jobstate("Adding Alpha Column");

  if(!strcmp(fextn(fname),".img"))
    alpha = GetRATColumnIndex(hRAT, "Opacity");
  else
    alpha = GetRATColumnIndex(hRAT, "Alpha");

  GDALRATSetValueAsInt(hRAT, 0, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 1, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 2, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 3, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 4, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 5, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 6, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 7, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 8, alpha, 255);
  GDALRATSetValueAsInt(hRAT, 9, alpha, 255);

  GDALClose(hDataset);
}

int
GetRATColumnIndex(GDALRasterAttributeTableH hRAT, char *cname)
{
  int i;
  int col = -1;

  for(i = 0; i < GDALRATGetColumnCount(hRAT); i++)
    if(strcmp(GDALRATGetNameOfCol(hRAT, i), cname) == 0)
      col = i;

  if(col < 0)
    errexit(outerr,"%s column not found", cname);

  return(col);
}

void
CreateRasterFile(char *fname, int bands, int width, int height, GDALDataType pixeltype, double *GeoTransform, char *pszProjection)
{
  GDALDatasetH hDataset;
  char **pszStringList = NULL;
  char driver[80];
          
  if(!strcmp(fextn(fname),".kea"))
    sprintf(driver, "KEA");
  else
    errexit(outerr,"unsupported file extension");

  hDataset = GDALOpen(fname, GA_Update);

  if(hDataset != NULL)
    errexit(outerr,"%s already exists", fname);

  jobstate("Creating %s... (%s)", fname, driver);

  hDataset = GDALCreate(GDALGetDriverByName(driver), fname, width, height, bands, pixeltype, pszStringList);

  if(hDataset == NULL)
    errexit(outerr,"GDALCreate failed");

  GDALSetGeoTransform(hDataset, GeoTransform);
  GDALSetProjection(hDataset, pszProjection);

  GDALClose(hDataset);
}

void
GlayerWriteWholeLayer(Glayer *glayer, char *fname, int band)
{
  glayer->hDataset = GDALOpen(fname, GA_Update);

  if(glayer->hDataset == NULL)
    errexit(outerr,"Unable to open file %s", fname);

  glayer->hBand = GDALGetRasterBand(glayer->hDataset, band);

  if(glayer->hBand == NULL)
    errexit(outerr,"Unable to get band %d", band);

  jobstate("Writing Layer %d", band);

  GDALRasterIO(glayer->hBand, GF_Write, 0, 0, glayer->width, glayer->height, glayer->data, glayer->width, glayer->height, glayer->pixeltype, 0, 0);

  GDALClose(glayer->hDataset);
}

void
BuildOverviews(char *fname, char *aggregationType)
{
/*
 * Adds Pyramid layers to the dataset. Adds levels until
 * the raster dimension of the overview layer is < minoverviewdim,
 * up to a maximum level controlled by the levels parameter. 
 * 
 * Uses GDALBuildOverviews() to do the work. 
 */

/* Build raster overview(s)
 *
 * If the operation is unsupported for the indicated dataset, then CE_Failure is returned, and CPLGetLastErrorNo() will return CPLE_NotSupported.
 *
 * Depending on the actual file format, all overviews level can be also deleted by specifying nOverviews == 0. This works at least for external overviews (.ovr), TIFF internal overviews, etc.
 *
 * This method is the same as the C function GDALBuildOverviews().
 *
 * Parameters
 *    pszResampling	one of "AVERAGE", "AVERAGE_MAGPHASE", "BILINEAR", "CUBIC", "CUBICSPLINE", "GAUSS", "LANCZOS", "MODE", "NEAREST", or "NONE" controlling the downsampling method applied.
 *    nOverviews	number of overviews to build, or 0 to clean overviews.
 *    panOverviewList	the list of overview decimation factors to build, or NULL if nOverviews == 0.
 *    nListBands	number of bands to build overviews for in panBandList. Build for all bands if this is 0.
 *    panBandList	list of band numbers.
 *    pfnProgress	a function to call to report progress, or NULL.
 *    pProgressData	application data to pass to the progress function.
 *
 * Returns
 *    CE_None on success or CE_Failure if the operation doesn't work.
 *
 * For example, to build overview level 2, 4 and 8 on all bands the following call could be made:
 *
 *  int anOverviewList[3] = { 2, 4, 8 };
 *
 *  poDataset->BuildOverviews( "NEAREST", 3, anOverviewList, 0, nullptr,
 *                             GDALDummyProgress, nullptr );
 *
 * See also
 *    GDALRegenerateOverviews() 
 */
  
  GDALDatasetH hDataset;
  int nbands, width, height;
  int levels[8] = {4,8,16,32,64,128,256,512};
  int minoverviewdim = 33;
  int i, mindim, nOverviews;
  int *panOverviewList;
  
  hDataset = GDALOpen(fname, GA_Update);

  /* first we work out how many overviews to build based on the size */

  nbands = GDALGetRasterCount(hDataset);
  width  = GDALGetRasterXSize(hDataset);
  height = GDALGetRasterYSize(hDataset);

  if(width < height)
    mindim = width;
  else
    mindim = height;
    
  nOverviews = 0;

  for(i = 0; i < 8; i++)
    if(mindim / levels[i] > minoverviewdim)   
      nOverviews++;

  panOverviewList = (int *)calloc(nOverviews, sizeof(int));

  for(i = 0; i < nOverviews; i++)
    panOverviewList[i] = levels[i];
  
  jobstate("Computing Pyramid Layers..."); /* no progress bar for now */

  GDALBuildOverviews(hDataset, aggregationType,	nOverviews, panOverviewList, 0, NULL, NULL, NULL);
  GDALClose(hDataset);
}

void
errexit(FILE *fp, char *file, int line, char *errstr, ...)
{
  va_list args;

  fprintf(fp,"%s:%d: ", file, line);
  va_start(args, errstr);
  vfprintf(fp, errstr, args);
  va_end(args);
  fprintf(fp,"\n");
  exit(0);
}

void
jobstate(char *fmt, ...)
{
  va_list args;
  
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout,"\n");
}

void
jobprog(int percent)
{
  fprintf(stdout,"\r%d %%", percent);

  if(percent < 100)
    fflush(stdout);
  else
    fprintf(stdout,"\n");
}

int
GdalGetNumberOfBands(char *fname)
{
  GDALDatasetH hDataset;
  int n;

  hDataset = GDALOpen(fname, GA_ReadOnly);

  if(hDataset == NULL)
    errexit(outerr, "error opening %s", fname);

  n = GDALGetRasterCount(hDataset);

  GDALClose(hDataset);

  if(n <= 0)
    errexit(outerr, "invalid number of bands returned from %s [%d]", fname, n);
 
  return(n);
}

void
GlayerCheckType(Glayer *glayer, GDALDataType pixeltype)
{
  if(glayer->pixeltype != pixeltype)
    errexit(outerr,"expecting %s data [%s]", GDALGetDataTypeName(pixeltype), GDALGetDataTypeName(glayer->pixeltype));
}

char *
fextn(char *fname)
{
  int len = strlen(fname);
  int dot = len - 1;
  char *string;
  
  while(fname[dot] != '.' && dot >= 0) dot--;

  string = (char *)calloc(len - dot, sizeof(char));

  strcpy(string, &fname[dot]);
  
  return(string);
}

int
setnbpp(GDALDataType pixeltype)
{
  int nbpp=0;
  
  switch(pixeltype)
    {
    case GDT_Byte:
      nbpp = sizeof(GByte);
      break;
    case GDT_UInt16:
      nbpp = sizeof(GUInt16);
     break;
    case GDT_Int16:
      nbpp = sizeof(GInt16);
      break;
    case GDT_UInt32:
      nbpp = sizeof(GUInt32);
      break;
    case GDT_Int32:
      nbpp = sizeof(GInt32);
      break;
    case GDT_Float32:
      nbpp = sizeof(float);
      break;
    case GDT_Float64:
      nbpp = sizeof(double);
      break;
    default:
      errexit(outerr,"datatypes greater than Float64 not yet supported");
      break;
    }
  
  return(nbpp);
}

Glayer *
GlayerCreate(long width, long height, GDALDataType pixeltype, Eprj_MapInfo *info, char *pszProjection)
{
  Glayer *glayer = (Glayer *)calloc(1,sizeof(Glayer));
  long j;
  
  glayer->width = width;
  glayer->height = height;
  glayer->pixeltype = pixeltype;
  
  glayer->nbpp = setnbpp(glayer->pixeltype);

  if(info)
    {
      glayer->info = CopyMapInfo(glayer->info, info);

      glayer->pw = info->pixelSize->width;
      glayer->ph = info->pixelSize->height;
      glayer->pa = glayer->pw*glayer->ph;

      MapInfoToGeoTransform(glayer->GeoTransform, info);

      glayer->pszProjection = strdup(pszProjection);
    }

  jobstate("allocating layer memory (%ld x %ld)", width, height);

  glayer->nbpp = setnbpp(glayer->pixeltype);
  glayer->nbpl = width*glayer->nbpp;

  /* allocate data memory */

  glayer->data = malloc((size_t)(height*glayer->nbpl));
      
  if(!glayer->data)
    errexit(outerr,"error allocating image memory");
  
  glayer->data = memset(glayer->data, 0, (size_t)(height*glayer->nbpl));
  
  glayer->line = (void **)calloc(height, sizeof(void *));

  for(j = 0; j < height; j++)
    glayer->line[j] = glayer->data + (size_t)(j*glayer->nbpl);

  return(glayer);
}

Eprj_MapInfo *
GeoTransformToMapInfo(Eprj_MapInfo *info, double *GeoTransform, long width, long height)
{
  if(!GeoTransform)
    errexit(outerr, "undefined GeoTransform");
  
  if(GeoTransform[2] != 0 || GeoTransform[4] != 0)
    errexit(outerr, "expecting north up imagery only");

  if(!info)
    info = CreateMapInfo();
  
  info->upperLeftCenter->x  = GeoTransform[0] + 0.5*GeoTransform[1];
  info->upperLeftCenter->y  = GeoTransform[3] + 0.5*GeoTransform[5];

  info->pixelSize->width    = fabs(GeoTransform[1]);
  info->pixelSize->height   = fabs(GeoTransform[5]);

  info->lowerRightCenter->x = info->upperLeftCenter->x + (width - 1)*info->pixelSize->width;
  info->lowerRightCenter->y = info->upperLeftCenter->y - (height - 1)*info->pixelSize->height;

  return(info);
}

double *
MapInfoToGeoTransform(double *GeoTransform, Eprj_MapInfo *info)
{
  if(!info)
    errexit(outerr, "undefined MapInfo");
    
  if(!GeoTransform)
    GeoTransform = (double *)calloc(6, sizeof(double));

  GeoTransform[0] = info->upperLeftCenter->x - 0.5*info->pixelSize->width;
  GeoTransform[1] = info->pixelSize->width;
  GeoTransform[3] = info->upperLeftCenter->y + 0.5*info->pixelSize->width;
  GeoTransform[5] = -info->pixelSize->height;

  return(GeoTransform);  
}

Eprj_MapInfo *
CreateMapInfo()
{
  Eprj_MapInfo *info;

  info = (Eprj_MapInfo *)calloc(1, sizeof(Eprj_MapInfo));

  info->upperLeftCenter  = (Eprj_Coordinate *)calloc(1, sizeof(Eprj_Coordinate));
  info->lowerRightCenter = (Eprj_Coordinate *)calloc(1, sizeof(Eprj_Coordinate));
  info->pixelSize        = (Eprj_Size *)calloc(1, sizeof(Eprj_Size));
  
  return(info);
}

Eprj_MapInfo *
CopyMapInfo(Eprj_MapInfo *dest, Eprj_MapInfo *src)
{
  if(!dest)
    dest = CreateMapInfo();

  if(!src)
    errexit(outerr, "undefined source MapInfo");
  
  dest->upperLeftCenter->x  = src->upperLeftCenter->x;
  dest->upperLeftCenter->y  = src->upperLeftCenter->y;

  dest->lowerRightCenter->x = src->lowerRightCenter->x;
  dest->lowerRightCenter->y = src->lowerRightCenter->y;

  dest->pixelSize->width    = src->pixelSize->width;
  dest->pixelSize->height   = src->pixelSize->height;    
 
  return(dest);
}

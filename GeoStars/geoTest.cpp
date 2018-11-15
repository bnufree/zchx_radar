///**************************************************************************
//* @File: geoTest.cpp
//* @Description:  地理库测试类
//*   This file contains test routines to test out some of the aspects
//*  of geoPoint  namely the point position and the sun track
//*  are activated by the use of #define statments near the top of the file.
//*  To compile one of these, just uncomment the test's #define statement. Some
//*  these also have sub-tests in the routines that are also activated by #define
//*  statements.
//*  
//*  Here are the test routine's definitions:
//*    - \b GEO_TEST_MAIN - This routine tests most of the geoPoint.c . It is a command line program that take LLH in DMS as parameters.
//*    - \b GEO_TEST_POINT - this tests the geoPoint.c routines. It has a subtest that can be enabled
//*        -# \b GEO_TEST_POINT_XYZ_RAE - this prints out a table to verify that the XYZ and RAE routines are working.
//* 
//* @Copyright: Copyright (c) 2017
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李鹭
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2017/03/20    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

// --------test routines ----------------
// Uncomment one of these to run test programs

// main command line test
//#define  GEO_TEST_MAIN 1

// geoPoint tests
#define GEO_TEST_POINT 1
#define GEO_TEST_POINT_XYZ_RAE 1





//-----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "geoStars.h"

#define GEO_TEST_LAT   31.292250   //!< Test latitude 31-17-32.1
#define GEO_TEST_LON   -85.851056  //!< Test longitude  85-51-03.8
#define GEO_TEST_HGT   134.0       //!< Test height in meters
#define GEO_TEST_DATUM GEO_DATUM_DEFAULT      //!< WGS84 Datum


//-----------------------------------------------------------------

#ifdef GEO_TEST_POINT
int main(int argc, char *argv[])
{

   /* These coordinates should produce the following
	   az=124.793358     x= 7516.05080
	   el=  0.167697     y=-5222.49923
	   rg=9152.38961     z=   26.78781
   */
   double lat1 =   36.5343577778;    // WGS84 coordinates
   double lon1 = -115.5630833333;
   double hgt1 = 973.80509;

   double lat2 =   36.4872728222;
   double lon2 = -115.4792174278;
   double hgt2 = 1007.16;

   double xyz[3],rae[3],efg[3],lat,lon,hgt;
   GEO_LOCATION loc1,loc2;
   int i;

   printf("Lat=%f  lon=%f  hgt =%f\n",lat1,lon1,hgt1);

#ifdef GEO_TEST_POINT_XYZ_RAE
   /* First, make sure that rae-to-xyz and back work in every quadrant */
   for(i=0;i<36;i++)
   {
      rae[1]=i*10.0*DEG_TO_RAD;rae[0]=1000.0;rae[2]=100.0;
      geoRae2Xyz(rae,xyz);
      printf("az=%10.1f xyz=%10.1f %10.1f %10.1f | ",rae[1]*RAD_TO_DEG,xyz[0],xyz[1],xyz[2]);
      geoXyz2Rae(xyz,rae);
      printf(" azout=%10.1f\n",rae[1]*RAD_TO_DEG);
   }
#endif

   /*
      Next, convert two sets of geodetic coordinates into the
      local coordinates (polar and Cartesian) between them.
   */

	/* Build the location descriptors */
   geoInitLocation(&loc1, lat1, lon1, hgt1, GEO_DATUM_WE,  "Site 1");
   geoInitLocation(&loc2, lat2, lon2, hgt2, GEO_DATUM_WE,  "Site 2");

	/* Now convert them into local coordintates */
   if(geoEfg2XyzDiff(&loc1,&loc2,xyz)) printf("ERROR: Dissimilar Datums\n");
   geoXyz2Rae(xyz,rae);

   printf("%s\na=%8.3f   b=%8.3f\n  f=%1.8f  1/f=%4.10f\n e2=%1.8e  e2p=%e\n\n",ellips[loc1.datum].name,
               loc1.a, loc1.b, loc1.flat, 1.0/loc1.flat, loc1.e2, loc1.e2p);

   printf("%s [EFG] = %8.2f, %8.2f, %8.2f \n",loc1.name,loc1.e,loc1.f,loc1.g);
   printf("%s [EFG] = %8.2f, %8.2f, %8.2f \n",loc2.name,loc2.e,loc2.f,loc2.g);
   printf("\n Range=%f , Azimuth=%f , Elevation=%f\n XYZ=%f, %f, %f\n\n",
		rae[0],rae[1]/ DEG_TO_RAD,rae[2]/ DEG_TO_RAD,
		xyz[0],xyz[1],xyz[2]);

   /*
     Lastly, take the coordinates that we started out with and see if we get
     the same result if we use the derived geocentric (E,F,G) coordinates.
   */

   /* Convert back */
   efg[GEO_E] = loc1.e;
   efg[GEO_F] = loc1.f;
   efg[GEO_G] = loc1.g;
   geoEfg2Llh(GEO_TEST_DATUM,efg,&lat,&lon,&hgt);
   printf("Converted back: Lat=%f Lon=%f hgt=%f\n delta lat=%f delta lon =%f delta hgt =%f\n",
      lat/ DEG_TO_RAD,        lon/ DEG_TO_RAD,hgt,
      lat/ DEG_TO_RAD - lat1, lon/ DEG_TO_RAD - lon1, hgt - hgt1);

  printf("Magnetic Declination = %f degrees\n", loc1.Declination);
}


#endif


//-----------------------------------------------------------------

//#ifdef GEO_TEST_MAIN
//void main(int argc, char *argv[])
//{

//   int deg,min;
//   double lat1,lat2,lon1,lon2,hgt1,hgt2,sec;
//   double xyz[3],rae[3], sunAz1,sunAz2,sunEl1,sunEl2,sign;
//   GEO_LOCATION loc1, loc2;

//   /* print usage if there is a failure */
//   if(argc < 15 )
//   {
//      printf("\nUsage: geoTest  {site 1:lat lon hgt} {site 2: lat lon hgt}\n\n");
//      printf("Example:\ngeotest 36 32 3.688 -115 33 47.1 973.80509 36 29 14.18216 -115 28 45.18274 1007.16\n\n");
//      printf(" will produce\n\n");
//      printf("Origin: lat=36.534358 lon=-115.563083 hgt=973.805090\n");
//      printf("Point  [EFG] = -2208978.2, -4635545.5,  3772206.7 SunAz=304.647  SunEl=-60.286\n");
//      printf("Ellipsoid: Clarke 1866\n");
//      printf("Origin [EFG] = -2214407.9, -4629478.2,  3776386.9 SunAz=304.598  SunEl=-60.204\n");
//      printf("Point  [EFG] = -2208978.2, -4635545.5,  3772206.7 SunAz=304.647  SunEl=-60.286\n\n");
//      printf(" Range=9152.46531 , Azimuth=124.79206 , Elevation=0.16770\n");
//      printf("  XYZ=7516.23150, -5222.37185,   26.78776\n");

//      exit(1);
//   }

//   /* Get the ORIGIN coordinates */
//   sign = 1.0;
//   deg = atoi(argv[1]); if(deg < 0.0) sign = -1.0;
//   min = atoi(argv[2]);
//   sec = atof(argv[3]);
//   lat1 = deg + min/60.0*sign + sec/3600.0*sign;
//   sign = 1.0;
//   deg = atoi(argv[4]); if(deg < 0.0) sign = -1.0;
//   min = atoi(argv[5]);
//   sec = atof(argv[6]);
//   lon1 = deg + min/60.0*sign + sec/3600.0*sign;
//   hgt1 = atof(argv[7]);
//   printf("\nOrigin: lat=%f lon=%f hgt=%f\n",lat1,lon1,hgt1);

//   /* Get the POINT coordinates */
//   sign = 1.0;
//   deg = atoi(argv[8]); if(deg < 0.0) sign = -1.0;
//   min = atoi(argv[9]);
//   sec = atof(argv[10]);
//   lat2 = deg + min/60.0*sign + sec/3600.0*sign;
//   sign = 1.0;
//   deg = atoi(argv[11]); if(deg < 0.0) sign = -1.0;
//   min = atoi(argv[12]);
//   sec = atof(argv[13]);
//   lon2 = deg + min/60.0*sign + sec/3600.0*sign;
//   hgt2 = atof(argv[14]);
//   printf("Point : lat=%f lon=%f hgt=%f\n",lat2,lon2,hgt2);

//   /* Do the comps with Clarke 1866 ellisoid */
//   geoInitLocation(&loc1, lat1, lon1, hgt1, GEO_DATUM_CC,  "Origin");
//   geoInitLocation(&loc2, lat2, lon2, hgt2, GEO_DATUM_CC,  "Point ");

//   /* Get the sun positon at both sites */
//   geoSunAzElNow(&loc1, &sunAz1, &sunEl1);
//   geoSunAzElNow(&loc2, &sunAz2, &sunEl2);

//   /* Compute X,Y,Z between the sites */
//   if(geoEfg2XyzDiff(&loc1,&loc2,xyz)) printf("ERROR: Dissimilar Datums\n");

//   /* Compute R,A,E between the sites */
//   geoXyz2Rae(xyz,rae);

//   printf("Ellipsoid: %s\n\n",ellips[loc1.datum].name);

//   printf("%s [EFG] = %10.1f, %10.1f, %10.1f SunAz=%5.3f  SunEl=%5.3f\n",
//           loc1.name,loc1.e,loc1.f,loc1.g,sunAz1,sunEl1);
//   printf("%s [EFG] = %10.1f, %10.1f, %10.1f SunAz=%5.3f  SunEl=%5.3f\n",
//           loc2.name,loc2.e,loc2.f,loc2.g,sunAz2,sunEl2);
//   printf("\n Range=%10.5f , Azimuth=%5.5f , Elevation=%5.5f\n XYZ=%10.5f, %10.5f, %10.5f\n",
//		rae[GEO_RNG],
//		rae[GEO_AZ]*RAD_TO_DEG,
//		rae[GEO_EL]*RAD_TO_DEG,
//		xyz[GEO_X],
//		xyz[GEO_Y],
//		xyz[GEO_Z]);
//   printf("Magnetic Declination = %f degrees\n", loc.Declination);

///*
//Here is some test data that looks good

//Command line:
//geotest 36 32 3.688  -115 33 47.1 973.80509 36 29 14.18216 -115 28 45.18274 1007.16


//Origin: lat=36.534358 lon=-115.563083 hgt=973.805090
//Point : lat=36.487273 lon=-115.479217 hgt=1007.160000
//Ellipsoid: Clarke 1866

//Origin [EFG] = -2214407.9, -4629478.2,  3776386.9 SunAz=304.598  SunEl=-60.204
//Point  [EFG] = -2208978.2, -4635545.5,  3772206.7 SunAz=304.647  SunEl=-60.286

// Range=9152.46531 , Azimuth=124.79206 , Elevation=0.16770
// XYZ=7516.23150, -5222.37185,   26.78776

//*/

//}

//#endif

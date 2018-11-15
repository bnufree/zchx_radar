///**************************************************************************
//* @File: geoMagTest.cpp
//* @Description:  地磁模型的地磁惯例测试类
//*  This file contains the source for deriving geomagnetic variables
//*   from geodetic coordinates
//*  http://aa.usno.navy.mil/software/novas/novas_info.html
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "geoStars.h"


int main(int argc, const char* argv[])
{
	  int i;
      double dlat, dlon;
      double ati, adec, adip;
      double alt, gv;
      double time, dec, dip, ti;
      double x,y,z,h;
      double ax,ay,az,ah;


	  dlat =   80.0;  // Degrees of latitude
      dlon =    0.0;  // Degrees of longitude
	  alt  =    0.0;  // Altitude in meters (ASL)
/*
      // New Mexico coords (Santa Fe)
	  dlat =   35.5;  // Degrees of latitude
      dlon = -106.0;  // Degrees of longitude
	  alt  = 1500.0;  // Altitude in meters (ASL)
*/
	  time = 2007.5;  // Time in years

	  // use the geoMag routine to get all of the specific values
      geoMag(alt,dlat,dlon,time,&dec,&dip,&ti,&gv,&adec,&adip,&ati,
            &x, &y, &z, &h, &ax, &ay, &az, &ah);


      printf("\nLATITUDE: %7.2f Deg",dlat);
      printf("  LONGITUDE: %7.2f Deg",dlon);
      printf("  ALTITUDE: %.2f M",alt);
      printf("\n DATE : %5.1f\n",time);
      printf("\n\t\t\t      OUTPUT\n\t\t\t      ------");
      printf("\n\nMAIN FIELD COMPONENTS\t\t\t   ANNUAL CHANGE");
      printf("\n---------------------\t\t\t   -------------\n");
      printf("\n TI          = %-7.0f nT\t\t   TI  = %-6.0f  nT/yr",ti,ati);
      printf("\n HI          = %-7.0f nT\t\t   HI  = %-6.0f  nT/yr",h,ah);
      printf("\n X           = %-7.0f nT\t\t   X   = %-6.0f  nT/yr ",x,ax);
      printf("\n Y           = %-7.0f nT\t\t   Y   = %-6.0f  nT/yr ",y,ay);
      printf("\n Z           = %-7.0f nT\t\t   Z   = %-6.0f  nT/yr ",z,az);
      printf("\n DEC         = %-7.2f DEG\t\t   DEC = %-6.2f MIN/yr [%-6.2f deg/yr]  ",dec,adec,adec/60.0);
      printf("\n DIP         = %-7.2f DEG\t\t   DIP = %-6.2f  MIN/yr\n",dip,adip);


      // Print the yearly declination values
	  printf("----------------------------------------------------------\n");
      printf(" Yearly Declination Values \n");
      printf("----------------------------------------------------------\n");
      for(i=0;i<=9;i++)
	  {
		geoMagGetDec(dlat,dlon,alt,1,1,2000+i, &dec);
		printf("%04d : %f deg\n",i+2000,dec);
	  }

      // Print the simple declination values for a location and time
	  printf("\n\n----------------------------------------------------------\n");
      printf(" Lat, Lon, Hgt, and time (MM/DD/YY) declination example \n");
      printf("----------------------------------------------------------\n");

	  geoMagGetDec(dlat, dlon, alt, 6, 1, 2000, &dec);
	  printf(" Declination for %7.1f latitude, %5.1f longitude, %5.1f altitude \n is %4.2f degrees on 6/1/2000\n",
		         dlat, dlon, alt, dec);

	  dec  = geoMagGetDecRet(dlat, dlon, alt, 6, 1, 2000);
	  printf("RET: Declination for %7.1f latitude, %5.1f longitude, %5.1f altitude \n is %4.2f degrees on 6/1/2000\n",
		         dlat, dlon, alt, dec);

      // Print the simple declination values for a location and time
	  printf("\n\n----------------------------------------------------------\n");
      printf("Declination for the lower 5km of altitude near Santa Fe, Nm\n");
      printf("----------------------------------------------------------\n");

      for(i=0;i<=5;i++)
	  {

         geoMagGetDec(dlat, dlon, i*1000.0, 6, 1, 2000, &dec);
	     printf(" Declination for %7.1f latitude, %5.1f longitude, %6.0f meters altitude is %4.4f degrees on 6/1/2000\n",
		         dlat, dlon, i*1000.0, dec);
	  }


}

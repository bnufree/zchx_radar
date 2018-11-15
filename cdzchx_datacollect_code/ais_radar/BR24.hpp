
// ZCHXBR24.H

#ifndef ZCHXBR24_H
#define ZCHXBR24_H

namespace BR24 {
namespace Constants {



struct common_header {
  uint8_t headerLen;       // 1 bytes
  uint8_t status;          // 1 bytes
  uint8_t scan_number[2];  // 2 bytes, 0-4095
  uint8_t u00[4];          // 4 bytes
  uint8_t angle[2];        // 2 bytes
  uint8_t heading[2];      // 2 bytes heading with RI-10/11. See bitmask explanation above.
};

struct br24_header {
  uint8_t headerLen;       // 1 bytes
  uint8_t status;          // 1 bytes
  uint8_t scan_number[2];  // 2 bytes, 0-4095
  uint8_t mark[4];         // 4 bytes 0x00, 0x44, 0x0d, 0x0e
  uint8_t angle[2];        // 2 bytes
  uint8_t heading[2];      // 2 bytes heading with RI-10/11. See bitmask explanation above.
  uint8_t range[4];        // 4 bytes
  uint8_t u01[2];          // 2 bytes blank
  uint8_t u02[2];          // 2 bytes
  uint8_t u03[4];          // 4 bytes blank
};                       /* total size = 24 */

struct br4g_header {
  uint8_t headerLen;       // 1 bytes
  uint8_t status;          // 1 bytes
  uint8_t scan_number[2];  // 2 bytes, 0-4095
  uint8_t u00[2];          // Always 0x4400 (integer)
  uint8_t largerange[2];   // 2 bytes or -1
  uint8_t angle[2];        // 2 bytes
  uint8_t heading[2];      // 2 bytes heading with RI-10/11 or -1. See bitmask explanation above.
  uint8_t smallrange[2];   // 2 bytes or -1
  uint8_t rotation[2];     // 2 bytes, rotation/angle
  uint8_t u02[4];          // 4 bytes signed integer, always -1
  uint8_t u03[4];          // 4 bytes signed integer, mostly -1 (0x80 in last byte) or 0xa0 in last byte
};                       /* total size = 24 */

struct radar_line {
  union {
    common_header common;
    br24_header br24;
    br4g_header br4g;
  };
  uint8_t data[512];
};


/* Normally the packets are have 32 spokes, or scan lines, but we assume nothing
 * so we take up to 120 spokes. This is the nearest round figure without going over
 * 64kB.
 */

struct radar_frame_pkt {
  uint8_t frame_hdr[8];
  //radar_line line[120];  //  scan lines, or spokes
  radar_line line[32];
};


} // Constants
} // Core

#endif //ZCHXCONFIG.H


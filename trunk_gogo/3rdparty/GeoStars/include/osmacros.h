#ifndef ZCHX_OS_MACROS_HPP_
#define ZCHX_OS_MACROS_HPP_

#ifdef WIN32
#include "os-win32.h"
#else
#include "os-linux.h"
#endif

#endif /* ZCHX_OS_MACROS_HPP_ */
#ifndef ZCHX_OS_LINUX_HPP_
#define ZCHX_OS_LINUX_HPP_

#define LINUX_DLL_EXPORT
#define LINUX_DLL_IMPORT

#ifdef ZCHX_API_BUILD
#define ZCHX_API LINUX_DLL_EXPORT
#else
#define ZCHX_API LINUX_DLL_IMPORT
#endif

#endif /* ZCHX_OS_LINUX_HPP_ */
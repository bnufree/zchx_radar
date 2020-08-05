#ifndef ZCHX_OS_WINDOWS_HPP_
#define ZCHX_OS_WINDOWS_HPP_

#define WINDOWS_DLL_EXPORT __declspec(dllexport)
#define WINDOWS_DLL_IMPORT __declspec(dllimport)

#ifdef ZCHX_API_BUILD
#define ZCHX_API WINDOWS_DLL_EXPORT
#else
#define ZCHX_API WINDOWS_DLL_IMPORT
#endif

#endif /* ZCHX_OS_WINDOWS_HPP_ */
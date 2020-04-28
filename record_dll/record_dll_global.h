#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(RECORD_DLL_LIB)
//#  define RECORD_DLL_EXPORT Q_DECL_EXPORT
#define _DLL_EXP _declspec(dllexport)
# else
//#  define RECORD_DLL_EXPORT Q_DECL_IMPORT
#define _DLL_EXP _declspec(dllimport)
# endif
#else
# define RECORD_DLL_EXPORT
#endif

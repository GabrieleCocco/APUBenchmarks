// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SAXPYLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SAXPYLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

// This class is exported from the SaxpyLib.dll
#include "General.h"

class SAXPYLIB_API CSaxpyLib {
public:
	CSaxpyLib(void);
	// TODO: add your methods here.
};

extern SAXPYLIB_API int nSaxpyLib;

SAXPYLIB_API int fnSaxpyLib(void);

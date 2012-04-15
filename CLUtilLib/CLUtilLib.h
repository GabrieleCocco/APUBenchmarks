// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CLUTILLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CLUTILLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CLUTILLIB_EXPORTS
#define CLUTILLIB_API __declspec(dllexport)
#else
#define CLUTILLIB_API __declspec(dllimport)
#endif

// This class is exported from the CLUtilLib.dll
class CLUTILLIB_API CCLUtilLib {
public:
	CCLUtilLib(void);
	// TODO: add your methods here.
};

extern CLUTILLIB_API int nCLUtilLib;

CLUTILLIB_API int fnCLUtilLib(void);

// CLUtilLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "CLUtilLib.h"


// This is an example of an exported variable
CLUTILLIB_API int nCLUtilLib=0;

// This is an example of an exported function.
CLUTILLIB_API int fnCLUtilLib(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see CLUtilLib.h for the class definition
CCLUtilLib::CCLUtilLib()
{
	return;
}

// SaxpyLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "SaxpyLib.h"


// This is an example of an exported variable
SAXPYLIB_API int nSaxpyLib=0;

// This is an example of an exported function.
SAXPYLIB_API int fnSaxpyLib(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see SaxpyLib.h for the class definition
CSaxpyLib::CSaxpyLib()
{
	return;
}

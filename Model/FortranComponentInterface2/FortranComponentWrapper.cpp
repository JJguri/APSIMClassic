#include "FortranComponentWrapper.h"
#include <ComponentInterface2/ScienceAPI2.h>
#ifndef __WIN32__
   #include <dlfcn.h>
#else
   #include <windows.h>  // for LoadLibrary and mutex
#endif
#include <iostream>
#include <ComponentInterface2/CMPComponentInterface.h>
#include "DataTypes.h"
#include "FortranTemplates.h"
#include <ComponentInterface2/FortranArray.h>
#include <ComponentInterface2/FortranString.h>

using namespace std;

FortranComponentWrapper* currentWrapper = NULL;
#ifdef __WIN32__
CRITICAL_SECTION swapMutex;
bool swapMutexInited = false;
#endif

FortranComponentWrapper::FortranComponentWrapper(ScienceAPI2* api, CMPComponentInterface* ci, void* handle)
   : scienceapi(*api), dllHandle(handle), componentinterface(ci)
   {
   // -----------------------------------------------------------------------
   // Constructor
   // -----------------------------------------------------------------------
   realCommonBlock = NULL;
   scienceAPI().subscribe("init1", nullFunction(&FortranComponentWrapper::onInit1));
   }

FortranComponentWrapper::~FortranComponentWrapper()
   {
   // -----------------------------------------------------------------------
   // Destructor
   // -----------------------------------------------------------------------
   // get FORTRAN to release memory blocks.
     const int doAllocate = false;
     swapInstanceIn();
     allocDealloc(&doAllocate);
	 swapInstanceOut();
   }
void FortranComponentWrapper::onInit1()
   {
   // -----------------------------------------------------------------------
   // Init1 event handler.
   // -----------------------------------------------------------------------
   // get FORTRAN to create new memory blocks.
   #ifdef __WIN32__
       allocDealloc = (BoolMethod*) GetProcAddress((HMODULE)dllHandle, "alloc_dealloc_instance");
   #else
       allocDealloc = (void (*)(const int* doAllocate))dlsym(dllHandle, "alloc_dealloc_instance");
   #endif

   if (allocDealloc == NULL)
      throw runtime_error("Cannot find alloc_dealloc_instance routine in FORTRAN model");
   const int doAllocate = true;
   allocDealloc(&doAllocate);

   // Get the address of the common block and zero it's contents.
   typedef CommonBlock* CommonBlockPtr;
   typedef CommonBlockPtr (STDCALL GetInstanceMethod)();

   #ifdef __WIN32__
      GetInstanceMethod* getInstance = (GetInstanceMethod*) GetProcAddress((HMODULE)dllHandle, "getInstance");
   #else
      GetInstanceMethod* getInstance = (CommonBlockPtr (*)()) dlsym(dllHandle, "getInstance");
   #endif
   if (getInstance == NULL)
      throw runtime_error("Cannot find getInstance routine in FORTRAN model");
   realCommonBlock = getInstance();
   memcpy(&ourCommonBlock, realCommonBlock, sizeof(CommonBlock));

   // Call the FORTRAN OnInit1 routine.
   #ifdef __WIN32__
      NullMethod* OnInit1 = (NullMethod*) GetProcAddress((HMODULE)dllHandle, "OnInit1");
   #else
      NullMethod* OnInit1 = (void (*)())dlsym(dllHandle, "OnInit1");
   #endif
   if (OnInit1 == NULL)
      throw runtime_error("Cannot find onInit1 routine in FORTRAN model");
   swapInstanceIn();
   OnInit1();
   swapInstanceOut();
   }
void FortranComponentWrapper::swapInstanceIn()
   {
   // -----------------------------------------------------------------------
   // Swap an instance of the FORTRAN model into the appropriate address
   // space. i.e. move the contents of our stored common block into the
   // real common block space that is used by the FORTRAN code.
   // We also keep track of the previous wrappers, necessary when one instance of a FORTRAN
   // model calls into another instance of the same FORTRAN component interface.
   // -----------------------------------------------------------------------
#ifdef __WIN32__
   if (!swapMutexInited) {
     InitializeCriticalSection(&swapMutex);
     swapMutexInited = true;
   }
   EnterCriticalSection(&swapMutex);
#endif
   savedCommonBlocks.push(*realCommonBlock);
   *realCommonBlock = ourCommonBlock;
   callStack.push(currentWrapper);
   currentWrapper = this;
   }
void FortranComponentWrapper::swapInstanceOut()
   {
   // -----------------------------------------------------------------------
   // Save the contents of the real FORTRAN common block back into our
   // stored common block.
   // -----------------------------------------------------------------------
   *realCommonBlock = savedCommonBlocks.top();
   savedCommonBlocks.pop();
   currentWrapper = callStack.top();
   callStack.pop();
#ifdef __WIN32__
   LeaveCriticalSection(&swapMutex);
#endif
   }

// -----------------------------------------------------------------------
// Miscellaneous routines.
// -----------------------------------------------------------------------

extern "C" void EXPORT STDCALL WriteLine(const char* msg, unsigned int msgLength)
   {
   string line = FortranString(msg, msgLength).toString() + "\n";
   currentWrapper->componentInterface().write(line);
   }

extern "C" void EXPORT STDCALL Fatal(const char* msg, unsigned int msgLength)
   {
   currentWrapper->componentInterface().error(FortranString(msg, msgLength).toString(), true);
   }

extern "C" void EXPORT STDCALL Warning(const char* msg, unsigned int msgLength)
   {
   currentWrapper->componentInterface().error(FortranString(msg, msgLength).toString(), false);
   }

extern "C" void EXPORT STDCALL publish_null(const char* msg, unsigned int msgLength)
   {
   currentWrapper->componentInterface().publish(FortranString(msg, msgLength).toString(), NULL);
   }

// -----------------------------------------------------------------------
// The read routines for the FORTRAN.
// -----------------------------------------------------------------------

extern "C" void EXPORT STDCALL SetSearchOrder
   (const char* SectionName, unsigned SectionNameLength)
   {
   vector<string> sectionNames;
   sectionNames.push_back(FortranString(SectionName, SectionNameLength).toString());
   currentWrapper->componentInterface().setSearchOrder(sectionNames);
   }


extern "C" int EXPORT STDCALL ReadInteger
   (const char* name, const char* units, int* optional, int* value, int* lower, int* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   string variableName = FortranString(name, nameLength).toString();
   CMPBuiltInBounded<int, int> variable(variableName, *value, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadReal
   (const char* name, const char* units, int* optional, float* value, float* lower, float* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   string variableName = FortranString(name, nameLength).toString();
   CMPBuiltInBounded<float, float> variable(variableName, *value, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadDouble
   (const char* name, const char* units, int* optional, double* value, double* lower, double* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   string variableName = FortranString(name, nameLength).toString();
   CMPBuiltInBounded<double, double> variable(variableName, *value, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadString
   (const char* name, const char* units, int* optional, char* value,
    unsigned nameLength, unsigned unitsLength, unsigned valueLength)
   {
   vector<string> values;
   CMPBuiltIn<FortranString> variable(FortranString(value, valueLength));
   return currentWrapper->componentInterface().read(FortranString(name, nameLength).toString(),
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadIntegerArray
   (const char* name, const char* units, int* optional, int* value, int* numValues, int* maxValues, int* lower, int* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   *numValues = 0;
   string variableName = FortranString(name, nameLength).toString();
   FortranArray<int> arr(value, *numValues, *maxValues);
   CMPBuiltInBounded<FortranArray<int>, int> variable(variableName, arr, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadRealArray
   (const char* name, const char* units, int* optional, float* value, int* numValues, int* maxValues, float* lower, float* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   *numValues = 0;
   string variableName = FortranString(name, nameLength).toString();
   FortranArray<float> arr(value, *numValues, *maxValues);
   CMPBuiltInBounded<FortranArray<float>, float> variable(variableName, arr, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadDoubleArray
   (const char* name, const char* units, int* optional, double* value, int* numValues, int* maxValues, double* lower, double* upper,
    unsigned nameLength, unsigned unitsLength)
   {
   *numValues = 0;
   string variableName = FortranString(name, nameLength).toString();
   FortranArray<double> arr(value, *numValues, *maxValues);
   CMPBuiltInBounded<FortranArray<double>, double> variable(variableName, arr, *lower, *upper);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
extern "C" int EXPORT STDCALL ReadStringArray
   (const char* name, const char* units, int* optional, char* value, int* numValues, int* maxValues,
    unsigned nameLength, unsigned unitsLength, unsigned valueLength)
   {
   *numValues = 0;
   string variableName = FortranString(name, nameLength).toString();
   FortranStrings arr(value, valueLength, *maxValues, *numValues);
   CMPBuiltIn<FortranStrings> variable(arr);
   return currentWrapper->componentInterface().read(variableName,
                                                    &variable,
                                                    *optional != 0);
   }
// -----------------------------------------------------------------------
// The EXPOSE methods that FORTRAN calls to make one of its variables
// available to APSIM.
// -----------------------------------------------------------------------
extern "C" void EXPORT STDCALL ExposeInteger(const char* Name, const char* Units, const char* Description, int* Writable, int* Data,
                                             unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<int&>(*Data));
   }
extern "C" void EXPORT STDCALL ExposeReal(const char* Name, const char* Units, const char* Description, int* Writable, float* Data,
                                          unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<float&>(*Data));
   }
extern "C" void EXPORT STDCALL ExposeDouble(const char* Name, const char* Units, const char* Description, int* Writable, double* Data,
                                            unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
  currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<double&>(*Data));
   }
extern "C" void EXPORT STDCALL ExposeString(const char* Name, const char* Units, const char* Description, int* Writable, char* Data,
                                            unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength, unsigned DataLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<FortranString>(FortranString(Data, DataLength)));
   }
extern "C" void EXPORT STDCALL ExposeIntegerArray(const char* Name, const char* Units, const char* Description, int* Writable, int* Data, int* NumValues, int* MaxValues,
                                             unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<FortranArray<int> >(FortranArray<int>(Data, *NumValues, *MaxValues)));
   }
extern "C" void EXPORT STDCALL ExposeRealArray(const char* Name, const char* Units, const char* Description, int* Writable, float* Data, int* NumValues, int* MaxValues,
                                          unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<FortranArray<float> >(FortranArray<float>(Data, *NumValues, *MaxValues)));
   }
extern "C" void EXPORT STDCALL ExposeDoubleArray(const char* Name, const char* Units, const char* Description, int* Writable, double* Data, int* NumValues, int* MaxValues,
                                            unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<FortranArray<double> >(FortranArray<double>(Data, *NumValues, *MaxValues)));
   }
extern "C" void EXPORT STDCALL ExposeStringArray(const char* Name, const char* Units, const char* Description, int* Writable, char* Data, int* NumValues, int* MaxValues,
                                            unsigned NameLength, unsigned UnitsLength, unsigned DescriptionLength, unsigned DataLength)
   {
   currentWrapper->componentInterface().expose(FortranString(Name, NameLength).toString(),
                                               FortranString(Units, UnitsLength).toString(),
                                               FortranString(Description, DescriptionLength).toString(),
                                               *Writable != 0,
                                               new CMPBuiltIn<FortranStrings>(FortranStrings(Data, DataLength, 100, *NumValues)));
   }

// -----------------------------------------------------------------------
// Get methods.
// -----------------------------------------------------------------------
extern "C" int EXPORT STDCALL GetInteger
   (const char* Name, const char* Units, int* Optional, int* Value, int* Lower, int* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   string variableName = FortranString(Name, NameLength).toString();
   CMPBuiltInBounded<int&, int>* variable = new CMPBuiltInBounded<int&, int>(variableName, *Value, *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetReal
   (const char* Name, const char* Units, int* Optional, float* Value, float* Lower, float* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   string variableName = FortranString(Name, NameLength).toString();
   CMPBuiltInBounded<float&, float>* variable = new CMPBuiltInBounded<float&, float>(variableName, *Value, *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetDouble
   (const char* Name, const char* Units, int* Optional, double* Value, double* Lower, double* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   string variableName = FortranString(Name, NameLength).toString();
   CMPBuiltInBounded<double&, double>* variable = new CMPBuiltInBounded<double&, double>(variableName, *Value, *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetString
   (const char* Name, const char* Units, int* Optional, char* Value,
    unsigned NameLength, unsigned UnitsLength, unsigned ValueLength)
   {
   CMPBuiltIn<FortranString>* variable = new CMPBuiltIn<FortranString>(FortranString(Value, ValueLength));
   return currentWrapper->componentInterface().get(FortranString(Name, NameLength).toString(),
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetIntegerArray
   (const char* Name, const char* Units, int* Optional, int* Value, int* NumValues, int* MaxValues, int* Lower, int* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   *NumValues = 0;
   string variableName = FortranString(Name, NameLength).toString();
   typedef CMPBuiltInBounded<FortranArray<int>, int> Wrapper;
   Wrapper* variable = new Wrapper(variableName,
                                  FortranArray<int>(Value, *NumValues, *MaxValues),
                                  *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetRealArray
   (const char* Name, const char* Units, int* Optional, float* Value, int* NumValues, int* MaxValues, float* Lower, float* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   *NumValues = 0;
   string variableName = FortranString(Name, NameLength).toString();
   typedef CMPBuiltInBounded<FortranArray<float>, float> Wrapper;
   Wrapper* variable = new Wrapper(variableName,
                                  FortranArray<float>(Value, *NumValues, *MaxValues),
                                  *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetDoubleArray
   (const char* Name, const char* Units, int* Optional, double* Value, int* NumValues, int* MaxValues, double* Lower, double* Upper,
    unsigned NameLength, unsigned UnitsLength)
   {
   *NumValues = 0;
   string variableName = FortranString(Name, NameLength).toString();
   typedef CMPBuiltInBounded<FortranArray<double>, double> Wrapper;
   Wrapper* variable = new Wrapper(variableName,
                                  FortranArray<double>(Value, *NumValues, *MaxValues),
                                  *Lower, *Upper);
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                   variable);
   }
extern "C" int EXPORT STDCALL GetStringArray
   (const char* Name, const char* Units, int* Optional, char* Value, int* NumValues, int* MaxValues,
    unsigned NameLength, unsigned UnitsLength, unsigned ValueLength)
   {
   *NumValues = 0;
   string variableName = FortranString(Name, NameLength).toString();
   CMPBuiltIn<FortranStrings>* variable = new CMPBuiltIn<FortranStrings>(FortranStrings(Value, ValueLength, *MaxValues, *NumValues));
   return currentWrapper->componentInterface().get(variableName,
                                                   FortranString(Units, UnitsLength).toString(),
                                                   *Optional != 0,
                                                    variable);
   }
// -----------------------------------------------------------------------
// Set methods.
// -----------------------------------------------------------------------
extern "C" void EXPORT STDCALL SetInteger
   (const char* Name, const char* Units, int* Value,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<int>* wrapper = new CMPBuiltIn<int>(*Value);
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetReal
   (const char* Name, const char* Units, float* Value,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<float>* wrapper = new CMPBuiltIn<float>(*Value);
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetDouble
   (const char* Name, const char* Units, double* Value,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<double>* wrapper = new CMPBuiltIn<double>(*Value);
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetString
   (const char* Name, const char* Units, char* Value,
    unsigned NameLength, unsigned UnitsLength, unsigned ValueLength)
   {
   CMPBuiltIn<FortranString>* wrapper = new CMPBuiltIn<FortranString>(FortranString(Value, ValueLength));
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetIntegerArray
   (const char* Name, const char* Units, int* Value, int* NumValues,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<FortranArray<int> >* wrapper = new CMPBuiltIn<FortranArray<int> >(FortranArray<int>(Value, *NumValues, *NumValues));
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetRealArray
   (const char* Name, const char* Units, float* Value, int* NumValues,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<FortranArray<float> >* wrapper = new CMPBuiltIn<FortranArray<float> >(FortranArray<float>(Value, *NumValues, *NumValues));
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetDoubleArray
   (const char* Name, const char* Units, double* Value, int* NumValues,
    unsigned NameLength, unsigned UnitsLength)
   {
   CMPBuiltIn<FortranArray<double> >* wrapper = new CMPBuiltIn<FortranArray<double> >(FortranArray<double>(Value, *NumValues, *NumValues));
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }
extern "C" void EXPORT STDCALL SetStringArray
   (const char* Name, const char* Units, char* Value, int* NumValues,
    unsigned NameLength, unsigned UnitsLength, unsigned ValueLength)
   {
   CMPBuiltIn<FortranStrings>* wrapper = new CMPBuiltIn<FortranStrings>(FortranStrings(Value, ValueLength, *NumValues, *NumValues));
   currentWrapper->componentInterface().set(FortranString(Name, NameLength).toString(),
                                            FortranString(Units, UnitsLength).toString(),
                                            wrapper);
   }

// ------------------------------------------------------------------
// Provide access to a method for converting a string to a floating point numeric 
// which should be noticeably faster than a Fortran internal read (especially with gfortran)
// ------------------------------------------------------------------
extern "C" double EXPORT STDCALL string_to_float(const char* str, bool* ok, unsigned strLength)
{
	const unsigned MAX_CHARS=30;
	char buf[MAX_CHARS + 1];
	char* endPtr;
	int nChars = min(MAX_CHARS, strLength);
	memcpy(buf, str, nChars);
	buf[nChars] = '\0';
	double result = strtod(buf, &endPtr);
	// Check to be certain that we had a numeric value, and numeric value only
	*ok = (endPtr != buf) && ((*endPtr == NULL) || isspace(*endPtr));
	return result;
}

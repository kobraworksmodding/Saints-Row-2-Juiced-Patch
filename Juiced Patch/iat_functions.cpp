#include "stdafx.h"
#include "iat_functions.h"

#define PtrFromRva( base, rva ) ( ( ( PBYTE ) base ) + rva )

/*++
 Routine Description:
   Replace the function pointer in a module's IAT.

 Parameters:
   Module              - Module to use IAT from.
   ImportedModuleName  - Name of imported DLL from which
                         function is imported.
   ImportedProcName    - Name of imported function.
   AlternateProc       - Function to be written to IAT.
   OldProc             - Original function.

 Return Value:
   S_OK on success.
   (any HRESULT) on failure.
--*/
HRESULT PatchIat(HMODULE Module, PSTR ImportedModuleName, PSTR ImportedProcName, 
				 PVOID AlternateProc, PVOID *OldProc)
{
  PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER ) Module;
  PIMAGE_NT_HEADERS NtHeader;
  PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
  UINT Index;

//  _ASSERTE( Module );
//  _ASSERTE( ImportedModuleName );
//  _ASSERTE( ImportedProcName );
//  _ASSERTE( AlternateProc );

  NtHeader = ( PIMAGE_NT_HEADERS )
    PtrFromRva( DosHeader, DosHeader->e_lfanew );
  if( IMAGE_NT_SIGNATURE != NtHeader->Signature )
  {
    return HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
  }

  ImportDescriptor = ( PIMAGE_IMPORT_DESCRIPTOR )
    PtrFromRva( DosHeader,
      NtHeader->OptionalHeader.DataDirectory
        [ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );

  //
  // Iterate over import descriptors/DLLs.
  //
  for ( Index = 0;
        ImportDescriptor[ Index ].Characteristics != 0;
        Index++ )
  {
    PSTR dllName = ( PSTR )
      PtrFromRva( DosHeader, ImportDescriptor[ Index ].Name );

    if ( 0 == _strcmpi( dllName, ImportedModuleName ) )
    {
      //
      // This the DLL we are after.
      //
      PIMAGE_THUNK_DATA Thunk;
      PIMAGE_THUNK_DATA OrigThunk;

      if ( ! ImportDescriptor[ Index ].FirstThunk ||
         ! ImportDescriptor[ Index ].OriginalFirstThunk )
      {
        return E_INVALIDARG;
      }

      Thunk = ( PIMAGE_THUNK_DATA )
        PtrFromRva( DosHeader,
          ImportDescriptor[ Index ].FirstThunk );
      OrigThunk = ( PIMAGE_THUNK_DATA )
        PtrFromRva( DosHeader,
          ImportDescriptor[ Index ].OriginalFirstThunk );

      for ( ; OrigThunk->u1.Function != NULL;
              OrigThunk++, Thunk++ )
      {
        if ( OrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG )
        {
          //
          // Ordinal import - we can handle named imports
          // ony, so skip it.
          //
          continue;
        }

        PIMAGE_IMPORT_BY_NAME import = ( PIMAGE_IMPORT_BY_NAME )
          PtrFromRva( DosHeader, OrigThunk->u1.AddressOfData );

        if ( 0 == strcmp( ImportedProcName,
                              ( char* ) import->Name ) )
        {
          //
          // Proc found, patch it.
          //
          DWORD junk;
          MEMORY_BASIC_INFORMATION thunkMemInfo;

          //
          // Make page writable.
          //
          VirtualQuery(
            Thunk,
            &thunkMemInfo,
            sizeof( MEMORY_BASIC_INFORMATION ) );
          if ( ! VirtualProtect(
            thunkMemInfo.BaseAddress,
            thunkMemInfo.RegionSize,
            PAGE_EXECUTE_READWRITE,
            &thunkMemInfo.Protect ) )
          {
            return HRESULT_FROM_WIN32( GetLastError() );
          }

          //
          // Replace function pointers (non-atomically).
          //
          if ( OldProc )
          {
            *OldProc = ( PVOID ) ( DWORD_PTR )
                Thunk->u1.Function;
          }
#ifdef _WIN64
          Thunk->u1.Function = ( ULONGLONG ) ( DWORD_PTR )
              AlternateProc;
#else
          Thunk->u1.Function = ( DWORD ) ( DWORD_PTR )
              AlternateProc;
#endif
          //
          // Restore page protection.
          //
          if ( ! VirtualProtect(
            thunkMemInfo.BaseAddress,
            thunkMemInfo.RegionSize,
            thunkMemInfo.Protect,
            &junk ) )
          {
            return HRESULT_FROM_WIN32( GetLastError() );
          }

          return S_OK;
        }
      }

      //
      // Import not found.
      //
      return HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );    
    }
  }

  //
  // DLL not found.
  //
  return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
}
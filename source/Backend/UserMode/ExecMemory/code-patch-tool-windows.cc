#include "dobby/dobby_internal.h"

#include <windows.h>

using namespace zz;

PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
  DWORD oldProtect;
  int page_size;

  // Get page size
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  page_size = si.dwPageSize;

  void *addressPageAlign = (void *)ALIGN(address, page_size);

  HANDLE hProc = NULL;

  if (!VirtualProtect(addressPageAlign, page_size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    // Some games fiddles with VirtualProtect
    DWORD pid = GetCurrentProcessId();
    hProc = OpenProcess(PROCESS_VM_OPERATION, FALSE, pid);

    if (!VirtualProtectEx(hProc, addressPageAlign, pageSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
      CloseHandle(hProc);
      return -1;
    }
  }

  memcpy(address, buffer, buffer_size);

  if (hProc) {
    BOOL ok = VirtualProtectEx(hProc, addressPageAlign, pageSize, oldProtect, &oldProtect);
    CloseHandle(hProc);

    if (!ok)
      return -1;
  } else if (!VirtualProtect(addressPageAlign, pageSize, oldProtect, &oldProtect)) {
    return -1;
  }

  return 0;
}

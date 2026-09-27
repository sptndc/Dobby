#include "PlatformThread.h"

int zz::OSThread::GetCurrentProcessId() {
  return 0;
}

int zz::OSThread::GetCurrentThreadId() {
  return 0;
}

zz::OSThread::LocalStorageKey zz::OSThread::CreateThreadLocalKey() {
  return 0;
}

void zz::OSThread::DeleteThreadLocalKey(LocalStorageKey key) {
}

void *zz::OSThread::GetThreadLocal(LocalStorageKey key) {
  return NULL;
}

void zz::OSThread::SetThreadLocal(LocalStorageKey key, void *value) {
}

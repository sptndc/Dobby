#include "PlatformThread.h"

#include <unistd.h>  // getpid
#include <pthread.h> // pthread
#include <sys/syscall.h>

int zz::OSThread::GetCurrentProcessId() {
  return static_cast<int>(getpid());
}

int zz::OSThread::GetCurrentThreadId() {
#if defined(__APPLE__)
  return static_cast<int>(pthread_mach_thread_np(pthread_self()));
#elif defined(__ANDROID__)
  return static_cast<int>(gettid());
#elif defined(__linux__)
  return static_cast<int>(syscall(__NR_gettid));
#else
  return static_cast<int>(reinterpret_cast<intptr_t>(pthread_self()));
#endif
}

static zz::OSThread::LocalStorageKey PthreadKeyToLocalKey(pthread_key_t pthread_key) {
#if defined(__cygwin__)
  // We need to cast pthread_key_t to zz::OSThread::LocalStorageKey in two steps
  // because pthread_key_t is a pointer type on Cygwin. This will probably not
  // work on 64-bit platforms, but Cygwin doesn't support 64-bit anyway.
  assert(sizeof(zz::OSThread::LocalStorageKey) == sizeof(pthread_key_t));
  intptr_t ptr_key = reinterpret_cast<intptr_t>(pthread_key);
  return static_cast<zz::OSThread::LocalStorageKey>(ptr_key);
#else
  return static_cast<zz::OSThread::LocalStorageKey>(pthread_key);
#endif
}

static pthread_key_t LocalKeyToPthreadKey(zz::OSThread::LocalStorageKey local_key) {
#if defined(__cygwin__)
  assert(sizeof(zz::OSThread::LocalStorageKey) == sizeof(pthread_key_t));
  intptr_t ptr_key = static_cast<intptr_t>(local_key);
  return reinterpret_cast<pthread_key_t>(ptr_key);
#else
  return static_cast<pthread_key_t>(local_key);
#endif
}

zz::OSThread::LocalStorageKey zz::OSThread::CreateThreadLocalKey() {
  pthread_key_t key;
  int result = pthread_key_create(&key, nullptr);
  DCHECK_EQ(0, result);
  LocalStorageKey local_key = PthreadKeyToLocalKey(key);
  return local_key;
}

void zz::OSThread::DeleteThreadLocalKey(LocalStorageKey key) {
  pthread_key_t pthread_key = LocalKeyToPthreadKey(key);
  int result = pthread_key_delete(pthread_key);
  DCHECK_EQ(0, result);
}

void *zz::OSThread::GetThreadLocal(LocalStorageKey key) {
  pthread_key_t pthread_key = LocalKeyToPthreadKey(key);
  return pthread_getspecific(pthread_key);
}

void zz::OSThread::SetThreadLocal(LocalStorageKey key, void *value) {
  pthread_key_t pthread_key = LocalKeyToPthreadKey(key);
  int result = pthread_setspecific(pthread_key, value);
  DCHECK_EQ(0, result);
}

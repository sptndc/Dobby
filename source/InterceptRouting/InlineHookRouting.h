#pragma once

#include "dobby/common.h"
#include "InterceptRouting/InterceptRouting.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

struct InlineHookRouting : InterceptRouting {
  addr_t fake_func;

  InlineHookRouting(Interceptor::Entry *entry, addr_t fake_func) : InterceptRouting(entry), fake_func(fake_func) {
    entry->routing = this;
  }

  ~InlineHookRouting() = default;

  addr_t TrampolineTarget() override {
    __FUNC_CALL_TRACE__();

    return this->fake_func;
  }

  void SetFakeFunc(addr_t new_fake_func) {
    __FUNC_CALL_TRACE__();

    if (this->trampoline) {
      delete this->trampoline;
      this->trampoline = 0;
    }

    if (this->near_trampoline) {
      delete this->near_trampoline;
      this->near_trampoline = 0;
    }

    this->fake_func = new_fake_func;
    GenerateTrampoline();
  }

  void BuildRouting() {
    __FUNC_CALL_TRACE__();

    GenerateTrampoline();

    GenerateRelocatedCode();

    BackupOriginCode();
  }
};

PUBLIC inline int DobbyHook(void *address, void *fake_func, void **out_origin_func) {
  __FUNC_CALL_TRACE__();

  int result = DobbyPrepare(address, fake_func, out_origin_func);

  if (result != 0)
    return result;

  return DobbyCommit(address);
}

PUBLIC inline int DobbyPrepare(void *address, void *fake_func, void **out_origin_func) {
  __FUNC_CALL_TRACE__();

  if (!address) {
    ERROR_LOG("address is 0x0");
    return -1;
  }

  features::apple::arm64e_pac_strip(address);
  features::apple::arm64e_pac_strip(fake_func);
  features::android::make_memory_readable(address, 4);

  DEBUG_LOG("----- [DobbyPrepare: %p] -----", address);

  // check if already hooked
  auto entry = gInterceptor.find((addr_t)address);

  if (entry) {
    auto routing = (InlineHookRouting *)entry->routing;

    if (routing->TrampolineTarget() == (addr_t)fake_func) {
      ERROR_LOG("%p already been hooked.", address);
      return -1;
    }
  }

  // check if was previously hooked and rehook if needed
  entry = gOrigInterceptor.find((addr_t)address);

  if (entry) {
    auto routing = (InlineHookRouting *)entry->routing;

    if (routing->TrampolineTarget() != (addr_t)fake_func) {
      // no need to regenerate relocated buffer because its size and contents are the same
      routing->SetFakeFunc((addr_t)fake_func);
    }

    gInterceptor.add(entry);

    if (out_origin_func) {
      *out_origin_func = (void *)entry->relocated.addr();
    }

    return 0;
  }

  entry = new Interceptor::Entry((addr_t)address);
  entry->id = gInterceptor.count();
  entry->fake_func_addr = (addr_t)fake_func;

  auto routing = new InlineHookRouting(entry, (addr_t)fake_func);
  routing->BuildRouting();

  gInterceptor.add(entry);

  if (out_origin_func) {
    *out_origin_func = (void *)entry->relocated.addr();
  }

  features::apple::arm64e_pac_strip_and_sign(*out_origin_func);

  return 0;
}

PUBLIC inline int DobbyCommit(void *address) {
  __FUNC_CALL_TRACE__();

  if (!address) {
    ERROR_LOG("address is 0x0");
    return -1;
  }

  features::arm_thumb_fix_addr(address);
  features::apple::arm64e_pac_strip(address);
  features::android::make_memory_readable(address, 4);

  // check if already hooked
  auto entry = gInterceptor.find((addr_t)address);

  if (entry->is_commited) {
    ERROR_LOG("%p already been hooked.", address);
    return -1;
  }

  auto routing = (InlineHookRouting *)entry->routing;
  routing->Active();

  if (routing->error) {
    ERROR_LOG("build routing error.");
    return -1;
  }

  return 0;
}

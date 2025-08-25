#ifndef SHARE_RUNTIME_NMETHODGROUPER_HPP
#define SHARE_RUNTIME_NMETHODGROUPER_HPP

#include "memory/allStatic.hpp"
#include "utilities/linkedlist.hpp"
#include "runtime/nonJavaThread.hpp"

using NMethodList = LinkedListImpl<nmethod*>;
using NMethodListIterator = LinkedListIterator<nmethod*>;
using NMethodMap = ResizeableResourceHashtable<nmethod*, int64_t, AnyObj::C_HEAP, mtCode>;

class NMethodGrouper : public AllStatic {
 private:
  static void group_nmethods();
  static bool is_code_cache_unstable() {
    // Placeholder for actual implementation to check if the code cache is unstable.
    return false; // For now, we assume the code cache is stable.
  }

  static NonJavaThread *_nmethod_grouper_thread;
  static NMethodList _unregistered_nmethods;
  static bool _is_initialized;

 public:
  static void group_nmethods_loop();
  static void initialize();
  static void unregister_nmethod(nmethod* nm);
  static void register_nmethod(nmethod* nm);

  static const int64_t maxNotSeenInProfiles = 8;
  static NMethodMap _hot_nmethods;
};

#endif // SHARE_RUNTIME_NMETHODGROUPER_HPP
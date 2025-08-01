/*
 * Copyright (c) 1997, 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "gc/shared/gc_globals.hpp"
#include "logging/log.hpp"
#include "logging/logStream.hpp"
#include "memory/resourceArea.hpp"
#include "memory/universe.hpp"
#include "runtime/javaThread.hpp"
#include "runtime/mutexLocker.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/vmThread.hpp"
#include "utilities/vmError.hpp"

// Mutexes used in the VM (see comment in mutexLocker.hpp):

Mutex*   NMethodState_lock            = nullptr;
Mutex*   NMethodEntryBarrier_lock     = nullptr;
Monitor* SystemDictionary_lock        = nullptr;
Mutex*   InvokeMethodTypeTable_lock   = nullptr;
Monitor* InvokeMethodIntrinsicTable_lock = nullptr;
Mutex*   SharedDictionary_lock        = nullptr;
Monitor* ClassInitError_lock          = nullptr;
Mutex*   Module_lock                  = nullptr;
Mutex*   CompiledIC_lock              = nullptr;
Mutex*   VMStatistic_lock             = nullptr;
Mutex*   JmethodIdCreation_lock       = nullptr;
Mutex*   JfieldIdCreation_lock        = nullptr;
Monitor* JNICritical_lock             = nullptr;
Mutex*   JvmtiThreadState_lock        = nullptr;
Monitor* EscapeBarrier_lock           = nullptr;
Monitor* JvmtiVTMSTransition_lock     = nullptr;
Mutex*   JvmtiVThreadSuspend_lock     = nullptr;
Monitor* Heap_lock                    = nullptr;
#if INCLUDE_PARALLELGC
Mutex*   PSOldGenExpand_lock      = nullptr;
#endif
Mutex*   AdapterHandlerLibrary_lock   = nullptr;
Mutex*   SignatureHandlerLibrary_lock = nullptr;
Mutex*   VtableStubs_lock             = nullptr;
Mutex*   SymbolArena_lock             = nullptr;
Monitor* StringDedup_lock             = nullptr;
Mutex*   StringDedupIntern_lock       = nullptr;
Monitor* CodeCache_lock               = nullptr;
Mutex*   TouchedMethodLog_lock        = nullptr;
Mutex*   RetData_lock                 = nullptr;
Monitor* VMOperation_lock             = nullptr;
Monitor* ThreadsLockThrottle_lock     = nullptr;
Monitor* Threads_lock                 = nullptr;
Mutex*   NonJavaThreadsList_lock      = nullptr;
Mutex*   NonJavaThreadsListSync_lock  = nullptr;
Monitor* STS_lock                     = nullptr;
Mutex*   MonitoringSupport_lock       = nullptr;
Monitor* ConcurrentGCBreakpoints_lock = nullptr;
Mutex*   Compile_lock                 = nullptr;
Monitor* CompileTaskWait_lock         = nullptr;
Monitor* MethodCompileQueue_lock      = nullptr;
Monitor* CompileThread_lock           = nullptr;
Monitor* Compilation_lock             = nullptr;
Mutex*   CompileStatistics_lock       = nullptr;
Mutex*   DirectivesStack_lock         = nullptr;
Monitor* Terminator_lock              = nullptr;
Monitor* InitCompleted_lock           = nullptr;
Monitor* BeforeExit_lock              = nullptr;
Monitor* Notify_lock                  = nullptr;
Mutex*   ExceptionCache_lock          = nullptr;
Mutex*   TrainingData_lock            = nullptr;
Monitor* TrainingReplayQueue_lock     = nullptr;
#ifndef PRODUCT
Mutex*   FullGCALot_lock              = nullptr;
#endif

Mutex*   tty_lock                     = nullptr;

Mutex*   RawMonitor_lock              = nullptr;
Mutex*   PerfDataMemAlloc_lock        = nullptr;
Mutex*   PerfDataManager_lock         = nullptr;

Monitor* G1CGC_lock                   = nullptr;
Mutex*   G1FreeList_lock              = nullptr;
Mutex*   G1OldSets_lock               = nullptr;
Mutex*   G1Uncommit_lock              = nullptr;
Monitor* G1RootRegionScan_lock        = nullptr;
Monitor* G1OldGCCount_lock            = nullptr;
Mutex*   G1RareEvent_lock             = nullptr;
Mutex*   G1DetachedRefinementStats_lock = nullptr;
Mutex*   G1MarkStackFreeList_lock     = nullptr;
Mutex*   G1MarkStackChunkList_lock    = nullptr;

Mutex*   Management_lock              = nullptr;
Monitor* MonitorDeflation_lock        = nullptr;
Monitor* Service_lock                 = nullptr;
Monitor* Notification_lock            = nullptr;
Monitor* PeriodicTask_lock            = nullptr;
Monitor* RedefineClasses_lock         = nullptr;
Mutex*   Verify_lock                  = nullptr;

#if INCLUDE_JFR
Mutex*   JfrStacktrace_lock           = nullptr;
Monitor* JfrMsg_lock                  = nullptr;
Mutex*   JfrBuffer_lock               = nullptr;
#endif

Mutex*   CodeHeapStateAnalytics_lock  = nullptr;

Mutex*   ExternalsRecorder_lock       = nullptr;

Mutex*   AOTCodeCStrings_lock         = nullptr;

Monitor* ContinuationRelativize_lock  = nullptr;

Monitor* NMethodGrouper_lock          = nullptr;

Mutex*   Metaspace_lock               = nullptr;
Monitor* MetaspaceCritical_lock       = nullptr;
Mutex*   ClassLoaderDataGraph_lock    = nullptr;
Monitor* ThreadsSMRDelete_lock        = nullptr;
Mutex*   ThreadIdTableCreate_lock     = nullptr;
Mutex*   SharedDecoder_lock           = nullptr;
Mutex*   DCmdFactory_lock             = nullptr;
Mutex*   NMTQuery_lock                = nullptr;
Mutex*   NMTCompilationCostHistory_lock = nullptr;
Mutex*   NmtVirtualMemory_lock          = nullptr;

#if INCLUDE_CDS
#if INCLUDE_JVMTI
Mutex*   CDSClassFileStream_lock      = nullptr;
#endif
Mutex*   DumpTimeTable_lock           = nullptr;
Mutex*   CDSLambda_lock               = nullptr;
Mutex*   DumpRegion_lock              = nullptr;
Mutex*   ClassListFile_lock           = nullptr;
Mutex*   UnregisteredClassesTable_lock= nullptr;
Mutex*   LambdaFormInvokers_lock      = nullptr;
Mutex*   ScratchObjects_lock          = nullptr;
Mutex*   FinalImageRecipes_lock       = nullptr;
#endif // INCLUDE_CDS
Mutex*   Bootclasspath_lock           = nullptr;

#if INCLUDE_JVMCI
Monitor* JVMCI_lock                   = nullptr;
Monitor* JVMCIRuntime_lock            = nullptr;
#endif

// Only one RecursiveMutex
RecursiveMutex* MultiArray_lock       = nullptr;

#ifdef ASSERT
void assert_locked_or_safepoint(const Mutex* lock) {
  if (DebuggingContext::is_enabled() || VMError::is_error_reported()) return;
  // check if this thread owns the lock (common case)
  assert(lock != nullptr, "Need non-null lock");
  if (lock->owned_by_self()) return;
  if (SafepointSynchronize::is_at_safepoint()) return;
  if (!Universe::is_fully_initialized()) return;
  fatal("must own lock %s", lock->name());
}

// a stronger assertion than the above
void assert_lock_strong(const Mutex* lock) {
  if (DebuggingContext::is_enabled() || VMError::is_error_reported()) return;
  assert(lock != nullptr, "Need non-null lock");
  if (lock->owned_by_self()) return;
  fatal("must own lock %s", lock->name());
}
#endif

#define MUTEX_STORAGE_NAME(name) name##_storage
#define MUTEX_STORAGE(name, type) alignas(type) static uint8_t MUTEX_STORAGE_NAME(name)[sizeof(type)]
#define MUTEX_DEF(name, type, pri, ...) {                                                       \
  assert(name == nullptr, "Mutex/Monitor initialized twice");                                   \
  MUTEX_STORAGE(name, type);                                                                    \
  name = ::new(static_cast<void*>(MUTEX_STORAGE_NAME(name))) type((pri), #name, ##__VA_ARGS__); \
  Mutex::add_mutex(name);                                                                       \
}
#define MUTEX_DEFN(name, type, pri, ...) MUTEX_DEF(name, type, Mutex::pri, ##__VA_ARGS__)

// Specify relative ranked lock
#ifdef ASSERT
#define MUTEX_DEFL(name, type, held_lock, ...) MUTEX_DEF(name, type, (held_lock)->rank() - 1, ##__VA_ARGS__)
#else
#define MUTEX_DEFL(name, type, held_lock, ...) MUTEX_DEFN(name, type, safepoint, ##__VA_ARGS__)
#endif

// Using Padded subclasses to prevent false sharing of these global monitors and mutexes.
void mutex_init() {
  MUTEX_DEFN(tty_lock                        , PaddedMutex  , tty);      // allow to lock in VM

  MUTEX_DEFN(NMethodEntryBarrier_lock        , PaddedMutex  , service-1);

  MUTEX_DEFN(STS_lock                        , PaddedMonitor, nosafepoint);

  if (UseG1GC) {
    MUTEX_DEFN(G1CGC_lock                    , PaddedMonitor, nosafepoint);

    MUTEX_DEFN(G1DetachedRefinementStats_lock, PaddedMutex  , nosafepoint-2);

    MUTEX_DEFN(G1FreeList_lock               , PaddedMutex  , service-1);
    MUTEX_DEFN(G1OldSets_lock                , PaddedMutex  , nosafepoint);
    MUTEX_DEFN(G1Uncommit_lock               , PaddedMutex  , service-2);
    MUTEX_DEFN(G1RootRegionScan_lock         , PaddedMonitor, nosafepoint-1);

    MUTEX_DEFN(G1MarkStackFreeList_lock      , PaddedMutex  , nosafepoint);
    MUTEX_DEFN(G1MarkStackChunkList_lock     , PaddedMutex  , nosafepoint);
  }
  MUTEX_DEFN(MonitoringSupport_lock          , PaddedMutex  , service-1);        // used for serviceability monitoring support

  MUTEX_DEFN(StringDedup_lock                , PaddedMonitor, nosafepoint);
  MUTEX_DEFN(StringDedupIntern_lock          , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(RawMonitor_lock                 , PaddedMutex  , nosafepoint-1);

  MUTEX_DEFN(Metaspace_lock                  , PaddedMutex  , nosafepoint-3);
  MUTEX_DEFN(MetaspaceCritical_lock          , PaddedMonitor, nosafepoint-1);

  MUTEX_DEFN(MonitorDeflation_lock           , PaddedMonitor, nosafepoint);      // used for monitor deflation thread operations
  MUTEX_DEFN(Service_lock                    , PaddedMonitor, service);          // used for service thread operations
  MUTEX_DEFN(Notification_lock               , PaddedMonitor, service);          // used for notification thread operations

  MUTEX_DEFN(JmethodIdCreation_lock          , PaddedMutex  , nosafepoint-1);    // used for creating jmethodIDs can also lock HandshakeState_lock
  MUTEX_DEFN(InvokeMethodTypeTable_lock      , PaddedMutex  , safepoint);
  MUTEX_DEFN(InvokeMethodIntrinsicTable_lock , PaddedMonitor, safepoint);
  MUTEX_DEFN(AdapterHandlerLibrary_lock      , PaddedMutex  , safepoint);
  MUTEX_DEFN(SharedDictionary_lock           , PaddedMutex  , safepoint);
  MUTEX_DEFN(VMStatistic_lock                , PaddedMutex  , safepoint);
  MUTEX_DEFN(SignatureHandlerLibrary_lock    , PaddedMutex  , safepoint);
  MUTEX_DEFN(SymbolArena_lock                , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(ExceptionCache_lock             , PaddedMutex  , safepoint);
#ifndef PRODUCT
  MUTEX_DEFN(FullGCALot_lock                 , PaddedMutex  , safepoint); // a lock to make FullGCALot MT safe
#endif
  MUTEX_DEFN(BeforeExit_lock                 , PaddedMonitor, safepoint);

  MUTEX_DEFN(NonJavaThreadsList_lock         , PaddedMutex  , nosafepoint-1);
  MUTEX_DEFN(NonJavaThreadsListSync_lock     , PaddedMutex  , nosafepoint);

  MUTEX_DEFN(RetData_lock                    , PaddedMutex  , safepoint);
  MUTEX_DEFN(Terminator_lock                 , PaddedMonitor, safepoint, true);
  MUTEX_DEFN(InitCompleted_lock              , PaddedMonitor, nosafepoint);
  MUTEX_DEFN(Notify_lock                     , PaddedMonitor, safepoint, true);

  MUTEX_DEFN(JfieldIdCreation_lock           , PaddedMutex  , safepoint);

  MUTEX_DEFN(CompiledIC_lock                 , PaddedMutex  , nosafepoint);  // locks VtableStubs_lock
  MUTEX_DEFN(MethodCompileQueue_lock         , PaddedMonitor, safepoint);
  MUTEX_DEFL(TrainingData_lock               , PaddedMutex  , MethodCompileQueue_lock);
  MUTEX_DEFN(TrainingReplayQueue_lock        , PaddedMonitor, safepoint);
  MUTEX_DEFN(CompileStatistics_lock          , PaddedMutex  , safepoint);
  MUTEX_DEFN(DirectivesStack_lock            , PaddedMutex  , nosafepoint);

  MUTEX_DEFN(JvmtiVTMSTransition_lock        , PaddedMonitor, safepoint);   // used for Virtual Thread Mount State transition management
  MUTEX_DEFN(JvmtiVThreadSuspend_lock        , PaddedMutex,   nosafepoint-1);
  MUTEX_DEFN(EscapeBarrier_lock              , PaddedMonitor, nosafepoint); // Used to synchronize object reallocation/relocking triggered by JVMTI
  MUTEX_DEFN(Management_lock                 , PaddedMutex  , safepoint);   // used for JVM management

  MUTEX_DEFN(ConcurrentGCBreakpoints_lock    , PaddedMonitor, safepoint, true);
  MUTEX_DEFN(TouchedMethodLog_lock           , PaddedMutex  , safepoint);

  MUTEX_DEFN(CompileThread_lock              , PaddedMonitor, safepoint);
  MUTEX_DEFN(PeriodicTask_lock               , PaddedMonitor, safepoint, true);
  MUTEX_DEFN(RedefineClasses_lock            , PaddedMonitor, safepoint);
  MUTEX_DEFN(Verify_lock                     , PaddedMutex  , safepoint);
  MUTEX_DEFN(ClassLoaderDataGraph_lock       , PaddedMutex  , safepoint);

  if (WhiteBoxAPI) {
    MUTEX_DEFN(Compilation_lock              , PaddedMonitor, nosafepoint);
  }

#if INCLUDE_JFR
  MUTEX_DEFN(JfrBuffer_lock                  , PaddedMutex  , event);
  MUTEX_DEFN(JfrMsg_lock                     , PaddedMonitor, event);
  MUTEX_DEFN(JfrStacktrace_lock              , PaddedMutex  , event);
#endif

  MUTEX_DEFN(ContinuationRelativize_lock     , PaddedMonitor, nosafepoint-3);
  MUTEX_DEFN(CodeHeapStateAnalytics_lock     , PaddedMutex  , safepoint);
  MUTEX_DEFN(ThreadsSMRDelete_lock           , PaddedMonitor, service-2); // Holds ConcurrentHashTableResize_lock
  MUTEX_DEFN(ThreadIdTableCreate_lock        , PaddedMutex  , safepoint);
  MUTEX_DEFN(DCmdFactory_lock                , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(NMTQuery_lock                   , PaddedMutex  , safepoint);
  MUTEX_DEFN(NMTCompilationCostHistory_lock  , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(NmtVirtualMemory_lock           , PaddedMutex  , service-4); // Must be lower than G1Mapper_lock used from G1RegionsSmallerThanCommitSizeMapper::commit_regions
#if INCLUDE_CDS
#if INCLUDE_JVMTI
  MUTEX_DEFN(CDSClassFileStream_lock         , PaddedMutex  , safepoint);
#endif
  MUTEX_DEFN(DumpTimeTable_lock              , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(CDSLambda_lock                  , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(DumpRegion_lock                 , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(ClassListFile_lock              , PaddedMutex  , nosafepoint);
  MUTEX_DEFN(UnregisteredClassesTable_lock   , PaddedMutex  , nosafepoint-1);
  MUTEX_DEFN(LambdaFormInvokers_lock         , PaddedMutex  , safepoint);
  MUTEX_DEFN(ScratchObjects_lock             , PaddedMutex  , nosafepoint-1); // Holds DumpTimeTable_lock
  MUTEX_DEFN(FinalImageRecipes_lock          , PaddedMutex  , nosafepoint);
#endif // INCLUDE_CDS
  MUTEX_DEFN(Bootclasspath_lock              , PaddedMutex  , nosafepoint);

#if INCLUDE_JVMCI
  // JVMCIRuntime::_lock must be acquired before JVMCI_lock to avoid deadlock
  MUTEX_DEFN(JVMCIRuntime_lock               , PaddedMonitor, safepoint, true);
#endif

  MUTEX_DEFN(ThreadsLockThrottle_lock        , PaddedMonitor, safepoint);

  // These locks have relative rankings, and inherit safepoint checking attributes from that rank.
  MUTEX_DEFL(VtableStubs_lock               , PaddedMutex  , CompiledIC_lock);  // Also holds DumpTimeTable_lock
  MUTEX_DEFL(CodeCache_lock                 , PaddedMonitor, VtableStubs_lock);
  MUTEX_DEFL(NMethodState_lock              , PaddedMutex  , CodeCache_lock);

  // tty_lock is held when printing nmethod and its relocations which use this lock.
  MUTEX_DEFL(ExternalsRecorder_lock         , PaddedMutex  , tty_lock);

  MUTEX_DEFL(AOTCodeCStrings_lock           , PaddedMutex  , tty_lock);

  MUTEX_DEFL(Threads_lock                   , PaddedMonitor, CompileThread_lock, true);
  MUTEX_DEFL(Compile_lock                   , PaddedMutex  , MethodCompileQueue_lock);
  MUTEX_DEFL(JNICritical_lock               , PaddedMonitor, AdapterHandlerLibrary_lock); // used for JNI critical regions
  MUTEX_DEFL(Heap_lock                      , PaddedMonitor, JNICritical_lock);

  MUTEX_DEFL(PerfDataMemAlloc_lock          , PaddedMutex  , Heap_lock);
  MUTEX_DEFL(PerfDataManager_lock           , PaddedMutex  , Heap_lock);
  MUTEX_DEFL(VMOperation_lock               , PaddedMonitor, Heap_lock, true);
  MUTEX_DEFL(ClassInitError_lock            , PaddedMonitor, Threads_lock);

  if (UseG1GC) {
    MUTEX_DEFL(G1OldGCCount_lock            , PaddedMonitor, Threads_lock, true);
    MUTEX_DEFL(G1RareEvent_lock             , PaddedMutex  , Threads_lock, true);
  }

  MUTEX_DEFL(CompileTaskWait_lock           , PaddedMonitor, MethodCompileQueue_lock);

#if INCLUDE_PARALLELGC
  if (UseParallelGC) {
    MUTEX_DEFL(PSOldGenExpand_lock          , PaddedMutex  , Heap_lock, true);
  }
#endif
  MUTEX_DEFL(Module_lock                    , PaddedMutex  ,  ClassLoaderDataGraph_lock);
  MUTEX_DEFL(SystemDictionary_lock          , PaddedMonitor, Module_lock);
#if INCLUDE_JVMCI
  // JVMCIRuntime_lock must be acquired before JVMCI_lock to avoid deadlock
  MUTEX_DEFL(JVMCI_lock                     , PaddedMonitor, JVMCIRuntime_lock);
#endif
  MUTEX_DEFL(JvmtiThreadState_lock          , PaddedMutex  , JvmtiVTMSTransition_lock);   // Used by JvmtiThreadState/JvmtiEventController
  MUTEX_DEFL(SharedDecoder_lock             , PaddedMutex  , NmtVirtualMemory_lock); // Must be lower than NmtVirtualMemory_lock due to MemTracker::print_containing_region

  MUTEX_DEFL(NMethodGrouper_lock            , PaddedMonitor, CodeCache_lock);

  // Allocate RecursiveMutex
  MultiArray_lock = new RecursiveMutex();
}

#undef MUTEX_DEFL
#undef MUTEX_DEFN
#undef MUTEX_DEF
#undef MUTEX_STORAGE
#undef MUTEX_STORAGE_NAME

void MutexLockerImpl::post_initialize() {
  // Print mutex ranks if requested.
  LogTarget(Info, vmmutex) lt;
  if (lt.is_enabled()) {
    ResourceMark rm;
    LogStream ls(lt);
    Mutex::print_lock_ranks(&ls);
  }
}

GCMutexLocker::GCMutexLocker(Mutex* mutex) {
  if (SafepointSynchronize::is_at_safepoint()) {
    _locked = false;
  } else {
    _mutex = mutex;
    _locked = true;
    _mutex->lock();
  }
}

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

#include "classfile/stringTable.hpp"
#include "classfile/symbolTable.hpp"
#include "code/aotCodeCache.hpp"
#include "compiler/compiler_globals.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "gc/shared/gcHeapSummary.hpp"
#include "interpreter/bytecodes.hpp"
#include "logging/logAsyncWriter.hpp"
#include "memory/universe.hpp"
#include "nmt/memTracker.hpp"
#include "oops/trainingData.hpp"
#include "prims/downcallLinker.hpp"
#include "prims/jvmtiExport.hpp"
#include "prims/methodHandles.hpp"
#include "runtime/atomic.hpp"
#include "runtime/continuation.hpp"
#include "runtime/flags/jvmFlag.hpp"
#include "runtime/globals.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/icache.hpp"
#include "runtime/init.hpp"
#include "runtime/nmethodGrouper.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/sharedRuntime.hpp"
#include "sanitizers/leak.hpp"
#include "utilities/macros.hpp"
#if INCLUDE_JVMCI
#include "jvmci/jvmci.hpp"
#endif

// Initialization done by VM thread in vm_init_globals()
void check_ThreadShadow();
void eventlog_init();
void mutex_init();
void universe_oopstorage_init();
void perfMemory_init();
void SuspendibleThreadSet_init();
void ExternalsRecorder_init(); // After mutex_init() and before CodeCache_init

// Initialization done by Java thread in init_globals()
void management_init();
void bytecodes_init();
void classLoader_init1();
void compilationPolicy_init();
void codeCache_init();
void VM_Version_init();
void icache_init2();
void initialize_stub_info();    // must precede all blob/stub generation
void preuniverse_stubs_init();
void initial_stubs_init();

jint universe_init();           // depends on codeCache_init and preuniverse_stubs_init
// depends on universe_init, must be before interpreter_init (currently only on SPARC)
void gc_barrier_stubs_init();
void continuations_init();      // depends on flags (UseCompressedOops) and barrier sets
void continuation_stubs_init(); // depend on continuations_init
void interpreter_init_stub();   // before any methods loaded
void interpreter_init_code();   // after methods loaded, but before they are linked
void accessFlags_init();
void InterfaceSupport_init();
void universe2_init();  // dependent on codeCache_init and initial_stubs_init, loads primordial classes
void referenceProcessor_init();
void jni_handles_init();
void vmStructs_init() NOT_DEBUG_RETURN;

void vtableStubs_init();
bool compilerOracle_init();
bool compileBroker_init();
void dependencyContext_init();
void dependencies_init();

// Initialization after compiler initialization
bool universe_post_init();  // must happen after compiler_init
void javaClasses_init();    // must happen after vtable initialization
void compiler_stubs_init(bool in_compiler_thread); // compiler's StubRoutines stubs
void final_stubs_init();    // final StubRoutines stubs

// Do not disable thread-local-storage, as it is important for some
// JNI/JVM/JVMTI functions and signal handlers to work properly
// during VM shutdown
void perfMemory_exit();
void ostream_exit();

void vm_init_globals() {
  check_ThreadShadow();
  basic_types_init();
  eventlog_init();
  mutex_init();
  universe_oopstorage_init();
  perfMemory_init();
  SuspendibleThreadSet_init();
  ExternalsRecorder_init(); // After mutex_init() and before CodeCache_init
}


jint init_globals() {
  management_init();
  JvmtiExport::initialize_oop_storage();
#if INCLUDE_JVMTI
  if (AlwaysRecordEvolDependencies) {
    JvmtiExport::set_can_hotswap_or_post_breakpoint(true);
    JvmtiExport::set_all_dependencies_are_recorded(true);
  }
#endif
  bytecodes_init();
  classLoader_init1();
  compilationPolicy_init();
  codeCache_init();
  VM_Version_init();              // depends on codeCache_init for emitting code
  icache_init2();                 // depends on VM_Version for choosing the mechanism
  // ensure we know about all blobs, stubs and entries
  initialize_stub_info();
  // initialize stubs needed before we can init the universe
  preuniverse_stubs_init();
  jint status = universe_init();  // dependent on codeCache_init and preuniverse_stubs_init
  if (status != JNI_OK) {
    return status;
  }
#ifdef LEAK_SANITIZER
  {
    // Register the Java heap with LSan.
    VirtualSpaceSummary summary = Universe::heap()->create_heap_space_summary();
    LSAN_REGISTER_ROOT_REGION(summary.start(), summary.reserved_size());
  }
#endif // LEAK_SANITIZER
  AOTCodeCache::init2();     // depends on universe_init, must be before initial_stubs_init
  AsyncLogWriter::initialize();

  initial_stubs_init();      // stubgen initial stub routines
  // stack overflow exception blob is referenced by the interpreter
  AOTCodeCache::init_early_stubs_table();  // need this after stubgen initial stubs and before shared runtime initial stubs
  SharedRuntime::generate_initial_stubs();
  gc_barrier_stubs_init();   // depends on universe_init, must be before interpreter_init
  continuations_init();      // must precede continuation stub generation
  continuation_stubs_init(); // depends on continuations_init
#if INCLUDE_JFR
  SharedRuntime::generate_jfr_stubs();
#endif
  interpreter_init_stub();   // before methods get loaded
  accessFlags_init();
  InterfaceSupport_init();
  VMRegImpl::set_regName();  // need this before generate_stubs (for printing oop maps).
  SharedRuntime::generate_stubs();
  AOTCodeCache::init_shared_blobs_table();  // need this after generate_stubs
  SharedRuntime::init_adapter_library(); // do this after AOTCodeCache::init_shared_blobs_table
  return JNI_OK;
}

jint init_globals2() {
  universe2_init();          // dependent on codeCache_init and initial_stubs_init
  javaClasses_init();        // must happen after vtable initialization, before referenceProcessor_init
  interpreter_init_code();   // after javaClasses_init and before any method gets linked
  referenceProcessor_init();
  jni_handles_init();
#if INCLUDE_VM_STRUCTS
  vmStructs_init();
#endif // INCLUDE_VM_STRUCTS

  vtableStubs_init();
  if (!compilerOracle_init()) {
    return JNI_EINVAL;
  }
  dependencyContext_init();
  dependencies_init();

  if (!compileBroker_init()) {
    return JNI_EINVAL;
  }
#if INCLUDE_JVMCI
  if (EnableJVMCI) {
    JVMCI::initialize_globals();
  }
#endif

  // Initialize TrainingData only we're recording/replaying
  if (TrainingData::have_data() || TrainingData::need_data()) {
   TrainingData::initialize();
  }

  if (!universe_post_init()) {
    return JNI_ERR;
  }
  compiler_stubs_init(false /* in_compiler_thread */); // compiler's intrinsics stubs
  final_stubs_init();    // final StubRoutines stubs
  MethodHandles::generate_adapters();

  if (UseNewCode2) {
    NMethodGrouper::initialize();
  }

  // All the flags that get adjusted by VM_Version_init and os::init_2
  // have been set so dump the flags now.
  if (PrintFlagsFinal || PrintFlagsRanges) {
    JVMFlag::printFlags(tty, false, PrintFlagsRanges);
  }

  return JNI_OK;
}


void exit_globals() {
  static bool destructorsCalled = false;
  if (!destructorsCalled) {
    destructorsCalled = true;
    perfMemory_exit();
    SafepointTracing::statistics_exit_log();
    if (PrintStringTableStatistics) {
      SymbolTable::dump(tty);
      StringTable::dump(tty);
    }
    ostream_exit();
#ifdef LEAK_SANITIZER
    {
      // Unregister the Java heap with LSan.
      VirtualSpaceSummary summary = Universe::heap()->create_heap_space_summary();
      LSAN_UNREGISTER_ROOT_REGION(summary.start(), summary.reserved_size());
    }
#endif // LEAK_SANITIZER
  }
}

static volatile bool _init_completed = false;

bool is_init_completed() {
  return Atomic::load_acquire(&_init_completed);
}

void wait_init_completed() {
  MonitorLocker ml(InitCompleted_lock, Monitor::_no_safepoint_check_flag);
  while (!_init_completed) {
    ml.wait();
  }
}

void set_init_completed() {
  assert(Universe::is_fully_initialized(), "Should have completed initialization");
  MonitorLocker ml(InitCompleted_lock, Monitor::_no_safepoint_check_flag);
  Atomic::release_store(&_init_completed, true);
  ml.notify_all();
}

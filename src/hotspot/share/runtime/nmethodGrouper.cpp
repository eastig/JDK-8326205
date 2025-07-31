#include "code/codeCache.hpp"
#include "code/compiledIC.hpp"
#include "compiler/compilerDefinitions.inline.hpp"
#include "runtime/atomic.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/nmethodGrouper.hpp"
#include "runtime/suspendedThreadTask.hpp"
#include "runtime/os.hpp"
#include "runtime/threads.hpp"
#include "utilities/debug.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/resourceHash.hpp"

enum class ProfilingState {
  NotStarted,
  Running,
  Waiting
};

class C2NMethodGrouperThread : public NonJavaThread {
 public:
  void run() override {
    NMethodGrouper::group_nmethods_loop();
  }
  const char* name()      const override { return "C2 nmethod Grouper Thread"; }
  const char* type_name() const override { return "C2NMethodGrouperThread"; }
};

using UnregisteredNMethods = LinkedListImpl<nmethod*>;
using UnregisteredNMethodsIterator = LinkedListIterator<nmethod*>;

NonJavaThread *NMethodGrouper::_nmethod_grouper_thread = nullptr;
UnregisteredNMethods NMethodGrouper::_unregistered_nmethods;
bool NMethodGrouper::_is_initialized = false;

static volatile int _profiling_state = static_cast<int>(ProfilingState::NotStarted);

static volatile size_t _unregistered_c2_nmethods_count = 0;
static volatile size_t _new_c2_nmethods_count = 0;
static volatile size_t _total_c2_nmethods_count = 0;

static ProfilingState get_profiling_state() {
  return static_cast<ProfilingState>(Atomic::load_acquire(&_profiling_state));
}

static void set_profiling_state(ProfilingState new_state) {
  Atomic::release_store(&_profiling_state, static_cast<int>(new_state));
}

void NMethodGrouper::initialize() {
  if (!CompilerConfig::is_c2_enabled()) {
    return; // No C2 support, no need for nmethod grouping
  }
  _nmethod_grouper_thread = new C2NMethodGrouperThread();
  if (os::create_thread(_nmethod_grouper_thread, os::os_thread)) {
    os::start_thread(_nmethod_grouper_thread);
  } else {
    vm_exit_during_initialization("Failed to create C2 nmethod grouper thread");
  }
  _is_initialized = true;
}

/*static void wait_while_cpu_usage_low() {
  tty->print_cr("[NMethodGrouper]: Waiting for CPU activity to resume profiling...");
  constexpr int sleep_duration_sec = 50;
  while (true) {
    double cpu_time1 = os::elapsed_process_cpu_time();
    {
      MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
      if (!ml.wait(sleep_duration_sec * 1000)) {
        tty->print_cr("[NMethodGrouper]: Notified CodeCache being actively used, continuing profiling...");
        return;
      }
    }
    double cpu_time2 = os::elapsed_process_cpu_time();
    // Check if the CPU was active during the sleep period
    if ((cpu_time2 - cpu_time1) / sleep_duration_sec >= 1.0) {
      break;
    }
  }
  tty->print_cr("[NMethodGrouper]: CPU activity resumed, continuing profiling...");
}*/

void NMethodGrouper::group_nmethods_loop() {
  {
    tty->print_cr("[NMethodGrouper]: Waiting for C2 code size exceeding threshold for profiling...");
    MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
    ml.wait(0); // Wait without timeout until notified
    tty->print_cr("[NMethodGrouper]: C2 code size threshold exceeded, starting profiling...");
  }

  while (true) {
    // TODO: Need to find proper place to wait for CPU activity
    // wait_while_cpu_usage_low();
    set_profiling_state(ProfilingState::Running);
    group_nmethods();
    set_profiling_state(ProfilingState::Waiting);
    {
      MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
      ml.wait(0); // Wait without timeout until notified
    }
  }
}

static inline bool is_excluded(JavaThread* thread) {
  return (thread->is_hidden_from_external_view() ||
          thread->thread_state() != _thread_in_Java ||
          thread->in_deopt_handler());
}

class GetC2NMethodTask : public SuspendedThreadTask {
 public:
  nmethod *_nmethod;
  GetC2NMethodTask(JavaThread* thread) : SuspendedThreadTask(thread), _nmethod(nullptr) {}

  void do_task(const SuspendedThreadTaskContext& context) {
    JavaThread* jt = JavaThread::cast(context.thread());
    if (jt->thread_state() != _thread_in_Java) {
      return;
    }

    address pc = os::fetch_frame_from_context(context.ucontext(), nullptr, nullptr);

    if (pc != nullptr && !Interpreter::contains(pc) && CodeCache::contains(pc)) {
      const CodeBlob* const cb = CodeCache::find_blob_fast(pc);
      if (cb != nullptr && cb->is_nmethod()) {
        nmethod* nm = cb->as_nmethod();
        if (nm->is_compiled_by_c2() &&
            !nm->is_osr_method()  &&
            nm->is_in_use() &&
            !nm->is_marked_for_deoptimization() &&
            !nm->is_unloading()) {
          _nmethod = nm;
        }
      }
    }
  }
};

static inline int64_t get_monotonic_ms() {
  return os::javaTimeNanos() / 1000000;
}

static inline int64_t sampling_period_ms() {
  // This is the interval in milliseconds between samples.
  return 20;
}

static inline int min_samples() {
  return 3000; // Minimum number of samples to collect
}

using NMethodSamples = ResourceHashtable<nmethod*, int, 1024>;

class ThreadSampler : public StackObj {
 private:
  NMethodSamples _samples;
  int _total_samples;
  int _processed_threads;

 public:
  ThreadSampler() : _samples(), _total_samples(0), _processed_threads(0) {}

  void run() {
    MutexLocker ml(Threads_lock);

    for (JavaThreadIteratorWithHandle jtiwh; JavaThread *jt = jtiwh.next(); ) {
      if (is_excluded(jt)) {
        continue;
      }

      _processed_threads++;
      GetC2NMethodTask task(jt);
      task.run();
      if (task._nmethod != nullptr) {
        bool created = false;
        int *count = _samples.put_if_absent(task._nmethod, 0, &created);
        (*count)++;
        _total_samples++;
      }
    }
  }

  void collect_samples() {
    tty->print_cr("[NMethodGrouper]: Profiling nmethods");

    const int64_t period = sampling_period_ms();
    while (true) {
      const int64_t sampling_start = get_monotonic_ms();
      run();
      if (_total_samples >= min_samples()) {
        tty->print_cr("[NMethodGrouper]: Collected %d samples from %d threads", _total_samples, _processed_threads);
        break;
      }
      const int64_t next_sample =
          period - (get_monotonic_ms() - sampling_start);
      if (next_sample > 0) {
        os::naked_sleep(next_sample);
      }
    }
  }

  const NMethodSamples& samples() const {
    return _samples;
  }
  int total_samples() const {
    return _total_samples;
  }
  int processed_threads() const {
    return _processed_threads;
  }

  void exclude_unregistered_nmethods(const UnregisteredNMethods& unregistered);
};

void ThreadSampler::exclude_unregistered_nmethods(const UnregisteredNMethods& unregistered) {
  UnregisteredNMethodsIterator it(unregistered.head());
  while (!it.is_empty()) {
    nmethod* nm = *it.next();
    int* count = _samples.get(nm);
    if (count != nullptr) {
      _total_samples -= *count;
      *count = 0;
    }
  }
}

class HotCodeHeapCandidates : public StackObj {
 public:
  void find(const NMethodSamples& samples, int total_samples) {
    auto func = [&](nmethod* nm, int count) {
      double frequency = (double) count / total_samples;
      if (frequency < HotCodeMinMethodFrequency) {
        return true;
      }

      if (CodeCache::get_code_blob_type(nm) != CodeBlobType::MethodHot) {
        if (PrintCodeCache) {
          tty->print_cr("[NMethodGrouper]: \tRelocating nm: <%p> method: <%s> count: <%d> frequency: <%f>", nm, nm->method()->external_name(), count, frequency);
        }

        CompiledICLocker ic_locker(nm);
        nm->relocate(CodeBlobType::MethodHot);
      }
      return true;
    };
    samples.iterate(func);
  }

  void relocate() {}
};

void NMethodGrouper::group_nmethods() {
  ResourceMark rm;

  ThreadSampler sampler;
  sampler.collect_samples();

  {
    MutexLocker ml_Compile_lock(Compile_lock);
    MutexLocker ml_CompiledIC_lock(CompiledIC_lock, Mutex::_no_safepoint_check_flag);
    MutexLocker ml_CodeCache_lock(CodeCache_lock, Mutex::_no_safepoint_check_flag);
    int total_samples = sampler.total_samples();
    int processed_threads = sampler.processed_threads();
    tty->print_cr("[NMethodGrouper]: Profiling nmethods done: %d samples, %d nmethods, %d processed threads, %d unregistered nmethods",
       total_samples, sampler.samples().number_of_entries(),
       processed_threads, (int)_unregistered_nmethods.size());

    if (is_code_cache_unstable()) {
      tty->print_cr("[NMethodGrouper]: CodeCache is unstable, skipping nmethod grouping");
      return;
    }

    sampler.exclude_unregistered_nmethods(_unregistered_nmethods);
    tty->print_cr("[NMethodGrouper]: Total samples after excluding unregistered nmethods: %d",
       sampler.total_samples());
    _unregistered_nmethods.clear();

    // TODO: We might want to update nmethods GC status to prevent them from getting cold.

    HotCodeHeapCandidates candidates;
    candidates.find(sampler.samples(), sampler.total_samples());
    candidates.relocate();
  }
}

static size_t get_c2_code_size() {
  for (CodeHeap *ch : *CodeCache::nmethod_heaps()) {
    switch (ch->code_blob_type()) {
      case CodeBlobType::All:
      case CodeBlobType::MethodNonProfiled:
        return ch->allocated_capacity();
      default:
        break;
    }
  }
  ShouldNotReachHere(); // We always have at least one CodeHeap for C2 nmethods.
  return 0;
}

static bool percent_exceeds(size_t value, size_t total, size_t percent) {
  return (value * 100) > percent * total;
}

void NMethodGrouper::unregister_nmethod(nmethod* nm) {
  assert_locked_or_safepoint(CodeCache_lock);
  if (!_is_initialized) {
    return;
  }

  if (!nm->is_compiled_by_c2()) {
    return; // Only C2 nmethods are tracked for grouping
  }

  // We want the value be available before we release CodeCache_lock.
  Atomic::dec(&_total_c2_nmethods_count, memory_order_relaxed);

  _unregistered_c2_nmethods_count++;
  _unregistered_nmethods.add(nm);
  if (get_profiling_state() == ProfilingState::Waiting &&
      percent_exceeds(_unregistered_c2_nmethods_count, _total_c2_nmethods_count + _unregistered_c2_nmethods_count, 10)) {
    tty->print_cr("[NMethodGrouper]: Unregistered C2 nmethods count exceeded threshold: %zu. Total C2 nmethods count: %zu",
                  _unregistered_c2_nmethods_count, _total_c2_nmethods_count);
    _unregistered_c2_nmethods_count = 0;
    _unregistered_nmethods.clear();
    MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
    ml.notify();
  }
}

void NMethodGrouper::register_nmethod(nmethod* nm) {
  assert_locked_or_safepoint(CodeCache_lock);

  if (!nm->is_compiled_by_c2()) {
    return; // Only C2 nmethods are registered for grouping
  }

  // We want the value be available before we release CodeCache_lock.
  Atomic::inc(&_total_c2_nmethods_count, memory_order_relaxed);

  if (!_is_initialized) {
    return;
  }

  if (get_profiling_state() == ProfilingState::NotStarted &&
      get_c2_code_size() >= 16 * M) { // TODO: Use a configurable threshold
    MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
    ml.notify();
    return;
  }

  // CodeCache_lock is held, so we can safely increment the count.
  _new_c2_nmethods_count++;

  if (get_profiling_state() == ProfilingState::Waiting &&
      percent_exceeds(_new_c2_nmethods_count, _total_c2_nmethods_count, 5)) {
    tty->print_cr("[NMethodGrouper]: New C2 nmethods count exceeded threshold: %zu. Total C2 nmethods count: %zu %d",
                  _new_c2_nmethods_count, _total_c2_nmethods_count, CodeCache::nmethod_count());
    _new_c2_nmethods_count = 0;
    MonitorLocker ml(NMethodGrouper_lock, Mutex::_no_safepoint_check_flag);
    ml.notify();
  }
}
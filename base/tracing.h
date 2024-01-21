// A simple implementation of a trace event recorder and macros that can be
// used together with chrome::tracing and perfetto.dev to view program
// execution timeline.
//
// If you want to annotate a complete scope in a function, you can do:
//
//   void MyFunction() {
//     SCOPED_TRACE("function name");
//     # body
//   }
//
// If you don't like creating scopes and want more control, you can directly
// create begin/end (and possibly instant) events:
//
//   void MyFunction() {
//     TRACE_EVENT("some name", 'B');
//     # function body
//     TRACE_EVENT("some name", 'E');
//   }
//
// Note that the event strings must be known at compile-time. If you have a
// runtime-generated string, you can use `ScopedTraceEvent` object instead of
// SCOPED_TRACE() macro.
//
// When the program is executed with `--tracing=True` flag, macro invocation
// will create TraceEvent objects, which will be written to a json file by the
// Tracer object. The file to be used is controlled by `--tracing_file` flag,
// and frequency of flushing the trace buffer is controlled by the
// `--tracing_buffer_size` flag.
#include <chrono>
#include <fstream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"

ABSL_DECLARE_FLAG(bool, tracing_enable);
ABSL_DECLARE_FLAG(std::string, tracing_file);
ABSL_DECLARE_FLAG(int, tracing_buffer_size);

struct TraceEvent {
  std::string name;
  char ph;
  int64_t ts;
  int pid;
  int tid;

  // Creates the object appropriately filling in current time and pid/tid info.
  TraceEvent(const char *name, char phase);

  // Returns a JSON representation of the object.
  std::string ToString() const;
};

class Tracer {
public:
  static Tracer *instance();

  Tracer(const std::string &filename, int tracing_buffer_size);
  ~Tracer();

  void AddEvent(TraceEvent event);

private:
  void WriteRecords();

  std::vector<TraceEvent> events_;
  std::ofstream fp_;
  std::string filename_;
  size_t tracing_buffer_size_;
  int num_events_written_ = 0;
  std::mutex mu_;
};

// The basic macro that generates a TraceEvent given a string and a phase ('B',
// 'E', 'I') character.
#define TRACE_EVENT(name, phase)                                               \
  if (absl::GetFlag(FLAGS_tracing_enable)) {                                   \
    Tracer::instance()->AddEvent(TraceEvent(name, phase));                     \
  }

// The macro above is inconvenient to use because scope execution (execution of
// one or more functions that need to be profiled) requires two events. It's
// inconvenient to write the macro twice!
// This inconvenience can be lifted using the (very general) trick below.
// We create a little class that calls the macro above in the ctor and once
// again in the dtor.
struct ScopedTraceEvent {
  ScopedTraceEvent(const char *_name) : name(_name) { TRACE_EVENT(name, 'B'); }
  ~ScopedTraceEvent() { TRACE_EVENT(name, 'E'); }
  const char *name;
};

// Copied verbatim from chromium: macros to given unique names to temporary
// variables. Note that this precludes use of runtime-generated strings as
// scope names.
#define INTERNAL_TRACE_EVENT_UID3(a, b) trace_event_unique_##a##b
#define INTERNAL_TRACE_EVENT_UID2(a, b) INTERNAL_TRACE_EVENT_UID3(a, b)
#define INTERNAL_TRACE_EVENT_UID(name_prefix)                                  \
  INTERNAL_TRACE_EVENT_UID2(name_prefix, __LINE__)

// Finally, the macro that one will typically use to annotate a scope.
#define SCOPED_TRACE(name)                                                     \
  ScopedTraceEvent INTERNAL_TRACE_EVENT_UID(__FUNCTION__)(name);

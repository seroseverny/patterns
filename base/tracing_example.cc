//
// Example usage:
//
//  $ bazel run -- //base:tracing_example \
//          --tracing_enable --tracing_file ~/trace.json
//
// Resulting file can be opened in about:tracing in chrome.
#include "absl/flags/parse.h"
#include "base/tracing.h"

void FastFunction() {
  TRACE_EVENT(__FUNCTION__, 'B');
  std::this_thread::sleep_for(std::chrono::milliseconds(33));
  TRACE_EVENT(__FUNCTION__, 'E');
}

void SlowFunction() {
  TRACE_EVENT("SlowFunction", 'B');
  FastFunction();
  FastFunction();
  FastFunction();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  TRACE_EVENT("SlowFunction", 'E');
}

void FastFunctionScopedMacro() {
  SCOPED_TRACE("FastFunctionScopedMacro");
  std::this_thread::sleep_for(std::chrono::milliseconds(33));
}

void SlowFunctionScopedMacro() {
  SCOPED_TRACE("SlowFunctionScopedMacro");
  FastFunctionScopedMacro();
  FastFunctionScopedMacro();
  FastFunctionScopedMacro();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
void LoopBasic() {
  for (int i = 0; i < 10; ++i)
    SlowFunction();
}

void LoopScoped() {
  for (int i = 0; i < 10; ++i)
    SlowFunctionScopedMacro();
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  SCOPED_TRACE(__FUNCTION__);
  std::thread t1(LoopBasic);
  std::thread t2(LoopScoped);
  t1.join();
  t2.join();
  return 0;
}

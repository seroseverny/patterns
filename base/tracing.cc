#include "base/tracing.h"

#include <chrono>
#include <fstream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"

ABSL_FLAG(bool, tracing_enable, false, "Enable tracing.");
ABSL_FLAG(std::string, tracing_file, "/tmp/trace.json",
          "Filename that tracing events are written to.");
ABSL_FLAG(int, tracing_buffer_size, 1024,
          "Number of events that should be written to file.");

static std::unique_ptr<Tracer> g_tracer_ = nullptr;

Tracer *Tracer::instance() {
  if (!g_tracer_) {
    LOG(INFO) << "Creating tracing object; file is: "
              << absl::GetFlag(FLAGS_tracing_file);
    g_tracer_.reset(new Tracer(
        /*filename=*/absl::GetFlag(FLAGS_tracing_file),
        /*tracing_buffer_size=*/absl::GetFlag(FLAGS_tracing_buffer_size)));
  }
  return g_tracer_.get();
}

Tracer::Tracer(const std::string &filename, int tracing_buffer_size)
    : filename_(filename), tracing_buffer_size_(tracing_buffer_size) {
  fp_.open(filename);
  events_.reserve(tracing_buffer_size);
  CHECK(fp_.is_open());
}

Tracer::~Tracer() {
  WriteRecords();
  fp_ << "]";
  if (fp_.is_open())
    fp_.close();
  LOG(INFO) << "Tracing object will be destroyed; results in: " << filename_;
}

void Tracer::AddEvent(TraceEvent event) {
  std::lock_guard<std::mutex> lock(mu_);
  events_.push_back(std::move(event));
  if (events_.size() >= tracing_buffer_size_) {
    WriteRecords();
    events_.clear();
  }
}

void Tracer::WriteRecords() {
  LOG(INFO) << "Tracing object writing " << events_.size() << " events.";
  fp_ << (num_events_written_ ? "," : "[");
  for (const auto &event : events_) {
    fp_ << event.ToString();
    num_events_written_++;
    if (&event != &events_.back())
      fp_ << ",";
  }
}

namespace {

int64_t MicrosecondsSinceEpoch() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}
int CurrentThreadId() {
  return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

} // namespace

TraceEvent::TraceEvent(const char *_name, char phase)
    : name(std::string(_name)), ph(phase), ts(MicrosecondsSinceEpoch()),
      pid(getpid()), tid(CurrentThreadId()) {}

std::string TraceEvent::ToString() const {
  return absl::StrFormat(
      "{\"name\": \"%s\", \"ph\": \"%c\", \"ts\": %lld, \"pid\": %u, "
      "\"tid\": %u}",
      name, ph, ts, pid, tid);
}

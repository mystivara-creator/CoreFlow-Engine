#include "coreflow/observation/surface_flinger.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <map>
#include <optional>
#include <poll.h>
#include <regex>
#include <signal.h>
#include <sstream>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace coreflow {
namespace {
using Clock = std::chrono::steady_clock;
constexpr auto kCommandTimeout = std::chrono::milliseconds(1200);
constexpr auto kRefreshInterval = std::chrono::seconds(5);
constexpr std::size_t kMaximumOutput = 512U * 1024U;
constexpr unsigned long long kMinimumFrameSamples = 8;

std::optional<std::string> runDumpsys(const std::vector<std::string>& arguments) {
    // Build argv before fork. The child must not allocate or acquire C++ runtime
    // locks between fork() and execv() when this observer runs in a multithreaded daemon.
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 2);
    argv.push_back(const_cast<char*>("/system/bin/dumpsys"));
    for (const auto& argument : arguments)
        argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr);

    int pipe_fds[2]{};
    if (::pipe(pipe_fds) != 0) return std::nullopt;

    const pid_t child = ::fork();
    if (child < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        return std::nullopt;
    }
    if (child == 0) {
        (void)::dup2(pipe_fds[1], STDOUT_FILENO);
        (void)::dup2(pipe_fds[1], STDERR_FILENO);
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        ::execv("/system/bin/dumpsys", argv.data());
        _exit(127);
    }

    ::close(pipe_fds[1]);
    const int flags = ::fcntl(pipe_fds[0], F_GETFL, 0);
    if (flags >= 0) (void)::fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK);

    std::string output;
    output.reserve(16U * 1024U);
    const auto deadline = Clock::now() + kCommandTimeout;
    bool eof = false;
    bool timed_out = false;
    bool too_large = false;
    char buffer[8192];

    while (!eof) {
        const auto now = Clock::now();
        if (now >= deadline) {
            timed_out = true;
            break;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        const int wait_ms = static_cast<int>(std::clamp<long long>(remaining.count(), 1, 100));
        pollfd descriptor{pipe_fds[0], POLLIN | POLLHUP, 0};
        int polled;
        do { polled = ::poll(&descriptor, 1, wait_ms); }
        while (polled < 0 && errno == EINTR);
        if (polled < 0) {
            timed_out = true;
            break;
        }
        if (polled == 0) continue;

        for (;;) {
            const ssize_t n = ::read(pipe_fds[0], buffer, sizeof(buffer));
            if (n > 0) {
                const auto amount = static_cast<std::size_t>(n);
                if (amount > kMaximumOutput - output.size()) {
                    too_large = true;
                    break;
                }
                output.append(buffer, amount);
                continue;
            }
            if (n == 0) eof = true;
            else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) eof = true;
            break;
        }
        if (too_large) break;
    }
    ::close(pipe_fds[0]);

    int status = 0;
    pid_t waited = -1;
    if (timed_out || too_large) {
        (void)::kill(child, SIGKILL);
        do { waited = ::waitpid(child, &status, 0); }
        while (waited < 0 && errno == EINTR);
    } else {
        // EOF does not prove the child exited; keep the process deadline in force
        // even when a child closes the pipe and then stalls.
        for (;;) {
            waited = ::waitpid(child, &status, WNOHANG);
            if (waited == child) break;
            if (waited < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (Clock::now() >= deadline) {
                timed_out = true;
                (void)::kill(child, SIGKILL);
                do { waited = ::waitpid(child, &status, 0); }
                while (waited < 0 && errno == EINTR);
                break;
            }
            (void)::poll(nullptr, 0, 10);
        }
    }

    if (timed_out || too_large || waited != child ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return std::nullopt;
    return output;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string foregroundPackage(const std::string& activity_dump) {
    static const std::regex component(
        R"(([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)+)/[A-Za-z0-9_.$]+)");
    std::istringstream lines(activity_dump);
    std::string line;
    std::string fallback;
    int best_score = 0;
    std::string best;
    while (std::getline(lines, line)) {
        int score = 0;
        if (line.find("topResumedActivity") != std::string::npos) score = 3;
        else if (line.find("mResumedActivity") != std::string::npos ||
                 line.find("ResumedActivity:") != std::string::npos) score = 2;
        else if (line.find("mFocusedActivity") != std::string::npos) score = 1;
        if (score == 0) continue;
        std::smatch match;
        if (!std::regex_search(line, match, component)) continue;
        const std::string package = match[1].str();
        if (fallback.empty()) fallback = package;
        if (score > best_score) {
            best_score = score;
            best = package;
        }
    }
    return best.empty() ? fallback : best;
}

struct LayerStats {
    std::string layer_name;
    std::string package_name;
    double average_fps{};
    double average_fps_resolution{};
    unsigned long long total_frames{};
    double interval_resolution_ms{};
    std::vector<std::pair<double, unsigned long long>> intervals;
};

bool parseUnsigned(const std::string& text, unsigned long long& value) {
    if (text.empty()) return false;
    for (const unsigned char c : text) {
        if (c < '0' || c > '9') return false;
    }

    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtoull(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || !end || *end != '\0')
        return false;

    value = parsed;
    return true;
}


double decimalQuantum(const std::string& text) {
    const auto exponent_pos = text.find_first_of("eE");
    const auto mantissa = text.substr(0, exponent_pos);
    int exponent = 0;
    if (exponent_pos != std::string::npos) {
        const std::string exponent_text = text.substr(exponent_pos + 1);
        char* end = nullptr;
        errno = 0;
        const long parsed = std::strtol(exponent_text.c_str(), &end, 10);
        if (errno != 0 || !end || end == exponent_text.c_str() || *end != '\0' ||
            parsed < std::numeric_limits<int>::min() ||
            parsed > std::numeric_limits<int>::max()) return 0.0;
        exponent = static_cast<int>(parsed);
    }
    const auto decimal_pos = mantissa.find('.');
    const int decimals = decimal_pos == std::string::npos
        ? 0 : static_cast<int>(mantissa.size() - decimal_pos - 1);
    const double quantum = std::pow(10.0, (static_cast<double>(exponent) - static_cast<double>(decimals)));
    return std::isfinite(quantum) && quantum > 0.0 ? quantum : 0.0;
}

bool parseHistogramLine(
    const std::string& line,
    std::vector<std::pair<double, unsigned long long>>& intervals,
    double& interval_resolution_ms) {
    std::istringstream tokens(line);
    std::string token;
    bool found_bucket = false;

    while (tokens >> token) {
        const auto ms_pos = token.find("ms=");
        if (ms_pos == std::string::npos) continue;
        if (ms_pos == 0) return false;

        unsigned long long count{};
        if (!parseUnsigned(token.substr(ms_pos + 3), count)) return false;

        const std::string bucket = token.substr(0, ms_pos);
        char* end = nullptr;
        errno = 0;
        const double ms = std::strtod(bucket.c_str(), &end);

        if (errno != 0 || end == bucket.c_str() || !end ||
            *end != '\0' || !std::isfinite(ms) || ms < 0.0) {
            return false;
        }

        intervals.emplace_back(ms, count);
        const double quantum = decimalQuantum(bucket);
        if (quantum > 0.0 &&
            (!(interval_resolution_ms > 0.0) || quantum < interval_resolution_ms)) {
            interval_resolution_ms = quantum;
        }
        found_bucket = true;
    }

    return found_bucket;
}

std::vector<LayerStats> parseLayers(const std::string& timestats_dump) {
    std::vector<LayerStats> layers;
    LayerStats current;
    bool has_layer = false;
    bool in_p2p_histogram = false;
    std::istringstream lines(timestats_dump);
    std::string line;
    auto finish_layer = [&]() {
        if (has_layer) layers.push_back(std::move(current));
        current = LayerStats{};
        has_layer = false;
        in_p2p_histogram = false;
    };

    while (std::getline(lines, line)) {
        // dumpsys formatting varies by Android/OEM version; normalize indentation
        // before recognizing fields so valid telemetry is not silently discarded.
        line = trim(std::move(line));
        if (line.rfind("layerName =", 0) == 0) {
            finish_layer();
            current.layer_name = trim(line.substr(std::string("layerName =").size()));
            has_layer = true;
            continue;
        }
        if (!has_layer) continue;
        if (line.rfind("packageName =", 0) == 0) {
            current.package_name = trim(line.substr(std::string("packageName =").size()));
            in_p2p_histogram = false;
        } else if (line.rfind("totalFrames =", 0) == 0) {
            parseUnsigned(trim(line.substr(std::string("totalFrames =").size())), current.total_frames);
            in_p2p_histogram = false;
        } else if (line.rfind("averageFPS =", 0) == 0) {
            char* end = nullptr;
            const std::string raw = trim(line.substr(std::string("averageFPS =").size()));
            errno = 0;
            current.average_fps = std::strtod(raw.c_str(), &end);
            if (errno != 0 || !end || *end != '\0' || !std::isfinite(current.average_fps) ||
                current.average_fps <= 0.0) {
                current.average_fps = 0.0;
                current.average_fps_resolution = 0.0;
            } else {
                current.average_fps_resolution = decimalQuantum(raw);
            }
            in_p2p_histogram = false;
        } else if (line.find("histogram is as below") != std::string::npos) {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            in_p2p_histogram = lower.find("present2present") != std::string::npos ||
                               lower.find("presenttopresent") != std::string::npos;
        } else if (in_p2p_histogram) {
            if (line.find("layerName =") != std::string::npos ||
                line.find("packageName =") != std::string::npos) {
                in_p2p_histogram = false;
            } else {
                if (line.find("ms=") != std::string::npos) {
                    if (!parseHistogramLine(line, current.intervals,
                                            current.interval_resolution_ms)) {
                        current.intervals.clear();
                        current.interval_resolution_ms = 0.0;
                        current.total_frames = 0;
                        current.average_fps = 0.0;
                        current.average_fps_resolution = 0.0;
                        in_p2p_histogram = false;
                    }
                } else if (line.find("histogram") == std::string::npos) {
                    in_p2p_histogram = false;
                }
            }
        }
    }
    finish_layer();
    return layers;
}

bool packageMatches(const LayerStats& layer, const std::string& package) {
    return !package.empty() &&
        (layer.package_name == package || layer.layer_name.find(package) != std::string::npos);
}

const LayerStats* selectLayer(const std::vector<LayerStats>& layers, const std::string& package) {
    const LayerStats* selected = nullptr;
    for (const auto& layer : layers) {
        if (!packageMatches(layer, package) || layer.total_frames < kMinimumFrameSamples) continue;
        if (!selected || layer.total_frames > selected->total_frames) selected = &layer;
    }
    return selected;
}

std::optional<double> percentile(
    std::vector<std::pair<double, unsigned long long>> intervals,
    unsigned long long sample_count, double quantile) {
    if (intervals.empty() || sample_count == 0 || quantile <= 0.0 || quantile > 1.0)
        return std::nullopt;
    std::sort(intervals.begin(), intervals.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    const auto target = static_cast<unsigned long long>(
        std::ceil(static_cast<long double>(sample_count) * quantile));
    unsigned long long cumulative = 0;
    for (const auto& interval : intervals) {
        if (interval.second > std::numeric_limits<unsigned long long>::max() - cumulative)
            return std::nullopt;
        cumulative += interval.second;
        if (cumulative >= target) return interval.first;
    }
    return intervals.back().first;
}

std::map<double, unsigned long long> aggregateHistogram(const LayerStats& layer) {
    std::map<double, unsigned long long> bins;
    for (const auto& [milliseconds, count] : layer.intervals) {
        auto& total = bins[milliseconds];
        if (count > std::numeric_limits<unsigned long long>::max() - total) return {};
        total += count;
    }
    return bins;
}

unsigned long long histogramCount(const std::map<double, unsigned long long>& bins, bool& valid) {
    unsigned long long total = 0;
    for (const auto& [_, count] : bins) {
        if (count > std::numeric_limits<unsigned long long>::max() - total) {
            valid = false;
            return 0;
        }
        total += count;
    }
    return total;
}

// Fraction of frames that missed at least one whole refresh. The dominant interval
// of the histogram is the display's own cadence, since most frames are presented on
// it; a mean-based threshold would be skewed by the very stalls it should detect.
// A frame that misses one refresh is presented at least two cadences after the
// previous one. This defines a missed vsync and is not a tuned threshold.
std::optional<double> missedRefreshRatio(const std::map<double, unsigned long long>& bins,
                                         unsigned long long samples) {
    double dominant_interval_ms = 0.0;
    unsigned long long dominant_count = 0;
    for (const auto& [interval_ms, count] : bins) {
        if (interval_ms > 0.0 && count > dominant_count) {
            dominant_interval_ms = interval_ms;
            dominant_count = count;
        }
    }
    if (!(dominant_interval_ms > 0.0) || !std::isfinite(dominant_interval_ms) || samples == 0) {
        return std::nullopt;
    }
    double slow_count = 0.0;
    for (const auto& [interval_ms, count] : bins) {
        if (interval_ms >= 2.0 * dominant_interval_ms) slow_count += static_cast<double>(count);
    }
    return std::clamp(slow_count / static_cast<double>(samples), 0.0, 1.0);
}

FrameTimingMetrics metricsFromSnapshot(const LayerStats& layer, const std::string& package) {
    FrameTimingMetrics result;
    if (!(layer.average_fps > 0.0) || !std::isfinite(layer.average_fps)) return result;
    const auto bins = aggregateHistogram(layer);
    bool valid = true;
    const auto samples = histogramCount(bins, valid);
    if (!valid || samples < kMinimumFrameSamples) return result;
    const std::vector<std::pair<double, unsigned long long>> intervals(bins.begin(), bins.end());
    const auto slow_ratio = missedRefreshRatio(bins, samples);
    if (!slow_ratio) return result;
    const auto p95 = percentile(intervals, samples, 0.95);
    const auto p99 = percentile(intervals, samples, 0.99);
    if (!p95 || !p99) return result;

    result.valid = true;
    result.average_fps = layer.average_fps;
    result.average_fps_resolution = layer.average_fps_resolution;
    result.p95_frame_interval_ms = *p95;
    result.p99_frame_interval_ms = *p99;
    result.slow_frame_ratio = *slow_ratio;
    result.interval_resolution_ms = layer.interval_resolution_ms;
    result.slow_frame_ratio_resolution = 1.0 / static_cast<double>(samples);
    result.total_frames = layer.total_frames;
    result.package_name = package;
    return result;
}

FrameTimingMetrics metricsFromDelta(const std::vector<LayerStats>& current_layers,
                                    const std::vector<LayerStats>& previous_layers,
                                    const std::string& package, double elapsed_seconds) {
    FrameTimingMetrics result;
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0) return result;
    const auto* current = selectLayer(current_layers, package);
    if (!current) return result;

    const LayerStats* previous = nullptr;
    for (const auto& layer : previous_layers) {
        if (layer.layer_name == current->layer_name && packageMatches(layer, package)) {
            previous = &layer;
            break;
        }
    }
    if (!previous || current->total_frames < previous->total_frames) return result;

    const auto current_bins = aggregateHistogram(*current);
    const auto previous_bins = aggregateHistogram(*previous);
    if (current_bins.empty() || previous_bins.empty()) return result;
    std::map<double, unsigned long long> delta_bins;
    for (const auto& [bucket, count_before] : previous_bins) {
        const auto found = current_bins.find(bucket);
        if (found == current_bins.end() || found->second < count_before) return result;
    }
    for (const auto& [bucket, count_now] : current_bins) {
        const auto found = previous_bins.find(bucket);
        const unsigned long long count_before = found == previous_bins.end() ? 0 : found->second;
        if (count_now < count_before) return result;
        if (count_now > count_before) delta_bins.emplace(bucket, count_now - count_before);
    }

    bool valid = true;
    const auto sample_count = histogramCount(delta_bins, valid);
    const auto frame_delta = current->total_frames - previous->total_frames;
    if (!valid || sample_count < kMinimumFrameSamples || frame_delta < kMinimumFrameSamples)
        return result;
    const double fps = static_cast<double>(frame_delta) / elapsed_seconds;
    if (!std::isfinite(fps) || fps <= 0.0) return result;

    const std::vector<std::pair<double, unsigned long long>> intervals(delta_bins.begin(), delta_bins.end());
    const auto p95 = percentile(intervals, sample_count, 0.95);
    const auto p99 = percentile(intervals, sample_count, 0.99);
    if (!p95 || !p99) return result;
    const auto slow_ratio = missedRefreshRatio(delta_bins, sample_count);
    if (!slow_ratio) return result;

    result.valid = true;
    result.average_fps = fps;
    result.average_fps_resolution = 1.0 / elapsed_seconds;
    result.p95_frame_interval_ms = *p95;
    result.p99_frame_interval_ms = *p99;
    result.slow_frame_ratio = *slow_ratio;
    result.interval_resolution_ms = current->interval_resolution_ms;
    result.slow_frame_ratio_resolution = 1.0 / static_cast<double>(sample_count);
    result.total_frames = frame_delta;
    result.package_name = package;
    return result;
}

} // namespace

FrameTimingMetrics SurfaceFlingerObserver::parse(const std::string& activity_dump,
                                                  const std::string& timestats_dump) {
    FrameTimingMetrics result;
    const std::string package = foregroundPackage(activity_dump);
    if (package.empty() || timestats_dump.empty()) return result;
    const auto layers = parseLayers(timestats_dump);
    const auto* selected = selectLayer(layers, package);
    if (!selected) return result;
    return metricsFromSnapshot(*selected, package);
}

FrameTimingMetrics SurfaceFlingerObserver::parseDelta(
    const std::string& activity_dump, const std::string& current_timestats_dump,
    const std::string& previous_timestats_dump, double elapsed_seconds) {
    const std::string package = foregroundPackage(activity_dump);
    if (package.empty() || current_timestats_dump.empty() || previous_timestats_dump.empty())
        return {};
    return metricsFromDelta(parseLayers(current_timestats_dump),
                            parseLayers(previous_timestats_dump), package, elapsed_seconds);
}

FrameTimingMetrics SurfaceFlingerObserver::observe() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = Clock::now();

    // Always verify the foreground package before returning cached timings. This
    // keeps an app switch from reusing another application's frame-pressure signal.
    const auto activity = runDumpsys({"activity", "activities"});
    if (!activity) {
        last_metrics_ = {};
        last_timestats_dump_.clear();
        last_activity_package_.clear();
        last_attempt_ = now;
        last_sample_time_ = now;
        return last_metrics_;
    }
    const std::string package = foregroundPackage(*activity);
    if (package.empty()) {
        last_metrics_ = {};
        last_timestats_dump_.clear();
        last_activity_package_.clear();
        last_attempt_ = now;
        last_sample_time_ = now;
        return last_metrics_;
    }
    if (last_attempt_ != Clock::time_point{} && now - last_attempt_ < kRefreshInterval &&
        package == last_activity_package_) {
        return last_metrics_;
    }

    const auto timestats = runDumpsys({"SurfaceFlinger", "--timestats", "-dump"});
    if (!timestats) {
        last_metrics_ = {};
        last_metrics_.package_name = package;
        last_timestats_dump_.clear();
        last_activity_package_ = package;
        last_attempt_ = now;
        last_sample_time_ = now;
        return last_metrics_;
    }

    if (package != last_activity_package_ || last_timestats_dump_.empty() ||
        last_sample_time_ == Clock::time_point{}) {
        // The first snapshot for a package is a baseline, not a valid real-time
        // sample. Wait for a later cumulative counter delta before authorizing tuning.
        last_metrics_ = {};
        last_metrics_.package_name = package;
    } else {
        const auto current_layers = parseLayers(*timestats);
        const auto previous_layers = parseLayers(last_timestats_dump_);
        const double elapsed = std::chrono::duration<double>(now - last_sample_time_).count();
        last_metrics_ = metricsFromDelta(current_layers, previous_layers, package, elapsed);
        if (!last_metrics_.valid) last_metrics_.package_name = package;
    }
    last_timestats_dump_ = *timestats;
    last_activity_package_ = package;
    last_attempt_ = now;
    last_sample_time_ = now;
    return last_metrics_;
}

} // namespace coreflow

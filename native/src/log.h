#pragma once
#include <string>
#include <vector>

// File + stderr logger. Never pass stream URLs or account data to it.
void log_init(const char *path);
void logf(const char *tag, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
// At most one line per `key` every `interval_ms` (repeated failures, study C10);
// the next line after a quiet spell says how many were skipped.
void logf_limited(const char *key, long long interval_ms, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
// Most recent lines, for the on-screen diagnostics page.
std::vector<std::string> log_tail(size_t count);
// "https://host/…" -> "https://host" so logs never carry tokens or paths.
std::string redact_uri(const std::string &uri);

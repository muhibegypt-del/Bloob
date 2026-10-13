#pragma once
#include <string>
#include <vector>

// File + stderr logger. Never pass stream URLs or account data to it.
void log_init(const char *path);
void logf(const char *tag, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
// Most recent lines, for the on-screen diagnostics page.
std::vector<std::string> log_tail(size_t count);
// "https://host/…" -> "https://host" so logs never carry tokens or paths.
std::string redact_uri(const std::string &uri);

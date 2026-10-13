#include "json.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const Json kNull;

const Json &Json::operator[](const std::string &key) const {
  if (type_ != Type::Object) return kNull;
  auto it = object_.find(key);
  return it == object_.end() ? kNull : it->second;
}

const Json &Json::operator[](size_t index) const {
  if (type_ != Type::Array || index >= array_.size()) return kNull;
  return array_[index];
}

class JsonParser {
 public:
  explicit JsonParser(const std::string &text) : s_(text) {}

  bool parse(Json &out) {
    if (!value(out, 0)) return false;
    ws();
    return pos_ == s_.size();
  }

 private:
  const std::string &s_;
  size_t pos_ = 0;

  void ws() {
    while (pos_ < s_.size() && std::strchr(" \t\r\n", s_[pos_])) pos_++;
  }

  bool literal(const char *word) {
    size_t n = std::strlen(word);
    if (s_.compare(pos_, n, word) != 0) return false;
    pos_ += n;
    return true;
  }

  static void append_utf8(std::string &out, unsigned cp) {
    if (cp < 0x80) out += static_cast<char>(cp);
    else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) {
      out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (cp >> 18)); out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F));
    }
  }

  bool hex4(unsigned &cp) {
    if (pos_ + 4 > s_.size()) return false;
    cp = 0;
    for (int i = 0; i < 4; i++) {
      char c = s_[pos_++];
      cp <<= 4;
      if (c >= '0' && c <= '9') cp |= static_cast<unsigned>(c - '0');
      else if (c >= 'a' && c <= 'f') cp |= static_cast<unsigned>(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') cp |= static_cast<unsigned>(c - 'A' + 10);
      else return false;
    }
    return true;
  }

  bool string(std::string &out) {
    if (s_[pos_] != '"') return false;
    pos_++;
    while (pos_ < s_.size()) {
      char c = s_[pos_++];
      if (c == '"') return true;
      if (c != '\\') { out += c; continue; }
      if (pos_ >= s_.size()) return false;
      char e = s_[pos_++];
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned cp;
          if (!hex4(cp)) return false;
          if (cp >= 0xD800 && cp < 0xDC00 && s_.compare(pos_, 2, "\\u") == 0) {
            pos_ += 2;
            unsigned low;
            if (!hex4(low)) return false;
            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
          }
          append_utf8(out, cp);
          break;
        }
        default: return false;
      }
    }
    return false;
  }

  bool value(Json &out, int depth) {
    if (depth > 64) return false;
    ws();
    if (pos_ >= s_.size()) return false;
    char c = s_[pos_];
    if (c == '{') {
      pos_++;
      out.type_ = Json::Type::Object;
      ws();
      if (pos_ < s_.size() && s_[pos_] == '}') { pos_++; return true; }
      for (;;) {
        ws();
        std::string key;
        if (pos_ >= s_.size() || !string(key)) return false;
        ws();
        if (pos_ >= s_.size() || s_[pos_++] != ':') return false;
        Json item;
        if (!value(item, depth + 1)) return false;
        out.object_[key] = std::move(item);
        ws();
        if (pos_ >= s_.size()) return false;
        if (s_[pos_] == ',') { pos_++; continue; }
        if (s_[pos_] == '}') { pos_++; return true; }
        return false;
      }
    }
    if (c == '[') {
      pos_++;
      out.type_ = Json::Type::Array;
      ws();
      if (pos_ < s_.size() && s_[pos_] == ']') { pos_++; return true; }
      for (;;) {
        Json item;
        if (!value(item, depth + 1)) return false;
        out.array_.push_back(std::move(item));
        ws();
        if (pos_ >= s_.size()) return false;
        if (s_[pos_] == ',') { pos_++; continue; }
        if (s_[pos_] == ']') { pos_++; return true; }
        return false;
      }
    }
    if (c == '"') { out.type_ = Json::Type::String; return string(out.string_); }
    if (literal("true")) { out.type_ = Json::Type::Bool; out.bool_ = true; return true; }
    if (literal("false")) { out.type_ = Json::Type::Bool; out.bool_ = false; return true; }
    if (literal("null")) { out.type_ = Json::Type::Null; return true; }
    const char *start = s_.c_str() + pos_;
    char *end = nullptr;
    double number = std::strtod(start, &end);
    if (end == start) return false;
    pos_ += static_cast<size_t>(end - start);
    out.type_ = Json::Type::Number;
    out.number_ = number;
    return true;
  }
};

Json Json::parse(const std::string &text, bool *ok) {
  Json out;
  JsonParser parser(text);
  bool good = parser.parse(out);
  if (ok) *ok = good;
  return good ? out : Json();
}

std::string Json::quote(const std::string &value) {
  std::string out = "\"";
  for (unsigned char c : value) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x", c);
          out += buf;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out + "\"";
}

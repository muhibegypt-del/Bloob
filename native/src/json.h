#pragma once
// Minimal JSON DOM for luna payloads: parse, look up, and escape strings.
#include <map>
#include <memory>
#include <string>
#include <vector>

class Json {
 public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  Json() = default;
  static Json parse(const std::string &text, bool *ok = nullptr);
  static std::string quote(const std::string &value);
  // Compact JSON text (for logs and request bodies).
  static std::string stringify(const Json &value);

  Type type() const { return type_; }
  bool is_null() const { return type_ == Type::Null; }
  bool is_object() const { return type_ == Type::Object; }
  bool is_array() const { return type_ == Type::Array; }
  bool has(const std::string &key) const { return type_ == Type::Object && object_.count(key) != 0; }

  const Json &operator[](const std::string &key) const;
  const Json &operator[](size_t index) const;
  size_t size() const { return type_ == Type::Array ? array_.size() : type_ == Type::Object ? object_.size() : 0; }
  const std::map<std::string, Json> &items() const { return object_; }

  bool as_bool(bool fallback = false) const { return type_ == Type::Bool ? bool_ : fallback; }
  double as_number(double fallback = 0) const { return type_ == Type::Number ? number_ : fallback; }
  long long as_int(long long fallback = 0) const { return type_ == Type::Number ? static_cast<long long>(number_) : fallback; }
  std::string as_string(const std::string &fallback = "") const { return type_ == Type::String ? string_ : fallback; }

 private:
  friend class JsonParser;
  Type type_ = Type::Null;
  bool bool_ = false;
  double number_ = 0;
  std::string string_;
  std::vector<Json> array_;
  std::map<std::string, Json> object_;
};

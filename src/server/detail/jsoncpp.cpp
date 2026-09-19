#include "json.hpp"

#if __has_include(<json/json.h>)
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libmcp {

struct json_doc::impl
{
    Json::Value value{ Json::nullValue };
};

namespace {

[[noreturn]] void fail(char const* what)
{
    throw json_error(what);
}

inline std::string write_compact(Json::Value const& v)
{
    Json::StreamWriterBuilder w;
    w["commentStyle"] = "None";
    w["indentation"]  = "";
    std::string s     = Json::writeString(w, v);
    if (!s.empty() && s.back() == '\n') {
        s.pop_back();
    }
    return s;
}

} // namespace

json_doc::~json_doc()                              = default;
json_doc::json_doc(json_doc&&) noexcept            = default;
json_doc& json_doc::operator=(json_doc&&) noexcept = default;

json_doc::json_doc(json_doc const& o)
{
    if (o.rep_ != nullptr) {
        rep_ = std::make_shared<impl>(*o.rep_);
    }
}

json_doc& json_doc::operator=(json_doc const& o)
{
    if (this != &o) {
        if (o.rep_ != nullptr) {
            rep_ = std::make_shared<impl>(*o.rep_);
        } else {
            rep_.reset();
        }
    }
    return *this;
}

bool json_doc::valid() const noexcept
{
    return static_cast<bool>(rep_);
}

bool json_doc::is_object() const noexcept
{
    return rep_ != nullptr && rep_->value.isObject();
}

bool json_doc::is_array() const noexcept
{
    return rep_ != nullptr && rep_->value.isArray();
}

bool json_doc::is_string() const noexcept
{
    return rep_ != nullptr && rep_->value.isString();
}

bool json_doc::is_bool() const noexcept
{
    return rep_ != nullptr && rep_->value.isBool();
}

bool json_doc::is_number() const noexcept
{
    return rep_ != nullptr && rep_->value.isNumeric();
}

bool json_doc::is_number_integer() const noexcept
{
    return rep_ != nullptr && rep_->value.isIntegral();
}

bool json_doc::is_number_float() const noexcept
{
    return rep_ != nullptr && rep_->value.isDouble();
}

bool json_doc::is_null() const noexcept
{
    return rep_ != nullptr && rep_->value.isNull();
}

bool json_doc::contains(std::string_view key) const noexcept
{
    if (rep_ == nullptr || !rep_->value.isObject()) {
        return false;
    }
    std::string k{ key };
    return rep_->value.isMember(k);
}

std::size_t json_doc::size() const noexcept
{
    if (rep_ == nullptr) {
        return 0;
    }
    return static_cast<std::size_t>(rep_->value.size());
}

json_doc json_doc::find(std::string_view key) const
{
    if (rep_ == nullptr) {
        return {};
    }
    if (!rep_->value.isObject()) {
        fail("find on non-object");
    }
    std::string k{ key };
    if (!rep_->value.isMember(k)) {
        return {};
    }
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = rep_->value[k];
    return out;
}

json_doc json_doc::at(std::size_t i) const
{
    if (rep_ == nullptr) {
        fail("at on invalid");
    }
    if (!rep_->value.isArray()) {
        fail("at on non-array");
    }
    if (i >= static_cast<std::size_t>(rep_->value.size())) {
        fail("at out of range");
    }
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = rep_->value[static_cast<Json::ArrayIndex>(i)];
    return out;
}

std::vector<json_doc> json_doc::elements() const
{
    if (rep_ == nullptr) {
        fail("elements on invalid");
    }
    if (!rep_->value.isArray()) {
        fail("elements on non-array");
    }
    std::vector<json_doc> out{};
    out.reserve(static_cast<std::size_t>(rep_->value.size()));
    for (Json::Value const& v : rep_->value) {
        json_doc e{};
        e.rep_        = std::make_shared<impl>();
        e.rep_->value = v;
        out.push_back(std::move(e));
    }
    return out;
}

std::vector<std::pair<std::string, json_doc>> json_doc::items() const
{
    if (rep_ == nullptr) {
        fail("items on invalid");
    }
    if (!rep_->value.isObject()) {
        fail("items on non-object");
    }
    std::vector<std::pair<std::string, json_doc>> out{};
    for (std::string const& k : rep_->value.getMemberNames()) {
        json_doc e{};
        e.rep_        = std::make_shared<impl>();
        e.rep_->value = rep_->value[k];
        out.emplace_back(k, std::move(e));
    }
    return out;
}

std::string json_doc::as_string() const
{
    if (rep_ == nullptr) {
        fail("as_string on invalid");
    }
    if (!rep_->value.isString()) {
        fail("not a string");
    }
    return rep_->value.asString();
}

bool json_doc::as_bool() const
{
    if (rep_ == nullptr) {
        fail("as_bool on invalid");
    }
    if (!rep_->value.isBool()) {
        fail("not a bool");
    }
    return rep_->value.asBool();
}

std::int64_t json_doc::as_int() const
{
    if (rep_ == nullptr) {
        fail("as_int on invalid");
    }
    Json::Value const& v = rep_->value;
    if (v.isInt64()) {
        return v.asInt64();
    }
    if (v.isUInt64()) {
        Json::UInt64 u = v.asUInt64();
        if (u > static_cast<Json::UInt64>(
                    (std::numeric_limits<std::int64_t>::max)())) {
            fail("uint64 out of int64 range");
        }
        return static_cast<std::int64_t>(u);
    }
    fail("not an integer");
}

double json_doc::as_double() const
{
    if (rep_ == nullptr) {
        fail("as_double on invalid");
    }
    if (!rep_->value.isNumeric()) {
        fail("not a number");
    }
    return rep_->value.asDouble();
}

std::string json_doc::dump() const
{
    if (rep_ == nullptr) {
        fail("dump on invalid");
    }
    return write_compact(rep_->value);
}

std::map<std::string, std::string> json_doc::flat_map() const
{
    if (rep_ == nullptr) {
        fail("flat_map on invalid");
    }
    if (!rep_->value.isObject()) {
        fail("flat_map on non-object");
    }
    std::map<std::string, std::string> out{};
    for (std::string const& k : rep_->value.getMemberNames()) {
        Json::Value const& v = rep_->value[k];
        if (v.isString()) {
            out.emplace(k, v.asString());
        } else if (v.isNull()) {
            out.emplace(k, std::string{});
        } else {
            out.emplace(k, write_compact(v));
        }
    }
    return out;
}

void json_doc::set(std::string_view key, json_doc value)
{
    if (rep_ == nullptr) {
        fail("set on invalid");
    }
    if (!rep_->value.isObject()) {
        fail("set on non-object");
    }
    std::string k{ key };
    if (value.rep_ != nullptr) {
        rep_->value[k] = value.rep_->value;
    } else {
        rep_->value[k] = Json::Value(Json::nullValue);
    }
}

void json_doc::push(json_doc value)
{
    if (rep_ == nullptr) {
        fail("push on invalid");
    }
    if (!rep_->value.isArray()) {
        fail("push on non-array");
    }
    if (value.rep_ != nullptr) {
        rep_->value.append(value.rep_->value);
    } else {
        rep_->value.append(Json::Value(Json::nullValue));
    }
}

json_doc json_doc::parse(std::string_view text)
{
    if (text.empty()) {
        fail("empty document");
    }
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
    Json::Value                       root{ Json::nullValue };
    std::string                       errs{};
    bool                              ok =
        reader->parse(text.data(), text.data() + text.size(), &root, &errs);
    if (!ok) {
        if (errs.empty()) {
            fail("parse error");
        }
        throw json_error(errs);
    }
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = std::move(root);
    return out;
}

json_doc json_doc::object()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(Json::objectValue);
    return out;
}

json_doc json_doc::array()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(Json::arrayValue);
    return out;
}

json_doc json_doc::null()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(Json::nullValue);
    return out;
}

json_doc json_doc::string(std::string_view s)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(std::string{ s });
    return out;
}

json_doc json_doc::boolean(bool b)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(b);
    return out;
}

json_doc json_doc::integer(std::int64_t v)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(static_cast<Json::Int64>(v));
    return out;
}

json_doc json_doc::real(double v)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = Json::Value(v);
    return out;
}

} // namespace libmcp

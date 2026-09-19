#include "json.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace libmcp {

struct json_doc::impl
{
    nlohmann::json value{ nlohmann::json(nullptr) };
};

namespace {

[[noreturn]] void fail(char const* what)
{
    throw json_error(what);
}

} // namespace

json_doc::~json_doc()                              = default;
json_doc::json_doc(json_doc&&) noexcept            = default;
json_doc& json_doc::operator=(json_doc&&) noexcept = default;

json_doc::json_doc(json_doc const& o)
{
    if (o.rep_ != nullptr) {
        rep_        = std::make_shared<impl>();
        rep_->value = o.rep_->value;
    }
}

json_doc& json_doc::operator=(json_doc const& o)
{
    if (this != &o) {
        if (o.rep_ != nullptr) {
            auto tmp   = std::make_shared<impl>();
            tmp->value = o.rep_->value;
            rep_       = std::move(tmp);
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
    return rep_ != nullptr && rep_->value.is_object();
}

bool json_doc::is_array() const noexcept
{
    return rep_ != nullptr && rep_->value.is_array();
}

bool json_doc::is_string() const noexcept
{
    return rep_ != nullptr && rep_->value.is_string();
}

bool json_doc::is_bool() const noexcept
{
    return rep_ != nullptr && rep_->value.is_boolean();
}

bool json_doc::is_number() const noexcept
{
    return rep_ != nullptr && rep_->value.is_number();
}

bool json_doc::is_number_integer() const noexcept
{
    return rep_ != nullptr && rep_->value.is_number_integer();
}

bool json_doc::is_number_float() const noexcept
{
    return rep_ != nullptr && rep_->value.is_number_float();
}

bool json_doc::is_null() const noexcept
{
    return rep_ == nullptr || rep_->value.is_null();
}

bool json_doc::contains(std::string_view key) const noexcept
{
    if (rep_ == nullptr || !rep_->value.is_object())
        return false;
    return rep_->value.contains(std::string{ key });
}

std::size_t json_doc::size() const noexcept
{
    if (rep_ == nullptr)
        return 0;
    if (rep_->value.is_object() || rep_->value.is_array())
        return rep_->value.size();
    return 0;
}

json_doc json_doc::find(std::string_view key) const
{
    if (rep_ == nullptr || !rep_->value.is_object())
        return json_doc{};
    auto it = rep_->value.find(std::string{ key });
    if (it == rep_->value.end())
        return json_doc{};
    try {
        json_doc out{};
        out.rep_        = std::make_shared<impl>();
        out.rep_->value = *it;
        return out;
    } catch (...) {
        fail("json_doc::find failed");
    }
}

json_doc json_doc::at(std::size_t i) const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        if (!rep_->value.is_array())
            throw json_error("json_doc is not an array");
        json_doc out{};
        out.rep_        = std::make_shared<impl>();
        out.rep_->value = rep_->value.at(i);
        return out;
    } catch (json_error const&) {
        throw;
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::vector<json_doc> json_doc::elements() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        std::vector<json_doc> out{};
        if (!rep_->value.is_array())
            return out;
        out.reserve(rep_->value.size());
        for (auto const& e : rep_->value) {
            json_doc d{};
            d.rep_        = std::make_shared<impl>();
            d.rep_->value = e;
            out.push_back(std::move(d));
        }
        return out;
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::vector<std::pair<std::string, json_doc>> json_doc::items() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        std::vector<std::pair<std::string, json_doc>> out{};
        if (!rep_->value.is_object())
            return out;
        out.reserve(rep_->value.size());
        for (auto const& kv : rep_->value.items()) {
            json_doc d{};
            d.rep_        = std::make_shared<impl>();
            d.rep_->value = kv.value();
            out.emplace_back(kv.key(), std::move(d));
        }
        return out;
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::string json_doc::as_string() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        return rep_->value.get<std::string>();
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

bool json_doc::as_bool() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        return rep_->value.get<bool>();
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::int64_t json_doc::as_int() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        return rep_->value.get<std::int64_t>();
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

double json_doc::as_double() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        return rep_->value.get<double>();
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::string json_doc::dump() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        return rep_->value.dump();
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

std::map<std::string, std::string> json_doc::flat_map() const
{
    if (rep_ == nullptr)
        fail("json_doc is null");
    try {
        std::map<std::string, std::string> out{};
        for (auto const& [k, v] : rep_->value.items())
            out.emplace(k,
                        v.is_string() ? v.get<std::string>()
                        : v.is_null() ? std::string{}
                                      : v.dump());
        return out;
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

void json_doc::set(std::string_view key, json_doc value)
{
    if (rep_ == nullptr || value.rep_ == nullptr)
        fail("json_doc is null");
    try {
        rep_->value[std::string{ key }] = std::move(value.rep_->value);
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

void json_doc::push(json_doc value)
{
    if (rep_ == nullptr || value.rep_ == nullptr)
        fail("json_doc is null");
    try {
        rep_->value.push_back(std::move(value.rep_->value));
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

json_doc json_doc::parse(std::string_view text)
{
    try {
        json_doc out{};
        out.rep_        = std::make_shared<impl>();
        out.rep_->value = nlohmann::json::parse(text.begin(), text.end());
        return out;
    } catch (std::exception const& e) {
        throw json_error(e.what());
    }
}

json_doc json_doc::object()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = nlohmann::json::object();
    return out;
}

json_doc json_doc::array()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = nlohmann::json::array();
    return out;
}

json_doc json_doc::null()
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = nlohmann::json(nullptr);
    return out;
}

json_doc json_doc::string(std::string_view s)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = std::string{ s };
    return out;
}

json_doc json_doc::boolean(bool b)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = b;
    return out;
}

json_doc json_doc::integer(std::int64_t v)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = v;
    return out;
}

json_doc json_doc::real(double v)
{
    json_doc out{};
    out.rep_        = std::make_shared<impl>();
    out.rep_->value = v;
    return out;
}

} // namespace libmcp

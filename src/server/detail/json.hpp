#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libmcp {

struct json_error final : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class json_doc final
{
public:
    json_doc() noexcept = default;
    ~json_doc();
    json_doc(json_doc&&) noexcept;
    json_doc& operator=(json_doc&&) noexcept;
    json_doc(json_doc const&);
    json_doc& operator=(json_doc const&);

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool is_object() const noexcept;
    [[nodiscard]] bool is_array() const noexcept;
    [[nodiscard]] bool is_string() const noexcept;
    [[nodiscard]] bool is_bool() const noexcept;
    [[nodiscard]] bool is_number() const noexcept;
    [[nodiscard]] bool is_number_integer() const noexcept;
    [[nodiscard]] bool is_number_float() const noexcept;
    [[nodiscard]] bool is_null() const noexcept;

    [[nodiscard]] bool        contains(std::string_view key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] json_doc              find(std::string_view key) const;
    [[nodiscard]] json_doc              at(std::size_t i) const;
    [[nodiscard]] std::vector<json_doc> elements() const;
    [[nodiscard]] std::vector<std::pair<std::string, json_doc>> items() const;

    [[nodiscard]] std::string                        as_string() const;
    [[nodiscard]] bool                               as_bool() const;
    [[nodiscard]] std::int64_t                       as_int() const;
    [[nodiscard]] double                             as_double() const;
    [[nodiscard]] std::string                        dump() const;
    [[nodiscard]] std::map<std::string, std::string> flat_map() const;

    void set(std::string_view key, json_doc value);
    void push(json_doc value);

    [[nodiscard]] static json_doc parse(std::string_view text);
    [[nodiscard]] static json_doc object();
    [[nodiscard]] static json_doc array();
    [[nodiscard]] static json_doc null();
    [[nodiscard]] static json_doc string(std::string_view s);
    [[nodiscard]] static json_doc boolean(bool b);
    [[nodiscard]] static json_doc integer(std::int64_t v);
    [[nodiscard]] static json_doc real(double v);

private:
    struct impl;
    std::shared_ptr<impl> rep_{};
};

} // namespace libmcp

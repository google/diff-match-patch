#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <sstream>

namespace std {
using wstring_list = std::vector<std::wstring>;
using dmp_variant = std::variant<std::wstring, std::wstring_list>;

void replace_all(std::wstring& str, const std::wstring& from, const std::wstring& to);
bool ends_with(const std::wstring & a, const std::wstring& b);
bool starts_with(const std::wstring & a, const std::wstring& b);
std::wstring_list split(const std::wstring & s, wchar_t delimiter, bool skip_empty=false);
std::wstring format(const wchar_t * f, ...);
std::string url_encode(const std::string &value, const std::string & exclude="-_.~");
std::wstring url_encode(const std::wstring &value, const std::string & exclude="-_.~");
std::string url_decode(const std::string &value);
std::wstring url_decode(const std::wstring &value);
void debug_print(const wchar_t * f, ...);
template<class InputIterator, class value_type = typename InputIterator::value_type>
std::wstring join(const InputIterator & begin,
                 const InputIterator & end,
                 const std::wstring & delimiters) {
    std::wstringstream ss1;
    std::copy(begin, end,
              std::ostream_iterator<value_type, wchar_t>(ss1, delimiters.c_str()));

    return ss1.str();
}

template<class v_t, class value_type = typename v_t::value_type>
std::wstring join(const v_t & v, const std::wstring & delimiters) {
    return join<typename v_t::const_iterator, value_type>(v.begin(), v.end(), delimiters);
}

std::wstring var_to_string(const dmp_variant & var);
}

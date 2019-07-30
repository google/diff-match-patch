#include <algorithm>
#include <limits>
#include <deque>
#include <regex>
#include <sstream>
#include <iostream>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <list>

#include "diff_match_patch_util.h"

namespace std {
void replace_all(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

bool ends_with(const std::wstring & a, const std::wstring& b) {
    if (b.length() == 0) return true;
    if (a.length() == 0) return false;
    if (a.length() < b.length()) return false;

    auto r = a.rfind(b);

    if (r == std::wstring::npos) return false;

    return r + b.length() == a.length();
}

bool starts_with(const std::wstring & a, const std::wstring& b) {
    return a.find(b) == 0;
}

std::wstring_list split(const std::wstring & s, wchar_t delimiter, bool skip_empty) {
    std::wstring_list tokens;

    std::wistringstream f(s);

    std::wstring ss;

    while(std::getline(f, ss, delimiter)) {
        if (skip_empty && ss.length() == 0)
            continue;
        tokens.push_back(ss);
    }

    return tokens;
}

std::wstring format(const wchar_t * f, ...)
{
    va_list args;

    size_t len = 4096;
    int result = -1;
    std::vector<wchar_t> vec{};

    do {
        vec.resize(len);
        va_start (args, f);
        result = std::vswprintf(&vec[0], len - 1, f, args);

        if (result > 0) {
            vec[result] = 0;
        } else {
            len *= 2;
        }
        va_end (args);
    } while(result < 0);

    return std::wstring{&vec[0]};
}

void debug_print(const wchar_t * f, ...)
{
    va_list args;

    size_t len = 4096;
    int result = -1;
    std::vector<wchar_t> vec{};

    do {
        vec.resize(len);
        va_start (args, f);
        result = std::vswprintf(&vec[0], len - 1, f, args);

        if (result > 0) {
            vec[result] = 0;
        } else {
            len *= 2;
        }
        va_end (args);
    } while(result < 0);

    std::wcerr << std::wstring{&vec[0]} << std::endl;
}


static
std::wstring_convert<std::codecvt_utf8<wchar_t
#if defined(_WIN32) && defined(__GNUC__)
                                       , 0x10ffff, std::little_endian
#endif
                                       >, wchar_t> wcharconv;

std::string url_encode(const std::string &value, const std::string & exclude) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || exclude.find(c) != std::string::npos) {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}

std::wstring url_encode(const std::wstring &value, const std::string & exclude) {
    std::string utf8_str = wcharconv.to_bytes(value);

    auto result = url_encode(utf8_str, exclude);

    return wcharconv.from_bytes(result);
}


static
int get_val(std::string::value_type c) {
    if (c >= '0' && c <='9')
        return c - '0';

    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';

    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';

    return -1;
}

std::string url_decode(const std::string &value) {
    std::ostringstream escaped;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);


        if (c == '%') {
            auto v1 = get_val(*(i + 1));
            auto v2 = get_val(*(i + 2));

            if (v1 < 0 || v2 < 0)
                throw std::string("invalid value:") + value;

            escaped << (unsigned char)((v1 << 4) + v2);
            i+=2;
            continue;
        }

        escaped << c;
    }

    return escaped.str();
}

std::wstring url_decode(const std::wstring &value) {
    std::string utf8_str = wcharconv.to_bytes(value);

    auto result = url_decode(utf8_str);

    return wcharconv.from_bytes(result);
}

std::wstring var_to_string(const dmp_variant & var) {
    if (std::holds_alternative<std::wstring>(var)) {
        return std::get<std::wstring>(var);
    }
    if (std::holds_alternative<std::wstring_list>(var)) {
        return join(std::get<std::wstring_list>(var), L",");
    }

    return L"";
}

}

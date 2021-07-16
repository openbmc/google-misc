#include <flasher/mod.hpp>

namespace flasher
{

std::string removeEscapes(std::string_view in)
{
    std::string ret;
    ret.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '\\')
        {
            continue;
        }
        ret.push_back(in[i]);
    }
    return ret;
}

size_t findSep(std::string_view str, char sep)
{
    size_t i = 0;
    for (; i < str.size(); ++i)
    {
        if (str[i] == '\\')
        {
            ++i;
            continue;
        }
        if (str[i] == sep)
        {
            return i;
        }
    }
    return str.npos;
}

ModArgs::ModArgs(std::string_view str)
{
    while (true)
    {
	auto sep_pos = findSep(str, ',');
        auto val = str.substr(0, sep_pos);
        size_t eq_pos = findSep(val, '=');
        if (eq_pos != val.npos)
        {
            dict.emplace(removeEscapes(val.substr(0, eq_pos)), removeEscapes(val.substr(eq_pos + 1)));
        }
        else
        {
            arr.emplace_back(removeEscapes(val));
        }
        if (sep_pos == str.npos)
        {
            return;
        }
        str = str.substr(sep_pos + 1);
    }
}

} // namespace flasher

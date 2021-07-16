// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
            dict.emplace(removeEscapes(val.substr(0, eq_pos)),
                         removeEscapes(val.substr(eq_pos + 1)));
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

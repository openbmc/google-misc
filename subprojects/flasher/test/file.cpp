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

#include <flasher/file.hpp>

#include <gtest/gtest.h>

namespace flasher
{

using stdplus::fd::OpenAccess;
using stdplus::fd::OpenFlags;

TEST(OpenFile, Missing)
{
    ModArgs args("");
    args.arr.clear();
    EXPECT_THROW(openFile(args, OpenFlags(OpenAccess::ReadOnly)),
                 std::invalid_argument);
}

TEST(OpenFile, Invalid)
{
    ModArgs args("invalid,/dev/null");
    EXPECT_THROW(openFile(args, OpenFlags(OpenAccess::ReadOnly)),
                 std::invalid_argument);
}

TEST(OpenDevice, SimpleDefault)
{
    ModArgs args("/dev/null");
    openFile(args, OpenFlags(OpenAccess::ReadOnly));
}

TEST(OpenDevice, Valid)
{
    ModArgs args("simple,/dev/null");
    openFile(args, OpenFlags(OpenAccess::ReadOnly));
}

} // namespace flasher

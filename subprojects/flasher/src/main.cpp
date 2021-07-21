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

#include <flasher/args.hpp>
#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/logging.hpp>
#include <flasher/mutate.hpp>
#include <flasher/ops.hpp>
#include <stdplus/util/cexec.hpp>

#include <optional>
#include <stdexcept>

namespace flasher
{

NestedMutate makeNestedMutate(stdplus::span<const ModArgs> mutations_args)
{
    NestedMutate ret;
    for (const auto& args : mutations_args)
    {
        ret.mutations.push_back(openMutate(args));
    }
    return ret;
}

void main_wrapped(int argc, char* argv[])
{
    using stdplus::fd::OpenAccess;
    using stdplus::fd::OpenFlag;
    using stdplus::fd::OpenFlags;

    auto args = Args::argsOrHelp(argc, argv);
    reinterpret_cast<uint8_t&>(logLevel) += args.verbose;

    switch (args.op)
    {
        case Args::Op::Automatic:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            auto mutate = makeNestedMutate(args.mutate);
            ops::automatic(*dev, args.dev_offset, *file, args.file_offset,
                           mutate, args.max_size, args.stride, args.noread);
            if (args.verify)
            {
                ops::verify(*dev, args.dev_offset, *file, args.file_offset,
                            mutate, args.max_size, args.stride);
            }
        }
        break;
        case Args::Op::Read:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::WriteOnly)
                                                 .set(OpenFlag::Create)
                                                 .set(OpenFlag::Trunc));
            auto mutate = makeNestedMutate(args.mutate);
            ops::read(*dev, args.dev_offset, *file, args.file_offset, mutate,
                      args.max_size, args.stride);
        }
        break;
        case Args::Op::Write:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            auto mutate = makeNestedMutate(args.mutate);
            ops::write(*dev, args.dev_offset, *file, args.file_offset, mutate,
                       args.max_size, args.stride, args.noread);
            if (args.verify)
            {
                ops::verify(*dev, args.dev_offset, *file, args.file_offset,
                            mutate, args.max_size, args.stride);
            }
        }
        break;
        case Args::Op::Erase:
        {
            auto dev = openDevice(*args.dev);
            ops::erase(*dev, args.dev_offset, args.max_size, args.stride,
                       args.noread);
            if (args.verify)
            {
                throw std::runtime_error("Not implemented");
            }
        }
        break;
        case Args::Op::Verify:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            auto mutate = makeNestedMutate(args.mutate);
            ops::verify(*dev, args.dev_offset, *file, args.file_offset, mutate,
                        args.max_size, args.stride);
        }
        break;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        main_wrapped(argc, argv);
        return 0;
    }
    catch (const std::exception& e)
    {
        log(LogLevel::Error, "ERROR: {}\n", e.what());
        return 1;
    }
}

} // namespace flasher

int main(int argc, char* argv[])
{
    return flasher::main(argc, argv);
}

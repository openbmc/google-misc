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

#include <flashupdate/args.hpp>
#include <flashupdate/logging.hpp>
#include <flashupdate/ops.hpp>

#include <iostream>
#include <vector>

namespace flashupdate
{

void main_wrapped(int argc, char* argv[])
{
    auto args = Args::argsOrHelp(argc, argv);

    reinterpret_cast<uint8_t&>(logLevel) += args.verbose;
    switch (args.op)
    {
        case Args::Op::ValidateConfig:
        {
            log(LogLevel::Notice,
                "NOTICE: empty command to validate the json config.\n");
        }
        break;
        case Args::Op::InjectPersistent:
        {
            ops::injectPersistent(args);
        }
        break;
        case Args::Op::HashDescriptor:
        {
            ops::hashDescriptor(args);
        }
        break;
        case Args::Op::Read:
        {
            ops::read(args);
        }
        break;
        case Args::Op::Write:
        {
            ops::write(args);
        }
        break;
        case Args::Op::UpdateState:
        {
            ops::updateState(args);
        }
        break;
        case Args::Op::UpdateStagedVersion:
        {
            ops::updateStagedVersion(args);
        }
        break;
        case Args::Op::Info:
        {
            ops::info(args);
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
    catch (const std::logic_error& e)
    {
        log(LogLevel::Error, "ERROR: logic_error: {}\n", e.what());
        return 1;
    }
    catch (const std::exception& e)
    {
        log(LogLevel::Error, "ERROR: {}\n", e.what());
        return 2;
    }
}

} // namespace flashupdate

int main(int argc, char* argv[])
{
    return flashupdate::main(argc, argv);
}

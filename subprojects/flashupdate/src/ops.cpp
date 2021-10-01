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

#include <fmt/format.h>
#include <unistd.h>

#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/mod.hpp>
#include <flasher/mutate.hpp>
#include <flasher/ops.hpp>
#include <flashupdate/args.hpp>
#include <flashupdate/info.hpp>
#include <flashupdate/logging.hpp>
#include <stdplus/exception.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace flashupdate
{
namespace ops
{

// Operation Definitions
void info(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void updateState(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void updateStagedVersion(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void injectPersistent(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void hashDescriptor(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void read(const Args&)
{
    throw std::runtime_error("Not implemented");
}

void write(const Args&)
{
    throw std::runtime_error("Not implemented");
}

} // namespace ops
} // namespace flashupdate

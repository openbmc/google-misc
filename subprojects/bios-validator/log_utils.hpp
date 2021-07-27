/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <libcr51sign/cr51_image_descriptor.h>

#include <string>
#include <vector>

namespace google
{
namespace bios_validator
{
namespace internal
{

/** @brief Format Image version information into a string
 *  @param descriptor Validated image descriptor
 *  @param format Format string.
 */
std::string formatImageVersion(const struct image_descriptor& descriptor,
                               const std::string& format = "{0}.{1}.{2}.{3}");

} // namespace internal
} // namespace bios_validator
} // namespace google

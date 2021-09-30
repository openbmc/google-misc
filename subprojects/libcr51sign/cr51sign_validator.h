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
#include "libcr51sign.h"

#include <optional>
#include <string_view>
#include <vector>

namespace google
{
namespace cr51
{

std::vector<uint8_t>& getBuffer();

bool readImage(std::string_view fileName, size_t size);

std::optional<std::vector<uint8_t>> hashDescriptor(struct libcr51sign_ctx* ctx);

std::optional<struct libcr51sign_validated_regions>
    validateDescriptor(struct libcr51sign_ctx* ctx);

} // namespace cr51
} // namespace google
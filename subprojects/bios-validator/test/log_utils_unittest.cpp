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
#include "log_utils.hpp"

#include <libcr51sign/cr51_image_descriptor.h>

#include <gtest/gtest.h>

TEST(VersionTest, TestVersionFormatting)
{
    namespace gbi = google::bios_validator::internal;

    struct image_descriptor descriptor;
    descriptor.image_major = 0;
    descriptor.image_minor = 20121205;
    descriptor.image_point = 0;
    descriptor.image_subpoint = 0;

    EXPECT_STREQ(gbi::formatImageVersion(descriptor, "{0}.{1}.{2}-{3}").c_str(),
                 "0.20121205.0-0");

    EXPECT_EQ(gbi::formatImageVersion(descriptor), "0.20121205.0.0");
}

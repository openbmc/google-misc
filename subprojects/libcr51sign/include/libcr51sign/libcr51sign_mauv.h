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

#ifndef PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_MAUV_H_
#define PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_MAUV_H_

#include <libcr51sign/libcr51sign.h>
#include <libcr51sign/libcr51sign_internal.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

failure_reason validate_payload_image_mauv(
    const struct libcr51sign_ctx* ctx, const struct libcr51sign_intf* intf,
    uint32_t payload_blob_offset, uint32_t payload_blob_size);

#ifdef __cplusplus
} //  extern "C"
#endif

#endif // PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_MAUV_H_

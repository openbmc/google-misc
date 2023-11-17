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

#ifndef PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_INTERNAL_H_
#define PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_INTERNAL_H_

#include <libcr51sign/libcr51sign.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef USER_PRINT
#define CPRINTS(ctx, ...) fprintf(stderr, __VA_ARGS__)
#endif

#define MEMBER_SIZE(type, field) sizeof(((type*)0)->field)

typedef enum libcr51sign_validation_failure_reason failure_reason;

#ifdef __cplusplus
} //  extern "C"
#endif

#endif // PLATFORMS_HAVEN_LIBCR51SIGN_LIBCR51SIGN_INTERNAL_H_

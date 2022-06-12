// Copyright 2022 Google LLC
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

#include "blobHandler.hpp"

#include <blobs-ipmid/blobs.hpp>

#include <memory>

// Extern "C" is used due to the usage of dlopen() for loading IPMI handlers
// and IPMI blob handlers. This happens in two steps:
//
// 1) ipmid loads all libraries in /usr/lib/ipmid-providers from its
//    loadLibraries() function using dlopen().
// 2) The blobs library, libblobcmds.so, loads all libraries in
//    /usr/lib/blobs-ipmid from its loadLibraries() function. It uses dlsym()
//    to locate the createHandler function by its un-mangled name
//    "createHandler". Using extern "C" prevents its name from being mangled
//    into, for example, "_Z13createHandlerv".

extern "C" std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    return std::make_unique<blobs::BlobHandler>();
}

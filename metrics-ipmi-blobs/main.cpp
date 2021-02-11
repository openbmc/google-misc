#include "handler.hpp"

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
    return std::make_unique<blobs::MetricBlobHandler>();
}

#include <blobs-ipmid/blobs.hpp>
#include <ec-ipmi-blobs/ipmid.hpp>
#include <ec-ipmi-blobs/skm/handler.hpp>

#include <memory>

extern "C" std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    return std::make_unique<blobs::ec::skm::Handler>(blobs::ec::getIpmidCmd());
}

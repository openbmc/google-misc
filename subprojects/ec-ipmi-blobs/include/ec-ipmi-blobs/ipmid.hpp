#pragma once
#include <ec-ipmi-blobs/cmd.hpp>
#include <ec-ipmi-blobs/io_uring.hpp>

namespace blobs
{
namespace ec
{

IoUring& getIpmidRing();
Cmd& getIpmidCmd();

} // namespace ec
} // namespace blobs

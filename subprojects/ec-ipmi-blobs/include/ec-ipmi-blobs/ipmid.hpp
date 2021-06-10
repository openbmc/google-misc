#pragma once
#include <ec-ipmi-blobs/cmd.hpp>
#include <stdplus/io_uring.hpp>

namespace blobs
{
namespace ec
{

stdplus::IoUring& getIpmidRing();
Cmd& getIpmidCmd();

} // namespace ec
} // namespace blobs

#include "dbus_command_impl.hpp"
#include "hoth_command.hpp"

#include <blobs-ipmid/blobs.hpp>

#include <memory>

extern "C" std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    /** @brief Default instantiation of Dbus */
    static ipmi_hoth::internal::DbusCommandImpl dbusCommand_impl;

    return std::make_unique<ipmi_hoth::HothCommandBlobHandler>(
        &dbusCommand_impl);
}

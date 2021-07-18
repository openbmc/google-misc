// Copyright 2021 Google LLC
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

#include "dbus.hpp"

#include "dbus_impl.hpp"

#include <ipmid/api.h>

#include <sdbusplus/exception.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace ipmi_hoth
{
namespace internal
{

DbusImpl::DbusImpl() : bus(ipmid_get_sd_bus_connection())
{}

Dbus::SubTreeMapping DbusImpl::getHothdMapping()
{
    auto req =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    req.append("/");
    req.append(0);
    req.append(std::vector<std::string>({"com.google.gbmc.Hoth"}));
    auto rsp = bus.call(req, kTimeout);
    SubTreeMapping mapping;
    rsp.read(mapping);
    return mapping;
}

std::string hothIdToSvc(std::string_view hothId)
{
    std::string svc = "com.google.gbmc.Hoth";
    if (!hothId.empty())
    {
        svc += ".";
        svc += hothId;
    }
    return svc;
}

bool DbusImpl::pingHothd(std::string_view hothId)
{
    try
    {
        auto req =
            bus.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetNameOwner");
        req.append(hothIdToSvc(hothId));
        bus.call(req, kTimeout);
        return true;
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }
}

sdbusplus::message::message DbusImpl::newHothdCall(std::string_view hothId,
                                                   const char* intf,
                                                   const char* method)
{
    std::string svc = hothIdToSvc(hothId);
    std::string obj = "/com/google/gbmc/Hoth";
    if (!hothId.empty())
    {
        obj += "/";
        obj += hothId;
    }
    return bus.new_method_call(svc.c_str(), obj.c_str(), intf, method);
}

sdbusplus::message::message DbusImpl::newHothdCall(std::string_view hothId,
                                                   const char* method)
{
    return newHothdCall(hothId, "com.google.gbmc.Hoth", method);
}

} // namespace internal

} // namespace ipmi_hoth

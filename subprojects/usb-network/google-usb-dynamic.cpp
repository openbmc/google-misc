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

#include <fmt/format.h>
#include <systemd/sd-daemon.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace google
{
namespace usb_network
{

constexpr char kInterface[] = "com.google.gbmc.USB";

using sdeventplus::source::Signal;
using USBProps =
    std::unordered_map<std::string,
                       std::variant<std::monostate, std::string, uint64_t>>;

void execute(std::span<std::string> args)
{
    fmt::print(stderr, "Executing: usb_network.sh `{}`\n",
               fmt::join(args, "` `"));
    pid_t pid = fork();
    if (pid != 0)
    {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0)
        {
            throw std::runtime_error(
                fmt::format("Execution failed: {}", status));
        }
        return;
    }
    std::vector<char*> cptr;
    cptr.reserve(args.size() + 3);
    char arg0[] = "usb_network.sh";
    cptr.push_back(arg0);
    for (auto& a : args)
    {
        cptr.push_back(a.data());
    }
    cptr.push_back(nullptr);
    exit(execv("/usr/bin/usb_network.sh", cptr.data()));
}

class DeviceManager
{
  public:
    DeviceManager(sdbusplus::bus_t& bus) : bus(bus)
    {}
    ~DeviceManager();

    void handleInterfacesAdded(sdbusplus::message_t& m);
    void handleInterfacesRemoved(sdbusplus::message_t& m);
    void populate();

  private:
    std::reference_wrapper<sdbusplus::bus_t> bus;
    struct Device
    {
        size_t idx;
        std::vector<std::string> args;
    };
    std::vector<std::unique_ptr<Device>> devices;
    std::unordered_map<std::string, std::reference_wrapper<Device>> device_map;

    void addDev(const char* obj, USBProps& props);
    void readDev(const char* svc, const char* obj, const char* intf);
};

DeviceManager::~DeviceManager()
{
    for (auto& dev : devices)
    {
        if (dev)
        {
            dev->args.push_back("stop");
            try
            {
                execute(dev->args);
            }
            catch (const std::exception& e)
            {
                fmt::print(stderr, "Cleanup: {}\n", e.what());
            }
        }
    }
}

template <typename>
inline constexpr bool always_false_v = false;

void DeviceManager::addDev(const char* obj, USBProps& props)
{
    size_t idx = 0;
    auto it = device_map.find(obj);
    if (it != device_map.end())
    {
        idx = it->second.get().idx;
    }
    else
    {
        for (; idx < devices.size(); ++idx)
        {
            if (!devices[idx])
            {
                break;
            }
        }
    }
    auto device = std::make_unique<Device>(idx);
    auto& args = device->args;

    auto add = [&](const char* arg, const char* key, bool required) {
        auto pn = props.find(key);
        if (pn != props.end())
        {
            args.push_back(arg);
            std::visit(
                [&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_arithmetic_v<T>)
                    {
                        args.push_back(fmt::format("{}", arg));
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        args.push_back(std::move(arg));
                    }
                    else if constexpr (std::is_same_v<T, std::monostate>)
                    {
                        throw std::runtime_error(fmt::format(
                            "Obj {} unrecognized type for {}", obj, key));
                    }
                    else
                    {
                        static_assert(always_false_v<T>, "Invalid type");
                    }
                },
                pn->second);
        }
        else if (required)
        {
            throw std::runtime_error(
                fmt::format("Obj {} missing param {}", obj, key));
        }
    };
    add("--product-id", "ProductId", /*required=*/true);
    add("--bind-device", "BindDevice", /*required=*/true);
    add("--product-name", "ProductName", /*required=*/false);
    add("--dev-type", "DevType", /*required=*/false);

    auto pn = props.find("IFName");
    std::string ifname;
    if (pn != props.end())
    {
        ifname = std::move(std::get<std::string>(pn->second));
    }
    else
    {
        ifname = fmt::format("gusbem{}", idx);
    }
    args.push_back("--iface-name");
    args.push_back(ifname);

    if (it != device_map.end())
    {
        if (it->second.get().args == args)
        {
            fmt::print(stderr, "Device config {} duplicate, ignoring\n", obj);
            return;
        }
        fmt::print(stderr, "Replacing interface {}\n", obj);
        it->second.get().args.push_back("stop");
        execute(it->second.get().args);
    }
    else
    {
        fmt::print(stderr, "Adding interface {}\n", obj);
    }
    execute(args);

    if (idx >= devices.size())
    {
        devices.resize(devices.size() * 2 + 7);
    }
    devices[idx] = std::move(device);
    if (it != device_map.end())
    {
        it->second = *devices[idx];
    }
    else
    {
        device_map.emplace(obj, *devices[idx]);
    }
}

void DeviceManager::handleInterfacesAdded(sdbusplus::message_t& m)
{
    sdbusplus::message::object_path path;
    std::unordered_map<std::string, USBProps> ip;
    m.read(path, ip);

    auto it = ip.find(kInterface);
    if (it == ip.end())
    {
        return;
    }
    addDev(path.str.c_str(), it->second);
}

void DeviceManager::handleInterfacesRemoved(sdbusplus::message_t& m)
{
    sdbusplus::message::object_path path;
    std::unordered_set<std::string> ip;
    m.read(path, ip);

    if (!ip.contains(kInterface))
    {
        return;
    }

    auto it = device_map.find(path.str);
    if (it == device_map.end())
    {
        return;
    }
    auto dev = std::move(devices[it->second.get().idx]);
    device_map.erase(it);

    dev->args.push_back("stop");
    execute(dev->args);
}

void DeviceManager::readDev(const char* svc, const char* obj, const char* intf)
{
    USBProps props;
    auto req = bus.get().new_method_call(
        svc, obj, "org.freedesktop.DBus.Properties", "GetAll");
    req.append(intf);
    req.call().read(props);
    addDev(obj, props);
}

void DeviceManager::populate()
{
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        subtree;
    auto req = bus.get().new_method_call("xyz.openbmc_project.ObjectMapper",
                                         "/xyz/openbmc_project/object_mapper",
                                         "xyz.openbmc_project.ObjectMapper",
                                         "GetSubTree");
    req.append("/", int32_t{0}, std::vector<std::string>{kInterface});
    req.call().read(subtree);
    for (const auto& [obj, svcm] : subtree)
    {
        for (const auto& [svc, intfs] : svcm)
        {
            for (const auto& intf : intfs)
            {
                if (intf != kInterface)
                {
                    continue;
                }
                try
                {
                    readDev(svc.c_str(), obj.c_str(), intf.c_str());
                }
                catch (const std::exception& e)
                {
                    fmt::print(stderr, "Init {}: {}\n", obj, e.what());
                }
            }
        }
    }
}

int execute()
{
    // Set up our DBus and event loop
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    DeviceManager devm(bus);

    // Configure basic signal handling
    auto exit_handler = [&event](Signal&, const struct signalfd_siginfo*) {
        fmt::print(stderr, "Interrupted, Exiting\n");
        event.exit(0);
    };
    stdplus::signal::block(SIGINT);
    Signal sig_int(event, SIGINT, exit_handler);
    stdplus::signal::block(SIGTERM);
    Signal sig_term(event, SIGTERM, exit_handler);

    sdbusplus::bus::match_t addedMatch(
        bus, sdbusplus::bus::match::rules::interfacesAdded(),
        [&devm](sdbusplus::message_t& m) {
            try
            {
                devm.handleInterfacesAdded(m);
            }
            catch (const std::exception& e)
            {
                fmt::print(stderr, "Add handler: {}\n", e.what());
            }
        });
    sdbusplus::bus::match_t removedMatch(
        bus, sdbusplus::bus::match::rules::interfacesRemoved(),
        [&devm](sdbusplus::message_t& m) {
            try
            {
                devm.handleInterfacesRemoved(m);
            }
            catch (const std::exception& e)
            {
                fmt::print(stderr, "Removed handler: {}\n", e.what());
            }
        });

    devm.populate();

    sd_notify(0, "READY=1");
    return event.loop();
}

} // namespace usb_network
} // namespace google

int main(int, char*[])
{
    try
    {
        return google::usb_network::execute();
    }
    catch (const std::exception& e)
    {
        fmt::print(stderr, "FAILED: {}\n", e.what());
        return 1;
    }
}

#include "daemon.hpp"
#include "port.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <variant>
#include <vector>

const std::string serviceName = "xyz.openbmc_project.Metric";
const std::string bmcObjectPath = "/xyz/openbmc_project/metric/bmc0";
const std::string bmcInterfaceName = "xyz.openbmc_project.Metric.BMC";
const std::string associationInterfaceName =
    "xyz.openbmc_project.Association.Definitions";

class MetricCollector
{
  public:
    MetricCollector(boost::asio::io_context& ioc,
                    std::shared_ptr<sdbusplus::asio::connection> conn,
                    sdbusplus::asio::object_server& objServer);

    void registerPorts();
    void registerAssociations();
    void addPortAssociations(std::vector<Association>& associations);
    void addDaemonAssociations(std::vector<Association>& associations);
    std::optional<std::string> getDaemonObjectPathFromPid(const int pid);
    void registerDaemons();
    void unregisterPorts();
    void unregisterDaemons();
    void update();
    void updateBootCount();
    int getNumPorts();

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server& objServer;

    size_t bootCount;
    size_t crashCount;
    std::vector<Port> bmcPorts;
    std::vector<Daemon> bmcDaemons;
};
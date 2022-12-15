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

#include <array>
#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

const std::string serviceName = "xyz.openbmc_project.Metric";
const std::string bmcObjectPath = "/xyz/openbmc_project/metric/bmc0";
const std::string bmcInterfaceName = "xyz.openbmc_project.Metric.BMC";
const std::string associationInterfaceName =
    "xyz.openbmc_project.Association.Definitions";

constexpr std::string_view procPath = "/proc/";

namespace {

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
    void removeAssociation(const std::string objectPath);

    std::optional<std::string> getDaemonObjectPathFromPid(const int pid);
    void registerDaemons();

    void updatePorts();
    void updateDaemons();
    void update();

    void updateBootCount();
    int getNumPorts();

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server& objServer;

    size_t bootCount;
    size_t crashCount;
    std::unordered_map<std::string, Port> bmcPorts;
    std::unordered_map<int, Daemon> bmcDaemons;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association_interface;
    std::vector<Association> associations;
};

}
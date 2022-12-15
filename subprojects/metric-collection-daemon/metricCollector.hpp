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

const std::string objectPath = "/xyz/openbmc_project/metric/bmc0";
const std::string interfaceName = "xyz.openbmc_project.Metric";

const std::string bootCountName = "BootCount";

class MetricCollector
{
  public:
    MetricCollector(boost::asio::io_context& ioc,
                    sdbusplus::asio::connection& bus,
                    sdbusplus::asio::object_server& objServer) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer)
    {
        metrics_ = objServer_.add_unique_interface(objectPath, interfaceName);

        metrics_->register_property(bootCountName, bootCount);

        metrics_->initialize();
    }

    void updateBootCount()
    {
        bootCount++;
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::unique_ptr<sdbusplus::asio::dbus_interface> metrics_;
    size_t bootCount = 0;
};
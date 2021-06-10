#include <boost/asio/posix/stream_descriptor.hpp>
#include <ec-ipmi-blobs/cmd_net.hpp>
#include <ec-ipmi-blobs/ipmid.hpp>
#include <ipmid/api.hpp>

namespace blobs
{
namespace ec
{

using boost::asio::posix::stream_descriptor;

class IpmidIoUring : public stdplus::IoUring
{
  public:
    IpmidIoUring() : stdplus::IoUring(), pd(*getIoContext(), getEventFd().get())
    {
        scheduleEventProcess();
    }

    void scheduleEventProcess()
    {
        pd.async_wait(stream_descriptor::wait_read,
                      [&](const boost::system::error_code&) {
                          this->processEvents();
                          this->scheduleEventProcess();
                      });
    }

  private:
    stream_descriptor pd;
};

stdplus::IoUring& getIpmidRing()
{
    static IpmidIoUring ring;
    return ring;
}

Cmd& getIpmidCmd()
{
    // The EC will always live at this fixed IP on the BMC network
    static CmdNet cmd(getIpmidRing(), "fdb5:0481:10ce::42ff:fe4d:4300");
    return cmd;
}

} // namespace ec
} // namespace blobs

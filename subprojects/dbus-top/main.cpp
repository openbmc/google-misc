#include <ncurses.h>
#include <stdio.h>
#include <systemd/sd-event.h>

#include <sdbusplus/bus.hpp>

int WIN_H, WIN_W;

sdbusplus::bus::bus* g_bus;

int Foo(const std::string& dbus_iface, const std::string& desc, int x, int y)
{
    // Find all chassis in the system
    sdbusplus::message::message msg = g_bus->new_method_call(
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths");
    // Signature of GetSubTreePaths: sias
    msg.append("/"); // Req Path
    msg.append(0);   // Depth

    msg.append(std::vector<std::string>{dbus_iface});
    sdbusplus::message::message reply = g_bus->call(msg, 0);
    std::vector<std::string> chassisIds;

    char buf[200];

    if (!strcmp(reply.get_signature(), "as"))
    {
        reply.read(chassisIds);
        sprintf(buf, "Found %u %s:", chassisIds.size(), desc.c_str());
        mvaddstr(y, x, buf);
        for (int i = 0; i < int(chassisIds.size()); i++)
        {
            sprintf(buf, "%d: %s", i, chassisIds[i].c_str());
            mvaddstr(y + 1 + i, x, buf);
        }
    }
    else
    {
        sprintf(buf, "Did not find any chassis, cannot create association");
        mvaddstr(y, x, buf);
    }
    return int(chassisIds.size());
}

void Test1()
{
    initscr();
    noecho();
    getmaxyx(stdscr, WIN_H, WIN_W);
    clear();

    int y = 3, x = 3;
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    g_bus = &bus;
    int ret =
        Foo("xyz.openbmc_project.Inventory.Item.Chassis", "chassis", x, y);
    y = y + ret + 2;
    Foo("xyz.openbmc_project.Inventory.Item.Bmc", "bmc", x, y);

    refresh();
    getch();
    endwin();
    echo();
}

int main(int argc, char** argv)
{
    Test1();
    return 0;
}

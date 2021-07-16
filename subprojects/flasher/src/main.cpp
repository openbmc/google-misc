#include <sys/utsname.h>

#include <flasher/args.hpp>
#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/logging.hpp>
#include <flasher/ops.hpp>
#include <stdplus/util/cexec.hpp>

#include <optional>
#include <stdexcept>

namespace flasher
{

void main_wrapped(int argc, char* argv[])
{
    using stdplus::fd::OpenAccess;
    using stdplus::fd::OpenFlag;
    using stdplus::fd::OpenFlags;

    auto args = Args::argsOrHelp(argc, argv);
    reinterpret_cast<uint8_t&>(logLevel) += args.verbose;

    switch (args.op)
    {
        case Args::Op::Automatic:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            ops::automatic(*dev, args.dev_offset, *file, args.file_offset,
                           args.max_size, args.stride, args.noread);
            if (args.verify)
            {
                ops::verify(*dev, args.dev_offset, *file, args.file_offset,
                            args.max_size, args.stride);
            }
        }
        break;
        case Args::Op::Read:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::WriteOnly)
                                                 .set(OpenFlag::Create)
                                                 .set(OpenFlag::Trunc));
            ops::read(*dev, args.dev_offset, *file, args.file_offset,
                      args.max_size, args.stride);
        }
        break;
        case Args::Op::Write:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            ops::write(*dev, args.dev_offset, *file, args.file_offset,
                       args.max_size, args.stride, args.noread);
            if (args.verify)
            {
                ops::verify(*dev, args.dev_offset, *file, args.file_offset,
                            args.max_size, args.stride);
            }
        }
        break;
        case Args::Op::Erase:
        {
            auto dev = openDevice(*args.dev);
            ops::erase(*dev, args.dev_offset, args.max_size, args.stride,
                       args.noread);
            if (args.verify)
            {
                throw std::runtime_error("Not implemented");
            }
        }
        break;
        case Args::Op::Verify:
        {
            auto dev = openDevice(*args.dev);
            auto file = openFile(*args.file, OpenFlags(OpenAccess::ReadOnly));
            ops::verify(*dev, args.dev_offset, *file, args.file_offset,
                        args.max_size, args.stride);
        }
        break;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        main_wrapped(argc, argv);
        return 0;
    }
    catch (const std::exception& e)
    {
        log(LogLevel::Error, "ERROR: {}\n", e.what());
        return 1;
    }
}

} // namespace flasher

int main(int argc, char* argv[])
{
    return flasher::main(argc, argv);
}

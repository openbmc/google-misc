#include <fmt/format.h>
#include <getopt.h>

#include <flasher/args.hpp>
#include <flasher/convert.hpp>
#include <flasher/device.hpp>
#include <flasher/file.hpp>
#include <flasher/mutate.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace flasher
{

const std::map<std::string_view, Args::Op> string_to_op = {
    {"auto", Args::Op::Automatic}, {"read", Args::Op::Read},
    {"write", Args::Op::Write},    {"erase", Args::Op::Erase},
    {"verify", Args::Op::Verify},
};

Args::Args(int argc, char* argv[])
{
    static const char opts[] = ":m:no:O:rs:S:v";
    static const struct option longopts[] = {
        {"mutate", required_argument, nullptr, 'm'},
        {"and-verify", no_argument, nullptr, 'n'},
        {"dev-offset", required_argument, nullptr, 'o'},
        {"file-offset", required_argument, nullptr, 'O'},
        {"no-read", no_argument, nullptr, 'r'},
        {"size", required_argument, nullptr, 's'},
        {"stride", required_argument, nullptr, 'S'},
        {"verbose", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0},
    };
    int c;
    optind = 0;
    while ((c = getopt_long(argc, argv, opts, longopts, nullptr)) > 0)
    {
        switch (c)
        {
            case 'm':
                mutate.emplace_back(optarg);
                break;
            case 'n':
                verify = true;
                break;
            case 'o':
                dev_offset = toUint32(optarg);
                break;
            case 'O':
                file_offset = toUint32(optarg);
                break;
            case 'r':
                noread = true;
                break;
            case 's':
                max_size = toUint32(optarg);
                break;
            case 'S':
                stride = toUint32(optarg);
                break;
            case 'v':
                verbose++;
                break;
            case ':':
                throw std::runtime_error(
                    fmt::format("Missing argument for `{}`", argv[optind - 1]));
                break;
            default:
                throw std::runtime_error(fmt::format(
                    "Invalid command line argument `{}`", argv[optind - 1]));
        }
    }

    if (optind == argc)
    {
        throw std::runtime_error("Mising flasher operation");
    }
    auto it = string_to_op.find(argv[optind]);
    if (it == string_to_op.end())
    {
        throw std::runtime_error(
            fmt::format("Invalid operation: {}", argv[optind]));
    }
    optind++;
    op = it->second;
    switch (op)
    {
        case Args::Op::Automatic:
        case Args::Op::Write:
        case Args::Op::Verify:
            if (optind + 2 != argc)
            {
                throw std::runtime_error("Must specify FILE and DEV");
            }
            file.emplace(argv[optind]);
            dev.emplace(argv[optind + 1]);
            break;
        case Args::Op::Read:
            if (optind + 2 != argc)
            {
                throw std::runtime_error("Must specify DEV and FILE");
            }
            dev.emplace(argv[optind]);
            file.emplace(argv[optind + 1]);
            break;
        case Args::Op::Erase:
            if (optind + 1 != argc)
            {
                throw std::runtime_error("Must specify DEV");
            }
            dev.emplace(argv[optind]);
            break;
    }
}

void Args::printHelp(const char* arg0)
{
    fmt::print(stderr, "Usage: {} [OPTION]... auto FILE DEV\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... read DEV FILE\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... write FILE DEV\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... erase DEV\n", arg0);
    fmt::print(stderr, "   or: {} [OPTION]... verify FILE DEV\n", arg0);
    fmt::print(stderr, "\n");
    fmt::print(stderr, "Optional Arguments:\n");
    fmt::print(
        stderr,
        "  -m, --mutate[=MUTATE_OPTS]   Applies another mutation from the file"
        "contents during operation\n");
    fmt::print(stderr, "  -n, --and-verify             Verifies the flash "
                       "contents during operation\n");
    fmt::print(
        stderr,
        "  -o, --dev-offset[=OFFSET]    The device offset starting point\n");
    fmt::print(
        stderr,
        "  -O, --file-offset[=OFFSET]   The file offset starting point\n");
    fmt::print(stderr, "  -r, --no-read                Doesn't read the flash "
                       "before erasing or writing\n");
    fmt::print(
        stderr,
        "  -s, --size[=SIZE]            The max size to write to the flash\n");
    fmt::print(
        stderr,
        "  -S, --stride[=SIZE]          The number of bytes per operation\n");
    fmt::print(stderr, "  -v, --verbose                Increases the verbosity "
                       "level of error message output\n");
    fmt::print(stderr, "\n");

    fmt::print(stderr,
               "FILE options (separated by ,) (simple is the default):\n");
    for (const auto& [_, type] : fileTypes)
    {
        type->printHelp();
    }
    fmt::print(stderr, "\n");

    fmt::print(stderr, "MUTATION options (separated by ,)\n");
    for (const auto& [_, type] : mutateTypes)
    {
        type->printHelp();
    }
    fmt::print(stderr, "\n");

    fmt::print(stderr, "DEVICE options (separated by ,)\n");
    for (const auto& [_, type] : deviceTypes)
    {
        type->printHelp();
    }
    fmt::print(stderr, "\n");

    fmt::print(stderr, "Ex: {} -n -m rot128 auto image.bin mtd,/dev/mtd7\n",
               arg0);
    fmt::print(stderr,
               "Ex: {} erase fake,type=nor,erase=4096,size=16384,fake.img\n",
               arg0);
    fmt::print(stderr, "\n");
}

Args Args::argsOrHelp(int argc, char* argv[])
{
    try
    {
        return Args(argc, argv);
    }
    catch (...)
    {
        printHelp(argv[0]);
        throw;
    }
}

} // namespace flasher

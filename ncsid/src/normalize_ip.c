#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    if (argc < 1)
    {
        return 1;
    }
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ip address>\n", argv[0]);
        return 1;
    }

    union
    {
        struct in_addr in;
        struct in6_addr in6;
    } buf;
    int af = AF_INET6;
    if (inet_pton(af, argv[1], &buf) != 1)
    {
        af = AF_INET;
        if (inet_pton(af, argv[1], &buf) != 1)
        {
            fprintf(stderr, "Invalid IP Address: %s\n", argv[1]);
            return 2;
        }
    }

    char str[INET6_ADDRSTRLEN];
    if (inet_ntop(af, &buf, str, sizeof(str)) == NULL)
    {
        return 1;
    }

    printf("%s\n", str);
    return 0;
}

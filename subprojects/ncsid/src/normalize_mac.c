/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <stdio.h>

#define ETH_STRLEN (ETH_ALEN * 3)
#define HEX_OUT "%02" PRIx8
#define ETH_OUT                                                                \
    HEX_OUT ":" HEX_OUT ":" HEX_OUT ":" HEX_OUT ":" HEX_OUT ":" HEX_OUT
#define HEX_IN "%2" SCNx8
#define ETH_IN HEX_IN ":" HEX_IN ":" HEX_IN ":" HEX_IN ":" HEX_IN ":" HEX_IN

int to_ether_addr(const char* str, struct ether_addr* ret)
{
    char sentinel;
    return sscanf(str, ETH_IN "%c", &ret->ether_addr_octet[0],
                  &ret->ether_addr_octet[1], &ret->ether_addr_octet[2],
                  &ret->ether_addr_octet[3], &ret->ether_addr_octet[4],
                  &ret->ether_addr_octet[5], &sentinel) != ETH_ALEN;
}

void from_ether_addr(const struct ether_addr* addr, char* ret)
{
    sprintf(ret, ETH_OUT, addr->ether_addr_octet[0], addr->ether_addr_octet[1],
            addr->ether_addr_octet[2], addr->ether_addr_octet[3],
            addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
}

int main(int argc, char* argv[])
{
    if (argc < 1)
    {
        return 1;
    }
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <mac address>\n", argv[0]);
        return 1;
    }

    struct ether_addr addr;
    if (to_ether_addr(argv[1], &addr) != 0)
    {
        fprintf(stderr, "Invalid MAC Address: %s\n", argv[1]);
        return 2;
    }

    char str[ETH_STRLEN];
    from_ether_addr(&addr, str);
    printf("%s\n", str);
    return 0;
}

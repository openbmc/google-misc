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

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

#pragma once

#include <sys/types.h>

namespace net
{

class SockIO
{
  public:
    SockIO() = default;
    explicit SockIO(int sockfd) : sockfd_{sockfd}
    {}
    virtual ~SockIO();

    int get_sockfd() const
    {
        return sockfd_;
    }

    virtual int write(const void* buf, size_t len);

    virtual int close();

    virtual int recv(void* buf, size_t maxlen);

  protected:
    int sockfd_ = -1;
};

} // namespace net

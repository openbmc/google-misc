# Nemora-postd

Nemora-postd is a daemon running on the BMC to stream host POST codes.

## Tests

The following instruction is for manual testing, but the Robot Framework test
can follow the same logic.

### Prerequisites

Install the latest version of
[Protocol Buffers](https://github.com/protocolbuffers/protobuf/releases/tag/v3.13.0),
and the latest version of
[Linux / UNIX TCP Port Forwarder](http://www.dest-unreach.org/socat/).

### Functional Tests

1. On BMC, stop the existing Nemora-postd;

   ```bash
   systemctl stop nemora-postd@eth0
   ```

2. On any machine, start a testing server at IP_SERVER listening to upcoming UDP
   datagrams;

   ```bash
   DECODE_CMD="protoc --decode=platforms.nemora.proto.EventSeries event_message.proto"
   exec socat udp-recvfrom:3960,fork exec:"$DECODE_CMD",fdout=stdout
   ```

3. On BMC, start a new nemora session which sends POST codes to the testing
   server;

   ```bash
   nemora-postd eth0 --udp4 $IP_SERVER
   ```

4. On BMC, manually change the DBus property via `busctl`;

   ```bash
   busctl set-property xyz.openbmc_project.State.Boot.Raw /xyz/openbmc_project/state/boot/raw0 xyz.openbmc_project.State.Boot.Raw Value '('tay')' 10000004 3 1 2 3
   ```

5. The testing server should receive the following packet in about 20 seconds.

   ```bash
   magic: 9876039085048616960
   mac: ...
   sent_time_us: ...
   postcodes: 0x989684
   postcodes_protocol: NATIVE_32_BIT
   ```

   Note that, `magic` and `postcodes_protocol` are fixed. `postcodes` should be
   what you set via `busctl`.

/**
 * vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 *
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <variant>
#include <vector>

#include "config.hpp"

static constexpr auto SNOOP_BUSNAME = "xyz.openbmc_project.State.Boot.Raw";
static constexpr auto WATCHDOG_PATH = "/xyz/openbmc_project/watchdog/host0";
static constexpr auto WATCHDOG_SERVICE = "xyz.openbmc_project.Watchdog";
static constexpr auto WATCHDOG_INTF = "xyz.openbmc_project.State.Watchdog";
static constexpr auto PROP_INTF = "org.freedesktop.DBus.Properties";
static constexpr auto MATCH =
    "type='signal',"
    "interface='org.freedesktop.DBus.Properties',"
    "member='PropertiesChanged',"
    "path='/xyz/openbmc_project/state/boot/raw'";

/*
 * This file contains either "true" or "false" and that controls whether or
 * not this daemon will enable the watchdog daemon upon receiving a post
 * code.  This change only exists to help transition Iceblink from not having
 * a watchdog to having one.
 */
static constexpr auto ENABLE_CONF = "/etc/watchdog.conf.d/enabled";
/*
 * Whether or not to enable the watchdog timeout every time we receive a POST
 * code.
 */
static bool enable = false;

/*
 * Reset the watchdog.
 *
 * This method takes into account the POST code received.
 */
static void ResetWatchdog(uint64_t code);

/*
 * Handle incoming dbus signal we care about.
 */
static int DbusHandleSignal(sd_bus_message* msg, void* data, sd_bus_error* err);

/*
 * Given a PoST code received over dbus return 0 if the configured
 * TimeRemaining value to use if found, otherwise return -1.
 */
static int GetTimeForCode(uint64_t code, uint64_t* value);

// Example object that listens for dbus updates.
class SnoopListen {
 public:
  SnoopListen(sdbusplus::bus::bus& bus)
      : _bus(bus), _signal(bus, MATCH, DbusHandleSignal, this) {}

 private:
  sdbusplus::bus::bus& _bus;
  sdbusplus::server::match::match _signal;
};

int main() {
  std::ifstream ifs;
  ifs.open(ENABLE_CONF);
  if (ifs.good()) {
    ifs >> std::boolalpha >> enable;
  } else {
    /*
     * If this file isn't present, the default behaviour is to not enable
     * the watchdog service every time a new PoST code is received.
     */
    fprintf(stderr, "Unable to open or process enable configuration: '%s'\n",
            ENABLE_CONF);
  }

  auto ListenBus = sdbusplus::bus::new_default();
  std::unique_ptr<SnoopListen> snoop = std::make_unique<SnoopListen>(ListenBus);

  while (true) {
    ListenBus.process_discard();
    ListenBus.wait();
  }

  return 0;
}

static int GetTimeForCode(uint64_t code, uint64_t* value) {
  if (!value) {
    return -1;
  }

  for (auto& curr : IntervalOverrideConfig) {
    if (curr.code == code) {
      (*value) = curr.value;
      return 0;
    }
  }

  return -1;
}

static void ResetWatchdog(uint64_t code) {
  auto WatchdogBus = sdbusplus::bus::new_default();
  uint64_t timeRemaining;

  if (enable) {
    // TODO(venture): We're going to set the watchdog to enabled
    // when we see it.
    auto pEnbMsg = WatchdogBus.new_method_call(WATCHDOG_SERVICE, WATCHDOG_PATH,
                                               PROP_INTF, "Set");
    using Boolean = std::variant<bool>;
    Boolean enabled{true};
    pEnbMsg.append(WATCHDOG_INTF);
    pEnbMsg.append("Enabled");
    pEnbMsg.append(enabled);

    auto enbResponseMsg = WatchdogBus.call(pEnbMsg);
    if (enbResponseMsg.is_method_error()) {
      fprintf(stderr, "Failed to enable watchdog\n");
      return;
    }
  }

  auto pReadMsg = WatchdogBus.new_method_call(WATCHDOG_SERVICE, WATCHDOG_PATH,
                                              PROP_INTF, "Get");
  pReadMsg.append(WATCHDOG_INTF);
  pReadMsg.append("Interval");

  auto valueResponseMsg = WatchdogBus.call(pReadMsg);
  if (valueResponseMsg.is_method_error()) {
    fprintf(stderr, "Failed to Get Interval for watchdog\n");
    return;
  }

  using Value = std::variant<uint64_t>;
  Value previousInterval;
  valueResponseMsg.read(previousInterval);

  if (0 != GetTimeForCode(code, &timeRemaining)) {
    // To get here, we should just use the current Interval.
    timeRemaining = std::get<uint64_t>(previousInterval);
  }

  Value remaining{timeRemaining};

  auto pWriteMsg = WatchdogBus.new_method_call(WATCHDOG_SERVICE, WATCHDOG_PATH,
                                               PROP_INTF, "Set");
  pWriteMsg.append(WATCHDOG_INTF);
  pWriteMsg.append("TimeRemaining");
  pWriteMsg.append(remaining);

  auto responseMsg = WatchdogBus.call(pWriteMsg);
  if (responseMsg.is_method_error()) {
    fprintf(stderr, "Failed to Set TimeRemaining for watchdog\n");
    return;
  }

  /*
   * Setting TimeRemaining sets Interval, which makes the change have more
   * of an impact than I would want.  We want to effectively revert the
   * Interval value back to what it had been.  This allows us to have a
   * one-time longer TimeRemaining if necessary without influencing future
   * updates.
   *
   * This code preserves the previous Interval value, because setting the
   * TimeRemaining value with phosphor-watchdog blows away the previous
   * Interval value
   */
  auto pSetIntervalMsg = WatchdogBus.new_method_call(
      WATCHDOG_SERVICE, WATCHDOG_PATH, PROP_INTF, "Set");
  pSetIntervalMsg.append(WATCHDOG_INTF);
  pSetIntervalMsg.append("Interval");
  pSetIntervalMsg.append(previousInterval);

  auto setIntervalMsg = WatchdogBus.call(pSetIntervalMsg);
}

static int DbusHandleSignal(sd_bus_message* msg, void* data,
                            sd_bus_error* err) {
  auto sdbpMsg = sdbusplus::message::message(msg);

  std::string msgSensor, busName{SNOOP_BUSNAME};
  std::map<std::string, std::variant<uint64_t>> msgData;
  sdbpMsg.read(msgSensor, msgData);

  if (msgSensor == busName) {
    auto valPropMap = msgData.find("Value");
    if (valPropMap != msgData.end()) {
      uint64_t rawValue = std::get<uint64_t>(valPropMap->second);
      ResetWatchdog(rawValue);
    }
  }

  return 0;
}

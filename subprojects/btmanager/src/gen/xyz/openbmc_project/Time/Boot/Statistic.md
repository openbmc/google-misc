# xyz.openbmc_project.Time.Boot.Statistic

Host reboot statistics.


## Methods
No methods.

## Properties
| name | type | description |
|------|------|-------------|
| **InternalRebootCount** | uint32 | The count of internal reboot during a power cycle. |
| **PowerCycleType** | enum[self.PowerCycleType] | AC power cycle or DC power cycle. |

## Signals
No signals.

## Enumerations
### PowerCycleType

AC power cycle or DC power cycle.


| name | description |
|------|-------------|
| **ACPowerCycle** | Last power cycle is AC power. |
| **DCPowerCycle** | Last power cycle is DC power. |


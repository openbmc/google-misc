# xyz.openbmc_project.Time.Boot.HostBootTime

Implement to manage host boot time.


## Methods
### Notify

Notifies BMC to label current monotonic time as <Timepoint> of a stage.


#### Parameters and Returns
| direction | name | type | description |
|:---------:|------|------|-------------|
| in | **Timepoint** | byte | Current timepoint code. |
| out | **Timestamp** | uint64 | The timestamp that BMC give to this stage. |

#### Errors
 * xyz.openbmc_project.Time.Boot.HostBootTime.Error.UnsupportedTimepoint
 * xyz.openbmc_project.Time.Boot.HostBootTime.Error.WrongOrder

### SetDuration

Directly set duration to a stage.


#### Parameters and Returns
| direction | name | type | description |
|:---------:|------|------|-------------|
| in | **Stage** | string | Stage name. |
| in | **DurationMicrosecond** | uint64 | Duration in microsecond of this stage. |
| out | **State** | enum[self.SetDurationStates] | The timestamp that BMC give to this stage. |



## Properties
No properties.

## Signals
No signals.

## Enumerations
### SetDurationStates

Identifies if the SetDuration has encountered an error or not.


| name | description |
|------|-------------|
| **KeyDurationSet** | Key duration is set successfully. |
| **ExtraDurationSet** | Extra duration is set successfully. |
| **DurationNotSettable** | Failed because this duration is not settable. |

## Errors

### UnsupportedTimepoint

An unsupported stage was sent.

### WrongOrder

Setting the timepoint in a wrong order.



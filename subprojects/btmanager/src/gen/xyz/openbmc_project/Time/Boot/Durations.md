# xyz.openbmc_project.Time.Boot.Durations

Duration definition of each stage during host reboot process.


## Methods
No methods.

## Properties
| name | type | description |
|------|------|-------------|
| **OSUserspaceShutdown** | uint64 | Host OS is shutdowning all the systemd service and all the processes that are owned by userspace. |
| **OSKernelShutdown** | uint64 | Host OS kernel is shutdowning all the processes and releasing all the resources that are owned by the kernel. |
| **BMCShutdown** | uint64 | Some components’ fw needs to be updated by BMC after itself power off such as BIOS. And their update time will be included in the stage as well. |
| **BMC** | uint64 | From BMC power on till BMC releases the host reset, including the BIOS verification time. |
| **BIOS** | uint64 | Host BIOS initialization period. |
| **NerfKernel** | uint64 | Host Nerf kernel initialization period. |
| **NerfUserspace** | uint64 | Host Nerf userspace initialization period. |
| **OSKernel** | uint64 | Host OS kernel initialization period. |
| **OSUserspace** | uint64 | Host OS userspace initialization period. |
| **Unmeasured** | uint64 | It doesn’t show on the diagram because It’s not a specific stage during a power cycle. It’s the sum of all the time frames that cannot be measured. |
| **Extra** | array[struct[string,uint64]] | Extra durations comes from the host. |

## Signals
No signals.

## Enumerations
No enumerations.


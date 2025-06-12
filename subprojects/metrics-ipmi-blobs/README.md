# metrics-ipmi-blobs

IPMI BLOBs handler to export BMC metrics snapshot

This BLOB handler registers one blob with the name "/metric/snapshot".

The contents of the BLOB is a protocol buffer containing an instantaneous
snapshot of the BMC's health metrics, which includes the following categories:

1. BMC memory metric: mem_available, slab, kernel_stack
2. Uptime: uptime in wall clock time, idle process across all cores
3. Disk space: free space in RWFS in KiB
4. Status of the top 10 processes: cmdline, utime, stime
5. File descriptor of top 10 processes: cmdline, file descriptor count

The size of the metrics are usually around 1KB to 1.5KB.

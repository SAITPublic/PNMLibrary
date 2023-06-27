#!/bin/sh

set -e

auto_online_blocks=$(cat /sys/devices/system/memory/auto_online_blocks)

if [ "$auto_online_blocks" = "online" ]; then
  cat << EOF
  Before running the test memory auto onlining must be disabled.
  Run following commands in shell and try again:

  $ sudo bash
  $ echo 'offline' > /sys/devices/system/memory/auto_online_blocks
  $ Ctrl-D
EOF

return 1
fi

# Configure AXDIMM as system RAM
sudo daxctl reconfigure-device dax0.0 --mode=system-ram

# Compile test app
g++ -std=c++11 test.cpp -lnuma -lfmt -o test_migration.x

# Run test app
./test_migration.x

# Sanity check for migration to be completed
cat /sys/devices/system/node/node1/meminfo | grep MemUsed

# Offline AXDIMM memory before reconfiguring
sudo daxctl offline-memory dax0.0

# Configure AXDIMM as devdax (accel mode)
sudo daxctl reconfigure-device dax0.0 --mode=devdax

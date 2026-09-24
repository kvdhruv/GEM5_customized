import argparse
from gem5.components.boards.x86_board import X86Board
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import PrivateL1SharedL2CacheHierarchy
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.simulate.exit_event import ExitEvent
from gem5.resources.resource import obtain_resource
from gem5.resources.resource import DiskImageResource, FileResource
from gem5.resources.resource import FileResource, CustomDiskImageResource

parser = argparse.ArgumentParser()
parser.add_argument("--benchmark", type=str, required=True)
# Dynamic L1 Data Cache Size
parser.add_argument("--l1d_size", type=str, required=True) 
args = parser.parse_args()

# Hardware Setup: 16 Cores, Dynamic L1D
cache_hierarchy = PrivateL1SharedL2CacheHierarchy(
    l1d_size=args.l1d_size, l1d_assoc=4,
    l1i_size="32KiB", l1i_assoc=4,
    l2_size="4MiB", l2_assoc=8,
)
memory = SingleChannelDDR4_2400(size="3GiB")
# Locked to 16 cores
processor = SimpleProcessor(cpu_type=CPUTypes.TIMING, num_cores=16, isa=ISA.X86)

board = X86Board(clk_freq="3GHz", processor=processor, memory=memory, cache_hierarchy=cache_hierarchy)

# Using simlarge to properly stress the L1D caches
command = f"cd /home/gem5/parsec-benchmark\nsource env.sh\nparsecmgmt -a run -p {args.benchmark} -c gcc-hooks -i simsmall -n 16\nm5 exit\n"


board.set_kernel_disk_workload(
    kernel=FileResource("/gem5_fs/full_system_images/binaries/vmlinux-5.4.49"), 
    disk_image=CustomDiskImageResource(
        local_path="/gem5_fs/full_system_images/disks/parsec.img",
        root_partition="1"
    ),
    readfile_contents=command
)

#Define the Simulator object
simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.WORKBEGIN : (lambda: simulator.reset_stats()),
        ExitEvent.WORKEND : (lambda: [simulator.dump_stats(), simulator.exit_simulator()]),
        ExitEvent.EXIT : (lambda: [simulator.dump_stats(), simulator.exit_simulator()])
    }
)


simulator.run()

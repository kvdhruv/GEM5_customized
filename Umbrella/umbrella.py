#!/usr/bin/env python3

import argparse
from pathlib import Path

from parsers.stats import StatsParser
from parsers.config import ConfigParser

from generators.mcpat import McPATGenerator

from utils.output import OutputManager


def banner():

    print("=" * 60)
    print("Umbrella v0.1")
    print("=" * 60)


def print_summary(config):

    caches = config.count_caches()

    print("\nHardware Summary")
    print("=" * 60)

    print(f"CPU Type            : {config.cpu_type()}")
    print(f"Number of CPUs      : {len(config.cpus())}")

    print()

    print(f"Ruby Enabled        : {config.ruby_enabled()}")

    print()

    print(f"L1I Caches          : {caches['L1I']}")
    print(f"L1D Caches          : {caches['L1D']}")
    print(f"L2 Caches           : {caches['L2']}")
    print(f"L3 Caches           : {caches['L3']}")

    print()

    print(
        f"Memory Controllers  : {len(config.memory_controllers())}"
    )

    print(
        f"Clock Domains       : {config.clock_domains()}"
    )

    print(
        f"Voltage Domains     : {config.voltage_domains()}"
    )

    print("=" * 60)


def main():

    banner()

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--input",
        required=True,
    )

    parser.add_argument(
        "--tool",
        required=True,
        choices=["mcpat", "dsent", "orion"],
    )

    args = parser.parse_args()

    input_dir = Path(args.input)

    print("\nReading gem5 outputs...")

    stats = StatsParser(
        input_dir / "stats.txt"
    ).parse()

    config = ConfigParser(
        input_dir / "config.json"
    ).parse()

    print(f"Loaded {len(stats)} statistics.")

    print_summary(config)

    print("\nCPU Information")
    print("-" * 60)

    for cpu in config.cpu_info():
        print(cpu)

    print("\nCache Information")
    print("-" * 60)

    for cache in config.cache_info():
        print(cache)

    print("\nMemory Controllers")
    print("-" * 60)

    for mem in config.memory_controller_info():
        print(mem)

    output_dir = OutputManager().create_run()

    print(f"\nOutput Directory : {output_dir}")

    if args.tool == "mcpat":

        generator = McPATGenerator(
            stats,
            config,
            input_dir,
            output_dir,
        )

        generator.generate()

        from runners.mcpat import McPATRunner

        runner = McPATRunner(output_dir)
        runner.run()

    else:

        print(f"{args.tool} support coming soon.")


if __name__ == "__main__":
    main()
import json
from pathlib import Path


class ConfigParser:

    CPU_TYPES = {
        "TimingSimpleCPU",
        "AtomicSimpleCPU",
        "MinorCPU",
        "DerivO3CPU",
        "BaseO3CPU",
        "X86O3CPU",
        "ArmO3CPU",
        "RiscvO3CPU",
        "GarnetSyntheticTraffic",
    }

    def __init__(self, filename):
        self.filename = Path(filename)
        self.data = None

    def parse(self):

        if not self.filename.exists():
            raise FileNotFoundError(self.filename)

        with open(self.filename, "r") as f:
            self.data = json.load(f)

        return self

    def raw(self):
        return self.data

    def _walk(self, obj):

        if isinstance(obj, dict):

            yield obj

            for value in obj.values():
                yield from self._walk(value)

        elif isinstance(obj, list):

            for item in obj:
                yield from self._walk(item)

    # -----------------------------------------------------
    # CPU
    # -----------------------------------------------------

    def cpus(self):

        cpus = []

        for obj in self._walk(self.data):

            if obj.get("type") in self.CPU_TYPES:
                cpus.append(obj)

        return cpus

    def cpu_type(self):

        cpus = self.cpus()

        if len(cpus) == 0:
            return "Unknown"

        return cpus[0]["type"]

    def cpu_info(self):

        info = []

        for cpu in self.cpus():

            info.append({

                "name": cpu.get("path", ""),

                "type": cpu.get("type", ""),

                "clock_domain": cpu.get("clk_domain", ""),

            })

        return info

    # -----------------------------------------------------
    # Ruby
    # -----------------------------------------------------

    def ruby_enabled(self):

        return "ruby" in self.data.get("system", {})

    # -----------------------------------------------------
    # Memory Controllers
    # -----------------------------------------------------

    def memory_controllers(self):

        controllers = []

        for obj in self._walk(self.data):

            obj_type = obj.get("type", "")

            if "MemCtrl" in obj_type or "HBMCtrl" in obj_type:

                controllers.append(obj)

        return controllers

    def memory_controller_info(self):

        info = []

        for ctrl in self.memory_controllers():

            info.append({

                "name": ctrl.get("path", ""),

                "type": ctrl.get("type", ""),

            })

        return info

    # -----------------------------------------------------
    # Cache
    # -----------------------------------------------------

    def cache_info(self):

        caches = []

        for obj in self._walk(self.data):

            path = obj.get("path", "")

            if "L1Icache" in path:

                level = "L1I"

            elif "L1Dcache" in path:

                level = "L1D"

            elif "L2cache" in path:

                level = "L2"

            elif "L3cache" in path:

                level = "L3"

            else:
                continue

            caches.append({

                "name": path,

                "level": level,

                "size": obj.get("size"),

                "assoc": obj.get("assoc"),

                "line_size": obj.get("cache_line_size"),

                "replacement_policy": obj.get("replacement_policy"),

            })

        return caches

    def count_caches(self):

        counts = {

            "L1I": 0,

            "L1D": 0,

            "L2": 0,

            "L3": 0,

        }

        for cache in self.cache_info():

            counts[cache["level"]] += 1

        return counts

    # -----------------------------------------------------
    # Clock Domains
    # -----------------------------------------------------

    def clock_domains(self):

        domains = []

        for obj in self._walk(self.data):

            if obj.get("type") == "SrcClockDomain":

                domains.append({

                    "name": obj.get("path", ""),

                    "clock": obj.get("clock"),

                })

        return domains

    # -----------------------------------------------------
    # Voltage Domains
    # -----------------------------------------------------

    def voltage_domains(self):

        domains = []

        for obj in self._walk(self.data):

            if obj.get("type") == "VoltageDomain":

                domains.append({

                    "name": obj.get("path", ""),

                    "voltage": obj.get("voltage"),

                })

        return domains
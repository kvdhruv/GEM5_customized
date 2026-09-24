from pathlib import Path
import xml.etree.ElementTree as ET


class McPATGenerator:

    def __init__(self, stats, config, input_dir, output_dir):
        self.stats = stats
        self.config = config
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)

    def _set_param(self, system, name, value):
        for param in system.findall("param"):
            if param.attrib.get("name") == name:
                param.set("value", str(value))
                return

        # if missing, create it
        ET.SubElement(system, "param", {"name": name, "value": str(value)})

    def _set_stat(self, system, name, value):
        for stat in system.findall("stat"):
            if stat.attrib.get("name") == name:
                stat.set("value", str(value))
                return

        ET.SubElement(system, "stat", {"name": name, "value": str(value)})

    def generate(self):

        print("\nGenerating McPAT XML...")

        template = Path("templates/Xeon.xml")
        tree = ET.parse(template)
        root = tree.getroot()

        system = root.find("./component")

        caches = self.config.count_caches()

        # REQUIRED McPAT FIELDS
        self._set_param(system, "number_of_cores", len(self.config.cpus()))
        self._set_param(system, "number_of_L2s", caches.get("L2", 1))
        self._set_param(system, "number_of_L3s", caches.get("L3", 0))
        self._set_param(system, "number_of_NoCs", 1)

        self._set_param(system, "homogeneous_cores", 1)
        self._set_param(system, "homogeneous_L2s", 1)
        self._set_param(system, "homogeneous_L3s", 1)

        self._set_param(system, "target_core_clockrate", 2000)

        # 🔥 THIS WAS YOUR ERROR (mandatory)
        self._set_param(system, "core_tech_node", 45)

        self._set_param(system, "device_type", 0)
        self._set_param(system, "temperature", 300)

        # stats
        total_cycles = int(float(self.stats.get("simTicks", 1000000)))

        self._set_stat(system, "total_cycles", total_cycles)
        self._set_stat(system, "busy_cycles", total_cycles)
        self._set_stat(system, "idle_cycles", 0)

        output = self.output_dir / "mcpat.xml"

        ET.indent(tree, space="    ")
        tree.write(output, encoding="utf-8", xml_declaration=True)

        print(f"Generated: {output}")
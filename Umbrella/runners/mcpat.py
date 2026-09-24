from pathlib import Path
import subprocess


class McPATRunner:

    def __init__(self, output_dir):

        self.output_dir = Path(output_dir)

    def run(self):

        xml = self.output_dir / "mcpat.xml"

        report = self.output_dir / "mcpat_report.txt"

        exe = "/gem5/mcpat/mcpat"

        with open(report, "w") as f:

            subprocess.run(
                [
                    exe,
                    "-infile",
                    str(xml)
                ],
                stdout=f,
                stderr=subprocess.STDOUT
            )

        print(f"McPAT report saved to {report}")
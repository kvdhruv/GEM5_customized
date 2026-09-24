from pathlib import Path


class StatsParser:

    def __init__(self, filename):

        self.filename = Path(filename)

    def parse(self):

        if not self.filename.exists():
            raise FileNotFoundError(self.filename)

        stats = {}

        with self.filename.open("r", encoding="utf-8") as file:

            for line in file:

                line = line.strip()

                if (
                    not line
                    or line.startswith("#")
                    or line.startswith("-")
                ):
                    continue

                parts = line.split()

                if len(parts) < 2:
                    continue

                key = parts[0]
                value = parts[1]

                try:
                    value = int(value)
                except ValueError:
                    try:
                        value = float(value)
                    except ValueError:
                        pass

                stats[key] = value

        return stats
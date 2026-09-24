from pathlib import Path


class OutputManager:

    def __init__(self):

        self.base = Path("outputs")

        self.base.mkdir(exist_ok=True)

    def create_run(self):

        runs = sorted(self.base.glob("run_*"))

        if not runs:
            number = 1
        else:
            number = int(
                runs[-1].name.split("_")[1]
            ) + 1

        run_dir = self.base / f"run_{number:03d}"

        run_dir.mkdir()

        (run_dir / "plots").mkdir()

        return run_dir
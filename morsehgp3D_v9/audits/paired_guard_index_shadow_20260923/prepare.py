#!/usr/bin/env python3
"""Pin both published S2 samples as a small, independently replayable table."""

import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "paired_guards_precore_20260923"


def main():
    lines = ["seed a b F mask q3_4 q4_4 closed_4 q3_8 q4_8 closed_8 q3_16 q4_16 closed_16"]
    for name in ("RESULT.json", "RESULT_SEED2.json"):
        data = json.loads((OLD / name).read_text())
        seed = data["sample_seed"]
        assert len(data["rows"]) == 60
        for row in data["rows"]:
            vals = [seed, row["a"], row["b"], row["F"], row["mask"]]
            for budget in (4, 8, 16):
                result = row["results"][str(budget)]
                vals.extend((result["q3_pairs"], result["q4_pairs"], int(result["closed"])))
            lines.append(" ".join(map(str, vals)))
    content = "\n".join(lines) + "\n"
    path = HERE / "SAMPLES.tsv"
    if path.exists():
        assert path.read_text() == content, "pinned sample changed"
    else:
        path.write_text(content)
    print("pinned samples", len(lines) - 1)


if __name__ == "__main__":
    main()

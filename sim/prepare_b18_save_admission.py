"""Inject the live Amstrad.sv save_admit expression into its focused bench."""

from pathlib import Path
import re


root = Path(__file__).resolve().parents[1]
production = (root / "Amstrad.sv").read_text()
matches = re.findall(
    r"(?m)^[ \t]*wire[ \t]+save_admit[ \t]*=[ \t]*(.*?);",
    production,
    re.S,
)
if len(matches) != 1:
    raise SystemExit(
        f"Expected one production save_admit assignment in Amstrad.sv; found {len(matches)}"
    )

template = Path(__file__).with_name("b18_save_admission_test.sv").read_text()
placeholder = "wire save_admit = 1'b0;"
if template.count(placeholder) != 1:
    raise SystemExit("Save admission test seam changed; expected one placeholder")

generated = template.replace(placeholder, f"wire save_admit = {matches[0].strip()};")
output = root / "sim/obj_dir/b18_save_admission/b18_save_admission_test.sv"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(generated)

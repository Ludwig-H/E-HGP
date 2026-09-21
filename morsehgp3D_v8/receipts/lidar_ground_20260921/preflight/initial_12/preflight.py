import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

base = Path(tempfile.mkdtemp(prefix="mhgp8-ground-preflight."))
bins = (
    Path("/workspaces/E-HGP/build/v8_ground_patchwork_20260921/mhgp8_patchwork_ground_probe"),
    Path("/workspaces/E-HGP/build/v8_ground_patchwork_sanitize_20260921/mhgp8_patchwork_ground_probe"),
)
def pack(points):
    return b"".join(struct.pack("<4f", *p) for p in points)
cases = {
    "empty": (b"", [], 0, b""),
    "three": (pack([(4,0,-1.7,float("nan")),(0,0,-1.7,1),(100,0,-1.7,1)]), [], 0, bytes([2,0,0])),
    "outside": (pack([(0,0,0,0),(100,0,0,0),(4,0,2**50,0)]) + struct.pack("<4I",0x40800000,0,0x00800000,0), [], 0, bytes(4)),
    "invalid": (pack([(float("nan"),0,0,0)]), [], 1, None),
    "tiny_seed": (pack([(5,0,1,0)]*12), ["--seed-distance","1e-300"], 0, None),
    "plane_object": (pack([(5+i*.25,-1+j*.125,-1.723+.05*(5+i*.25-8),1) for i in range(25) for j in range(17)] +
                           [(7+i*.1,0.2,0.5+j*.1,1) for i in range(5) for j in range(5)]), [], 0, None),
}
results = []
for name, (payload, extra, expected, expected_mask) in cases.items():
    source=base/(name+".bin");source.write_bytes(payload)
    for index, binary in enumerate(bins):
        output=base/(name+f"_{index}.u8")
        command=[str(binary),"--input",str(source),"--output",str(output),*extra]
        env=dict(os.environ,ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        completed=subprocess.run(command,capture_output=True,env=env)
        record={"case":name,"command":command,"exit_code":completed.returncode,
                "stdout":completed.stdout.decode(errors="replace"),"stderr":completed.stderr.decode(errors="replace"),
                "binary_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),
                "input_sha256":hashlib.sha256(payload).hexdigest()}
        (base/(name+f"_{index}.json")).write_text(json.dumps(record,indent=2)+"\n")
        if completed.returncode != expected:
            raise ValueError(record)
        if expected==0:
            row=json.loads(completed.stdout)
            mask=output.read_bytes()
            if expected_mask is not None and mask!=expected_mask: raise ValueError("mask differs")
            if row["raw_returns"]!=len(mask): raise ValueError("count differs")
            if name in ("empty","outside") and row["algorithm_invocations"]!=0: raise ValueError("library invoked")
            results.append(dict(case=name,build=index,counts=row["counts"],status="passed"))
        elif output.exists():
            raise ValueError("invalid input produced a mask")
print(json.dumps(dict(path=str(base),results=results),indent=2))

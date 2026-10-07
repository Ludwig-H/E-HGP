#!/usr/bin/env python3
"""Assemble le rapport L09 a partir des parties redigees et des tables regenerees."""
import subprocess, sys, os, re
W = "/tmp/v11-audit/l09_perf_lidar/work"
D = W + "/draft"
def run(script):
    return subprocess.run([sys.executable, os.path.join(W, script)], capture_output=True, text=True, check=True).stdout
def block(text, title):
    lines = text.splitlines()
    out = []; on = False
    for ln in lines:
        if ln.startswith("### "):
            on = ln.strip() == "### " + title or ln.strip().startswith("### " + title)
            continue
        if on: out.append(ln)
    while out and not out[-1].strip(): out.pop()
    while out and not out[0].strip(): out.pop(0)
    if not out: raise SystemExit("bloc vide: " + title)
    return "\n".join(out)
tl = run("tables_local.py"); tl2 = run("tables_local2.py")
sub = {
    "@@L1@@": block(tl, "L1"), "@@L2@@": block(tl, "L2"), "@@L3@@": block(tl, "L3"), "@@L4@@": block(tl, "L4"),
    "@@S1@@": block(tl2, "S1"), "@@S2@@": block(tl2, "S2"), "@@S3@@": block(tl2, "S3"), "@@S4@@": block(tl2, "S4"),
    "@@TABLE_S5@@": block(tl2, "S5"), "@@TABLE_S6@@": block(tl2, "S6"),
}
extra = {}
ep = os.path.join(D, "placeholders.txt")
if os.path.exists(ep):
    cur = None
    for ln in open(ep):
        m = re.match(r"^(@@[A-Z0-9_]+@@)\s*=\s*(.*)$", ln.rstrip("\n"))
        if m: extra[m.group(1)] = m.group(2)
sub.update(extra)
parts = ["part0.md", "part1.md", "part2.md", "part3.md", "part4.md", "part5.md", "part6.md", "part7.md", "part8.md", "part9.md", "part10.md"]
text = "\n\n".join(open(os.path.join(D, p)).read().rstrip("\n") for p in parts) + "\n"
for k, v in sub.items(): text = text.replace(k, v)
left = sorted(set(re.findall(r"@@[A-Z0-9_]+@@", text)))
if left: print("PLACEHOLDERS RESTANTS:", left, file=sys.stderr)
open(sys.argv[1], "w").write(text)
print("ecrit", sys.argv[1], len(text.splitlines()), "lignes", len(text), "octets")

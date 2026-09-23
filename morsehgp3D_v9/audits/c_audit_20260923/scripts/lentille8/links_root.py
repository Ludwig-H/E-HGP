#!/usr/bin/env python3
"""Verifie les liens Markdown relatifs des notes v9/audits dans un arbre Git donne (lecture seule)."""
import re, subprocess, sys, posixpath, unicodedata
from urllib.parse import unquote
REPO = "/workspaces/E-HGP/build/v9-audit-c-worktree"
rev = sys.argv[1]
files = subprocess.run(["git","-C",REPO,"ls-tree","-r","--name-only",rev],capture_output=True,text=True,check=True).stdout.split("\n")
fileset = set(f for f in files if f)
dirs = set()
for f in fileset:
    p = f
    while "/" in p:
        p = p.rsplit("/",1)[0]; dirs.add(p)
LINK = re.compile(r"!?\[[^\]]*\]\(([^)\s]+)")
def show(path):
    return subprocess.run(["git","-C",REPO,"show",f"{rev}:{path}"],capture_output=True,text=True,check=True).stdout
def slug(h):
    h = h.strip().lower()
    h = re.sub(r"[`*_~]", "", h)
    out = []
    for ch in h:
        if ch.isalnum() or ch in "-_ ":
            out.append(ch)
    s = "".join(out).replace(" ", "-")
    return s
targets = [f for f in fileset if (f.startswith("audits/morsehgp3D_v9/") and f.endswith(".md"))]
bad = []; total = 0; anchors_bad = []; per_file = {}
for src in sorted(targets):
    text = show(src)
    infence = False
    n_links = 0
    for no, line in enumerate(text.splitlines(), 1):
        if line.strip().startswith("```"):
            infence = not infence; continue
        if infence: continue
        for m in LINK.finditer(line):
            raw = m.group(1)
            t = unquote(raw)
            if t.startswith(("http://","https://","mailto:")): continue
            path, _, frag = t.partition("#")
            total += 1; n_links += 1
            if path == "":
                tgt = src
            else:
                tgt = posixpath.normpath(posixpath.join(posixpath.dirname(src), path.split("?")[0]))
            if tgt not in fileset and tgt not in dirs:
                bad.append((src, no, raw, tgt))
                continue
            if frag and tgt in fileset and tgt.endswith(".md"):
                heads = [slug(l.lstrip("#")) for l in show(tgt).splitlines() if l.startswith("#")]
                if frag.lower() not in heads:
                    anchors_bad.append((src, no, raw))
    per_file[src] = n_links
print(f"rev={rev} notes={len(targets)} liens_relatifs={total} morts={len(bad)} ancres_mortes={len(anchors_bad)}")
for b in bad: print("MORT", b)
for a in anchors_bad: print("ANCRE", a)
zero = [f for f,n in per_file.items() if n==0]
print("notes_sans_lien_relatif:", len(zero))
for z in zero: print("  ", z)

import subprocess, re, sys
REPO="/workspaces/E-HGP/build/v9-audit-c-worktree"
rev=sys.argv[1] if len(sys.argv)>1 else "origin/main"
def git(*a): return subprocess.run(["git","-C",REPO,*a],capture_output=True,text=True).stdout
chan=git("show",f"{rev}:audits/COORDINATION_MORSEHGP3D_V9.md")
# split channel into entries
entries=[]; cur=None
for l in chan.splitlines():
    if l.startswith("## "):
        cur=[l,[]]; entries.append(cur)
    elif cur: cur[1].append(l)
def role(h):
    m=re.search(r"\((développeur[^)]*|auditeur [ABC])\)",h)
    if m: return "DEV" if m.group(1).startswith("dév") else m.group(1)[-1]
    if "auditeur B" in h: return "B"
    return "?"
dev_text="\n".join("\n".join(b) for h,b in entries if role(h)=="DEV")
all_text=chan
files=[f for f in git("ls-tree","-r","--name-only",rev,"morsehgp3D_v9/audits").split("\n") if f]
for f in files:
    name=f.split("/")[-1]
    stem=re.sub(r"\.(md|py|cpp|txt|json|patch)$","",name)
    hs=[h[:8] for h in git("log","--format=%h",rev,"--",f).split()]
    in_dev_name = name in dev_text or stem in dev_text
    in_dev_hash=[h for h in hs if h[:8] in dev_text]
    in_chan_name = name in all_text
    in_chan_hash=[h for h in hs if h[:8] in all_text]
    print(f"{name:58} dev_name={int(in_dev_name)} dev_hash={','.join(in_dev_hash) or '-'} chan_name={int(in_chan_name)} chan_hash={','.join(in_chan_hash) or '-'}")
print("ROLES:", [(role(h),h[3:60]) for h,b in entries])

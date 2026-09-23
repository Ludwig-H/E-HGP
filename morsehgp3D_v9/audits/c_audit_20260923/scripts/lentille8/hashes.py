import subprocess, re, sys
REPO="/workspaces/E-HGP/build/v9-audit-c-worktree"
rev=sys.argv[1]
def git(*a, check=False):
    return subprocess.run(["git","-C",REPO,*a],capture_output=True,text=True)
main_commits=set(git("rev-list",rev).stdout.split())
files=[f for f in git("ls-tree","-r","--name-only",rev,"morsehgp3D_v9/audits").stdout.split() if f.endswith(".md")]+["audits/COORDINATION_MORSEHGP3D_V9.md"]
HX=re.compile(r"`([0-9a-f]{7,40})`|\(`?([0-9a-f]{8})`?[,)]|\b([0-9a-f]{8})\b")
cache={}
def classify(h):
    if h in cache: return cache[h]
    r=git("rev-parse","--verify","--quiet",h+"^{commit}")
    if r.returncode!=0:
        cache[h]="absent"; return "absent"
    full=r.stdout.strip()
    cache[h]="main" if full in main_commits else "hors_main"; return cache[h]
rows=[]
allbad=[]
for f in files:
    txt=git("show",f"{rev}:{f}").stdout
    hs=set()
    for m in re.finditer(r"`([0-9a-f]{7,12})`", txt): hs.add(m.group(1))
    for m in re.finditer(r"(?<![0-9a-zA-Z…/])([0-9a-f]{8})(?![0-9a-zA-Z…])", txt):
        tok=m.group(1)
        if re.search(r"[a-f]",tok) and re.search(r"[0-9]",tok): hs.add(tok)
    cls={h:classify(h) for h in hs}
    commits=[h for h,c in cls.items() if c!="absent"]
    hors=[h for h,c in cls.items() if c=="hors_main"]
    rows.append((f.split("/")[-1],len(commits),hors))
    for h in hors: allbad.append((f,h))
for n,c,h in rows: print(f"{n:58} commits_cites={c:3} hors_main={','.join(h) or '-'}")

import subprocess, re, collections
REPO="/workspaces/E-HGP/build/v9-audit-c-worktree"
out=subprocess.run(["git","-C",REPO,"log","--reverse","--format=@@%h %s","--name-status","origin/main","--","morsehgp3D_v9/audits","audits/COORDINATION_MORSEHGP3D_V9.md"],capture_output=True,text=True).stdout
commits=[]; cur=None
for l in out.splitlines():
    if l.startswith("@@"):
        h,s=l[2:].split(" ",1); cur=[h,s,[]]; commits.append(cur)
    elif l.strip():
        st,*p=l.split("\t"); cur[2].append((st,p[-1].replace("morsehgp3D_v9/audits/","")))
A_SEED=re.compile(r"^(AUDIT_A_|CONTRE_AUDIT_A_|Q3_STRUCTURE|Q4_STRUCTURE|CONTRAT_COUTS)")
B_SEED=re.compile(r"(^CONTRE_AUDIT_B_|^PISTE_B_|_B_2026|^CONTRELECTURE_B)")
lab={}
for h,s,fs in commits:
    names=[n for st,n in fs]
    a=any(A_SEED.search(n) for n in names); b=any(B_SEED.search(n) for n in names)
    if s.startswith(("v9","receipts","gcp","coordination","open","lidar")): lab[h]="DEV"
    elif a and b: lab[h]="A+B?"
    elif a: lab[h]="A"
    elif b: lab[h]="B"
    else: lab[h]="?"
for h,s,fs in commits:
    if lab[h] in ("?","A+B?"):
        print(lab[h],h,s,"|",[n for st,n in fs])
created={}
for h,s,fs in commits:
    for st,n in fs:
        if st.startswith("A") and n not in created: created[n]=(h,lab[h])
print()
for n,(h,l) in sorted(created.items()): print(f"{l:5} {h} {n}")

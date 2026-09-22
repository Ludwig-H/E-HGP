#!/usr/bin/env python3
"""Inventaire épinglé de l'audit d'ouverture v9 : état publié de la v8 (et de la v7) et tranche non commise.

Lecture seule sur le dépôt. Écrit un seul JSON (argument --output). Aucun assert : tient sous python3 -O.
"""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

ROOT = Path("/workspaces/E-HGP")


def git(*a, cwd=ROOT):
    return subprocess.check_output(["git", *a], cwd=cwd, text=True).strip()


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tree_stats(root, version):
    base = root / version
    out = {}
    for sub in sorted(p for p in base.iterdir() if p.is_dir()):
        files = [f for f in sub.rglob("*") if f.is_file()]
        out[sub.name] = dict(files=len(files), bytes=sum(f.stat().st_size for f in files))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--head-worktree", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    head = args.head_worktree
    commit = git("rev-parse", "HEAD", cwd=head)
    inv = dict(schema="mhgp9_audit_v8_inventory_v1", phase="exploration_v9_hors_registre", public_status="not_claimed", gcp_used=False,
               published_commit=commit, published_commit_date=git("show", "-s", "--format=%cI", commit, cwd=head))
    for version in ("morsehgp3D_v7", "morsehgp3D_v8"):
        tracked = git("ls-files", version, cwd=head).splitlines()
        inv[version] = dict(tracked_files=len(tracked), commits=int(git("rev-list", "--count", commit, "--", version, cwd=head)),
                            first_commit=git("log", "--reverse", "--format=%h %cI %s", commit, "--", version, cwd=head).splitlines()[0],
                            last_commit=git("log", "-1", "--format=%h %cI %s", commit, "--", version, cwd=head),
                            directories=tree_stats(head, version))
    v8 = head / "morsehgp3D_v8"
    inv["v8_docs_sha256"] = {str(p.relative_to(head)): sha(p) for p in sorted((v8 / "docs").glob("*.md"))}
    inv["v8_entry_sha256"] = {str(p.relative_to(head)): sha(p) for p in (v8 / "README.md", v8 / "PASSATION.md", v8 / "audits" / "ETAT_COURANT.md",
                                                                      head / "AGENTS.md", head / "audits" / "COORDINATION_MORSEHGP3D_V8.md")}
    receipts = {}
    for d in sorted(p for p in (v8 / "receipts").iterdir() if p.is_dir()):
        readme = d / "README.md"
        receipts[d.name] = dict(files=sum(1 for f in d.rglob("*") if f.is_file()), bytes=sum(f.stat().st_size for f in d.rglob("*") if f.is_file()),
                                readme_sha256=sha(readme) if readme.is_file() else None)
    inv["v8_receipts"] = receipts
    # Tranche indexée mais non commise dans le worktree partagé (autre acteur) : pins de contenu, jamais un statut.
    staged = git("diff", "--cached", "--name-status").splitlines()
    pending = []
    for line in staged:
        status, _, path = line.partition("\t")
        p = ROOT / path
        pending.append(dict(status=status, path=path, worktree_sha256=sha(p) if p.is_file() else None,
                            index_blob=git("rev-parse", f":{path}") if status != "D" else None))
    inv["shared_worktree_pending_index"] = dict(head=git("rev-parse", "HEAD"), entries=len(pending), files=pending,
                                                note="indexé par un autre acteur (reprise u18 et atlas saturant), non commis, non qualifié par cet audit")
    untracked = [l[3:] for l in git("status", "--porcelain", "--untracked-files=normal", "--", "morsehgp3D_v8").splitlines() if l.startswith("??")]
    inv["shared_worktree_untracked_v8"] = untracked
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(inv, indent=1, sort_keys=True, ensure_ascii=False) + "\n")
    print(json.dumps(dict(commit=commit[:8], v8_tracked=inv["morsehgp3D_v8"]["tracked_files"], receipts=len(receipts), pending=len(pending), untracked=len(untracked))))
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Contrôle léger et reproductible du déplacement, sans moteur ni données réelles."""
import collections
import hashlib
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PIN = "3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff"
LINK = re.compile(r"\[[^\]]*\]\(([^)]+)\)")


def need(value, message):
    if not value:
        raise ValueError(message)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def original(path):
    return git("show", f"{PIN}:{path}")


def exists(path):
    if path.exists():
        return True, "worktree"
    try:
        relative = path.relative_to(ROOT).as_posix()
        git("cat-file", "-e", f"{PIN}:{relative}")
        return True, "tracked_at_pin_sparse_checkout"
    except (ValueError, subprocess.CalledProcessError):
        return False, "missing"


def links(files):
    records = []
    for path in files:
        for match in LINK.finditer(path.read_text()):
            target = match.group(1)
            if target.startswith(("#", "https://", "http://", "mailto:", "app://")):
                continue
            target_path = unquote(target.split("#", 1)[0])
            found, mechanism = exists((path.parent / target_path).resolve())
            records.append(dict(source=path.relative_to(ROOT).as_posix(), target=target,
                                found=found, resolution=mechanism))
    return records


def rows(text):
    return {line.split("|")[1].strip(" `"): line for line in text.splitlines()
            if line.startswith("| `CST-")}


def main():
    mapping = json.loads((HERE / "archive_map.json").read_text())
    archived = []
    for entry in mapping["entries"]:
        before = original(entry["source_path"])
        after = (ROOT / entry["archive_path"]).read_bytes()
        need(hashlib.sha256(before).hexdigest() == entry["source_sha256"], "source hash changed")
        need(hashlib.sha256(after).hexdigest() == entry["archive_sha256"], "archive hash changed")
        restored = after.decode()
        for change in entry["link_rewrites"]:
            restored = restored.replace(
                "](" + change["archived_target"] + ")", "](" + change["original_target"] + ")")
            git("cat-file", "-e", f"{change['pin']}:{change['source_target']}")
        need(restored.encode() == before, "archived body changed beyond link targets")
        need(not (ROOT / entry["source_path"]).exists(), "old active note remains")
        archived.append(dict(source=entry["source_path"], archive=entry["archive_path"],
                             source_text_preserved_except_links=True,
                             pinned_link_targets=len(entry["link_rewrites"])))

    channel = ROOT / "morsehgp3D_v12/audits"
    files = sorted(channel.iterdir())
    need({p.name for p in files} == {"README.md", "CONSTATS.md", "AUDIT_CODEX_20261007.md",
                                    "AUDIT_CLAUDE_20261007.md"}, "unexpected active channel layout")
    sizes = {path.name: path.stat().st_size for path in files}
    need(all(p.is_file() and p.suffix == ".md" for p in files), "channel contains non-Markdown/directory")
    need(sum(sizes.values()) <= 64*1024, "channel over 64 KiB")
    need(sizes["README.md"] <= 4*1024, "README over 4 KiB")
    need(all(size <= 8*1024 for name,size in sizes.items() if name.startswith("AUDIT_")), "note over 8 KiB")
    before_names = git("ls-tree", "-r", "--name-only", PIN, "morsehgp3D_v12/audits").decode().splitlines()
    before_sizes = {Path(path).name: len(original(path)) for path in before_names}
    registry = "morsehgp3D_v12/audits/CONSTATS.md"
    before_rows = rows(original(registry).decode())
    after_rows = rows((ROOT / registry).read_text())
    need(set(before_rows) <= set(after_rows), "historical finding identifier lost")
    codex = "morsehgp3D_v12/audits/AUDIT_CODEX_20261007.md"
    codex_unchanged = original(codex) == (ROOT / codex).read_bytes()

    active_files = files + [ROOT / "morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md",
                            ROOT / "morsehgp3D_v12/docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md"]
    checked_links = links(active_files)
    broken = [link for link in checked_links if not link["found"]]
    need(not broken, "broken active Markdown links")
    historical_mentions = []
    historical_links = []
    names = [Path(entry["source_path"]).name for entry in mapping["entries"]]
    receipt_paths = git("ls-tree", "-r", "--name-only", PIN, "morsehgp3D_v12/receipts").decode().splitlines()
    for relative in receipt_paths:
        if not relative.endswith(".md"):
            continue
        text = original(relative).decode()
        for number, line in enumerate(text.splitlines(), 1):
            if any(name in line for name in names):
                historical_mentions.append(dict(source=relative, line=number, text=line))
        for match in LINK.finditer(text):
            if any(name in match.group(1) for name in names):
                historical_links.append(dict(source=relative, target=match.group(1)))
    changed = git("diff", "--name-status", PIN, "--", "morsehgp3D_v12").decode().splitlines()
    need(not git("diff", PIN, "--", *receipt_paths), "existing receipt modified")
    new_files = git("ls-files", "--others", "--exclude-standard", "--", "morsehgp3D_v12").decode().splitlines()
    states = collections.Counter(row.split("|")[-3].strip() for row in after_rows.values())
    result = dict(schema="ehgp.v12.audit_channel_cleanup.v1", pin=PIN,
                  before=dict(files=len(before_sizes), bytes=sum(before_sizes.values()), sizes=before_sizes),
                  after=dict(files=len(sizes), bytes=sum(sizes.values()), sizes=sizes),
                  registry=dict(rows=len(after_rows), original_rows_exactly_preserved=all(after_rows[k] == v for k, v in before_rows.items()),
                                original_identifiers_preserved=True, states=dict(states)),
                  codex_note_unchanged=codex_unchanged, archives=archived,
                  active_links_checked=len(checked_links), broken_active_links=broken,
                  sparse_targets=[link for link in checked_links if link["resolution"] != "worktree"],
                  immutable_receipt_mentions=historical_mentions,
                  immutable_receipt_markdown_links_to_moved_notes=historical_links,
                  tracked_edits=changed, new_files=new_files,
                  existing_receipts_unchanged=True,
                  product_tests_unchanged=not bool(git("diff", PIN, "--", "morsehgp3D_v12/src", "morsehgp3D_v12/reference", "morsehgp3D_v12/bench")),
                  tool_update="check_constats.py: hygiène du canal", gcp_used=False)
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()

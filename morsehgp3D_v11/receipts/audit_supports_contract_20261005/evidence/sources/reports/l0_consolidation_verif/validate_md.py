"""Applique tools/check_docs.validate aux Markdown v11 touches par L0, que check_docs ne parcourt pas, et resout les
liens absents du checkout partiel contre origin/main (git cat-file). Bibliotheque standard, aucun assert.

    python3 -S -B validate_md.py [racine du worktree]
"""
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else '/workspaces/E-HGP/build/v11-impl-l0').resolve()
sys.path.insert(0, str(ROOT / 'tools'))
import check_docs  # noqa: E402

FILES = ['morsehgp3D_v11/docs/MATHEMATIQUES.md', 'morsehgp3D_v11/docs/SORTIES.md', 'morsehgp3D_v11/docs/ARCHITECTURE.md',
         'morsehgp3D_v11/docs/PROVENANCE.md', 'morsehgp3D_v11/docs/HIERARCHIE_POINTS.md', 'morsehgp3D_v11/README.md',
         'morsehgp3D_v11/audits/REPONSE_CLAUDE_SUPPORTS_20261004.md', 'morsehgp3D_v11/audits/README.md',
         'morsehgp3D_v11/reference/README.md', 'docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md']
MISSING = re.compile(r"^(.*):(\d+): missing local link '([^']*)'$")


def on_origin(path):
    rel = os.path.relpath(str(path), str(ROOT))
    if rel.startswith('..'):
        return False
    return subprocess.run(['git', '-C', str(ROOT), 'cat-file', '-e', 'origin/main:' + rel],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0


remarks = 0
for name in FILES:
    path = ROOT / name
    kept = []
    for error in check_docs.validate(path):
        found = MISSING.match(error)
        if found:
            target = check_docs.local_target(path, found.group(3))
            if target is not None and on_origin(target):
                continue
        kept.append(error)
    remarks += len(kept)
    print('%s : %d remarque(s)' % (name, len(kept)))
    for error in kept:
        print('  ' + error)
print('validate_md %s remarques=%d' % ('ok' if remarks == 0 else 'ECHEC', remarks))
sys.exit(1 if remarks else 0)

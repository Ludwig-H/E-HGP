"""Refuse une apostrophe dans un bloc SSH entre guillemets simples.

Une session GCP passe ses commandes distantes dans une chaine `'...'`. Toute
apostrophe interne la FERME, et gcloud recoit alors les mots suivants comme des
arguments : la session meurt en `rc=2` apres avoir demarre la VM. C'est arrive
une fois, et le trap a bien certifie `TERMINATED` — mais la session etait
perdue. Ce controle rend la faute impossible.
"""
import pathlib
import sys

bad = []
for path in sorted(pathlib.Path("gcp-migration").glob("session_*.sh")):
    inside = False
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        if line.startswith('"${SSH[@]}" \''):
            inside = True
            continue
        if inside and line.startswith("' 2>&1"):
            inside = False
            continue
        if inside and "'" in line:
            bad.append(f"{path}:{lineno}: apostrophe dans un bloc SSH — {line.strip()[:60]}")

if bad:
    print("\n".join(bad), file=sys.stderr)
    sys.exit(1)
print("Blocs SSH des sessions : aucune apostrophe interne.")

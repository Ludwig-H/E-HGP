"""Fabrique un recu immuable : pin du commit, hashes des sources, commandes.

Un recu de ce depot n'est pas un resume : c'est ce qui permet a un auditeur de
rejouer exactement. Ce script rassemble donc, dans un dossier nouveau :

* le commit de reference (`git rev-parse HEAD`) et l'etat de l'arbre ;
* la version de Python, de numpy et de scipy ;
* le sha256 de chaque source du chantier au moment de la mesure ;
* les commandes demandees, avec leur code de sortie et leur sortie complete.

Le dossier est NOUVEAU : on ne modifie jamais un recu publie. Si le dossier
existe deja, le script refuse (code 2).

Usage :

    python3 bench/make_receipt.py --out receipts/ouverture_20260925 \
        --run "python3 bench/births_vs_dimension.py --n 8 --k-max 4 --dims 2,3,5,10" \
        --run "python3 -m unittest discover -s tests -p 'test_*.py'"
"""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

EXIT_OK = 0
EXIT_REFUSED = 2


def sha256_of(path):
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def collect_sources(root):
    entries = {}
    for path in sorted(root.rglob("*.py")):
        if "__pycache__" in path.parts:
            continue
        entries[str(path.relative_to(root))] = sha256_of(path)
    for path in sorted(root.rglob("*.md")):
        entries[str(path.relative_to(root))] = sha256_of(path)
    return entries


def run_command(command, cwd):
    completed = subprocess.run(
        command, shell=True, cwd=str(cwd), capture_output=True, text=True, check=False
    )
    return {
        "command": command,
        "exit_code": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description="fabrique un recu immuable")
    parser.add_argument("--out", type=str, required=True)
    parser.add_argument("--run", action="append", default=[])
    parser.add_argument("--label", type=str, default="")
    options = parser.parse_args(argv)

    project = Path(__file__).resolve().parent.parent
    repository = project.parent
    requested = Path(options.out)
    destination = requested if requested.is_absolute() else project / requested
    if destination.exists():
        print("refus : le recu existe deja, un recu publie ne se modifie pas")
        return EXIT_REFUSED
    destination.mkdir(parents=True)

    def git(*arguments):
        completed = subprocess.run(
            ["git"] + list(arguments), cwd=str(repository), capture_output=True, text=True,
            check=False,
        )
        return completed.stdout.strip()

    versions = {}
    for module in ("numpy", "scipy"):
        try:
            imported = __import__(module)
            versions[module] = getattr(imported, "__version__", "inconnu")
        except ImportError:
            versions[module] = "absent"

    manifest = {
        "object": "ehgp.receipt.v1",
        "label": options.label,
        "commit": git("rev-parse", "HEAD"),
        "commit_subject": git("log", "-1", "--pretty=%s"),
        "python": sys.version.split()[0],
        "modules": versions,
        "sources": collect_sources(project / "src"),
        "benches": collect_sources(project / "bench"),
        "docs": collect_sources(project / "docs"),
        "runs": [],
    }
    for command in options.run:
        print("commande : " + command)
        record = run_command(command, project)
        print("   code " + str(record["exit_code"]))
        manifest["runs"].append(record)
        name = "run_{:02d}.txt".format(len(manifest["runs"]))
        (destination / name).write_text(
            "$ " + command + "\n\n" + record["stdout"] + "\n" + record["stderr"],
            encoding="utf-8",
        )
    (destination / "MANIFESTE.json").write_text(
        json.dumps(manifest, indent=1, sort_keys=True), encoding="utf-8"
    )
    print("recu ecrit : " + str(destination))
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())

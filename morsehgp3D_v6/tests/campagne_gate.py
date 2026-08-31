#!/usr/bin/env python3
"""Porte du lanceur de campagne locale (bench/campagne_locale.sh) — la
provenance d'executable est PROUVEE (alerte auditeur « campagne CPU
mixte », deux tours).

Scenarios :
  1. nominal (faux binaire conforme, matrice 1x1x2) : DONE terminal, un
     statut code=0 par tuple, HASHES.txt avec avant==apres par run, META
     avec sha256 du binaire prive, des sorties, du lanceur, du validateur,
     de l'agregateur et l'autorite de profil (PROFIL.txt copie) ;
  2. auto-alteration de la copie privee pendant un tuple : arret SANS DONE
     avec `INVALID hash apres ...` (detecteur de tamper prive) ;
  3. VRAIE reconstruction concurrente : la SOURCE est REMPLACEE apres la
     copie (pendant le premier tuple) — les deux tuples executent la MEME
     copie privee et la campagne finit DONE avec des hashes homogenes
     (l'isolation annoncee, testee causalement — correction du deuxieme
     tour : l'ancienne fixture n'alterait que la copie) ;
  4. copie discordante de la source au moment de la copie : REFUS avant le
     premier run (aucun STATUS ecrit) — teste via un faux sha256sum qui ment
     sur la source ;
  5. profil absent ou incomplet : refus avant le premier run.

Codes : 0 conforme ; 1 un scenario en echec.
"""

from __future__ import annotations

import os
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
LAUNCHER = HERE.parent / "bench" / "campagne_locale.sh"

FAKE_OK = """#!/usr/bin/env bash
echo "sortie factice fam=$1"
exit 0
"""

FAKE_TAMPER = """#!/usr/bin/env bash
# Simule une alteration de la copie privee elle-meme.
chmod 755 "$0" && echo "# altere" >> "$0"
echo "sortie factice"
exit 0
"""

# Le faux binaire remplace la SOURCE pendant son premier run : la campagne
# doit rester sur la copie privee et finir DONE.
FAKE_REBUILD_SOURCE = """#!/usr/bin/env bash
if [ -n "${SOURCE_A_REMPLACER:-}" ] && [ ! -e "${SOURCE_A_REMPLACER}.remplacee" ]; then
  printf '#!/bin/sh\\nexit 1\\n' > "${SOURCE_A_REMPLACER}"
  touch "${SOURCE_A_REMPLACER}.remplacee"
fi
echo "sortie factice"
exit 0
"""

PROFILE_1x1x2 = "profil=porte_lanceur\nfamilles=uniform\nn=400\ngraines=3 4\n"


def write_exec(path: Path, content: str) -> None:
    path.write_text(content)
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def run_launcher(out: Path, bin_src: Path, profile: Path, env_extra=None) -> int:
    env = dict(os.environ, CAMPAGNE_ALLOW_DIRTY="1")
    if env_extra:
        env.update(env_extra)
    r = subprocess.run(["bash", str(LAUNCHER), str(out), str(bin_src),
                        "campagne de porte", str(profile)],
                       capture_output=True, text=True, env=env)
    return r.returncode


def main() -> int:
    failures = 0

    def check(name: str, ok: bool, detail: str = "") -> None:
        nonlocal failures
        if not ok:
            failures += 1
            print(f"ECHEC porte : {name} {detail}", file=sys.stderr)

    with tempfile.TemporaryDirectory() as td:
        base = Path(td)
        profile = base / "PROFIL.txt"
        profile.write_text(PROFILE_1x1x2)

        # 1. Nominal.
        src = base / "faux_mhgp6"
        write_exec(src, FAKE_OK)
        out1 = base / "nominal"
        rc = run_launcher(out1, src, profile)
        status = (out1 / "STATUS.txt").read_text().splitlines()
        hashes = (out1 / "HASHES.txt").read_text().splitlines()
        meta = (out1 / "META.txt").read_text()
        check("nominal : rc=0 et DONE terminal", rc == 0 and status and status[-1] == "DONE",
              f"rc={rc} fin={status[-1] if status else '?'}")
        check("nominal : deux tuples code=0",
              sum(1 for line in status if line.startswith("code=0 ")) == 2)
        check("nominal : hashes avant==apres par run",
              len(hashes) == 2 and all("avant=" in h and h.split()[0][6:] == h.split()[1][6:] for h in hashes))
        for key in ("sha256_binaire_prive=", "sha256_lanceur=", "sha256_validateur=",
                    "sha256_agregateur=", "autorite_profil=PROFIL.txt"):
            check(f"nominal : {key} au META", key in meta)
        check("nominal : hashes des sorties au META", "  out/uniform_400_s3.txt" in meta)
        check("nominal : PROFIL.txt copie identique",
              (out1 / "PROFIL.txt").read_text() == PROFILE_1x1x2)
        check("nominal : copie privee non modifiable",
              not os.access(out1 / "bin" / "mhgp6", os.W_OK))
        check("nominal : protocole archive (lanceur, validateur, agregateur)",
              all((out1 / "protocole" / f).is_file()
                  for f in ("campagne_locale.sh", "pentes.py", "agregateur.py")))
        check("nominal : publication atomique (plus de .partial)",
              not Path(str(out1) + ".partial").exists())

        # Dossier preexistant : refus (un reçu ne s'ecrit jamais en place).
        rc0 = run_launcher(out1, src, profile)
        check("dossier preexistant : refus rc=2", rc0 == 2, f"rc={rc0}")

        # 2. Auto-alteration de la copie privee : INVALID sans DONE.
        src2 = base / "faux_tamper"
        write_exec(src2, FAKE_TAMPER)
        out2 = base / "tamper"
        rc2 = run_launcher(out2, src2, profile)
        # une campagne INVALIDE n'est JAMAIS publiee : elle reste en .partial
        part2 = Path(str(out2) + ".partial")
        check("copie alteree : jamais publiee (reste en .partial)", not out2.exists() and part2.exists())
        status2 = (part2 / "STATUS.txt").read_text().splitlines()
        check("copie alteree : rc=3", rc2 == 3, f"rc={rc2}")
        check("copie alteree : marqueur INVALID hash apres",
              any(line.startswith("INVALID hash apres") for line in status2))
        check("copie alteree : PAS de DONE (l'agregateur refuse)", "DONE" not in status2)

        # 3. VRAIE reconstruction concurrente : la source est remplacee
        # pendant le premier tuple — les deux tuples executent la MEME copie
        # et la campagne finit DONE (isolation causale).
        src3 = base / "faux_source"
        write_exec(src3, FAKE_REBUILD_SOURCE)
        out3 = base / "rebuild"
        rc3 = run_launcher(out3, src3, profile, {"SOURCE_A_REMPLACER": str(src3)})
        status3 = (out3 / "STATUS.txt").read_text().splitlines()
        hashes3 = (out3 / "HASHES.txt").read_text().splitlines()
        check("source remplacee : DONE et deux tuples code=0",
              rc3 == 0 and status3[-1] == "DONE" and
              sum(1 for line in status3 if line.startswith("code=0 ")) == 2, f"rc={rc3}")
        check("source remplacee : hashes homogenes (la copie privee est restee la seule executee)",
              len({h.split()[0][6:] for h in hashes3} | {h.split()[1][6:] for h in hashes3}) == 1)
        check("source remplacee : la source a bien ete remplacee pendant la campagne",
              (base / (src3.name + ".remplacee")).exists() or (src3.with_suffix(".remplacee")).exists()
              or Path(str(src3) + ".remplacee").exists())

        # 4. Copie discordante de la source a la copie : refus AVANT tout run
        # (faux sha256sum qui ment sur la source).
        fakebin = base / "fbin"
        fakebin.mkdir()
        write_exec(fakebin / "sha256sum", """#!/usr/bin/env bash
if [ "${1:-}" = "%SRC%" ]; then
  echo "1111111111111111111111111111111111111111111111111111111111111111  $1"
else
  exec /usr/bin/sha256sum "$@"
fi
""".replace("%SRC%", str(src)))
        out4 = base / "mismatch"
        rc4 = run_launcher(out4, src, profile,
                           {"PATH": f"{fakebin}:{os.environ['PATH']}"})
        check("copie != source : refus rc=2 avant le premier run",
              rc4 == 2 and not (out4 / "STATUS.txt").exists(), f"rc={rc4}")

        # 5. Profil absent / incomplet : refus avant le premier run.
        out5 = base / "sansprofil"
        rc5 = run_launcher(out5, src, base / "inexistant.txt")
        badp = base / "PROFIL_INCOMPLET.txt"
        badp.write_text("profil=x\nfamilles=\nn=400\ngraines=3\n")
        out6 = base / "profilvide"
        rc6 = run_launcher(out6, src, badp)
        check("profil absent : refus rc=2", rc5 == 2, f"rc={rc5}")
        check("profil aux familles vides : refus rc=2", rc6 == 2, f"rc={rc6}")

    if failures:
        return 1
    print("campagne_gate : provenance d'executable prouvee (nominal, tamper prive, VRAIE reconstruction "
          "concurrente de la source, copie discordante refusee, profil obligatoire)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

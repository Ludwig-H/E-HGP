#!/usr/bin/env bash
# SELFTEST de revalidate_v6_receipt.sh (audit serie C § 5.19.3) : le
# re-jugement d'un recu durable est ferme contre son propre outil. Sur une
# COPIE d'un recu durable versionne : nominal vert ; puis chaque mutation
# doit etre REFUSEE pour sa cause exacte — ligne rc dupliquee, fichier non
# liste, piece durable absente, validateur qui altere le recu (le controle
# final domine), V6_RESUMES_DIR pointant dans le recu (garde permanente du
# validateur), recu de reprise. Ne touche JAMAIS GCP.
# Usage : selftest_revalidate_v6.sh [recu durable] (defaut : le dernier reçu
#         session_g4_* commite sous morsehgp3D_v6/receipts)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/.." && pwd)"
REVALIDATE="${HERE}/revalidate_v6_receipt.sh"
VALIDATOR="${HERE}/validate_v6_campaign.py"
SOURCE="${1:-}"
if [ -z "${SOURCE}" ]; then
  SOURCE="$(ls -d "${ROOT}"/morsehgp3D_v6/receipts/session_g4_*/ 2>/dev/null | sort | tail -n 1)"
  SOURCE="${SOURCE%/}"
fi
[ -f "${SOURCE}/RECU_SESSION.txt" ] || { echo "REFUS : recu source absent (${SOURCE})" >&2; exit 2; }
case "$(sed -n 's/^issue=//p' "${SOURCE}/RECU_SESSION.txt")" in reprise_*) echo "REFUS : le recu source est un recu de reprise" >&2; exit 2 ;; esac

WORK="$(mktemp -d "${TMPDIR:-/tmp}/ehgp-selftest-revalid.XXXXXXXX")"
trap 'rm -rf "${WORK}"' EXIT
FAILS=0
check_true() { # $1 = nom, reste = commande
  local name="$1"; shift
  if "$@"; then echo "ok : ${name}"; else echo "ECHEC selftest : ${name}"; FAILS=$((FAILS + 1)); fi
}
rehash() { # $1 = recu
  ( cd "$1" && find . -type f ! -name SHA256SUMS -printf '%P\n' | sort | xargs -d '\n' sha256sum > SHA256SUMS )
}
fresh_copy() { # $1 = nom de cas -> imprime le chemin
  local d="${WORK}/$1"
  rm -rf "${d}"; cp -r "${SOURCE}" "${d}"; printf '%s' "${d}"
}
run_reval() { # $1 = recu, [$2 = validateur] -> rc dans REVAL_RC, sortie dans REVAL_OUT
  # Un validateur non canonique n'est admis qu'en mode selftest EXPLICITE.
  REVAL_RC=0
  REVAL_OUT="$(cd "${ROOT}" && EHGP_REVALIDATE_SELFTEST="${2:+1}" bash "${REVALIDATE}" "$1" ${2:+"$2"} 2>&1)" || REVAL_RC=$?
}

echo "selftest revalidate v6 : recu source $(basename "${SOURCE}")"

# ---- Nominal : copie intacte => code du validateur (0 attendu sur un recu
# valide), recu intact apres re-validation.
D="$(fresh_copy nominal)"; run_reval "${D}"
check_true "nominal : copie intacte re-validee rc=0, recu intact" \
  bash -c "[ \"\$1\" -eq 0 ] && printf '%s' \"\$2\" | grep -q 'recu intact apres re-validation'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Ligne rc dupliquee (journal rejoue) : refus, meme avec rehash.
D="$(fresh_copy rc_double)"; echo "remote_campaign_rc=0" >> "${D}/session.log"; rehash "${D}"; run_reval "${D}"
check_true "remote_campaign_rc duplique (rehash) : REFUS exactement une ligne" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'exactement une ligne exigee'" _ "${REVAL_RC}" "${REVAL_OUT}"
D="$(fresh_copy scp_double)"; echo "scp_rc=0" >> "${D}/session.log"; rehash "${D}"; run_reval "${D}"
check_true "scp_rc duplique (rehash) : REFUS exactement une ligne" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'scp_rc absent ou multiple'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Fichier present non liste (sans rehash : SHA256SUMS reste vrai pour
# les fichiers listes) : refus de l'ensemble exact.
D="$(fresh_copy intrus)"; echo "intrus" > "${D}/out/intrus.txt"; run_reval "${D}"
check_true "fichier non liste dans le recu : REFUS ensemble exact" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'ensemble des fichiers du recu != SHA256SUMS'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Piece durable absente (avec rehash : l'ensemble est coherent, la piece manque).
D="$(fresh_copy resume_absent)"; rm "${D}/matrice_resume.txt"; rehash "${D}"; run_reval "${D}"
check_true "matrice_resume.txt supprime (rehash) : REFUS piece durable absente" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'piece durable absente du recu (matrice_resume.txt)'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Validateur qui ALTERE le recu (ecrit dans session.log) puis rend 0 :
# le controle final doit dominer (rc 3), jamais 0.
FAKEV="${WORK}/faux_validateur.py"
cat > "${FAKEV}" <<'EOF'
import os, shutil, sys
def _copie_resumes(recu):
    work = os.environ["V6_RESUMES_DIR"]
    for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
        src = os.path.join(recu, r + "_resume.txt")
        if os.path.exists(src):
            shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
recu = os.path.dirname(sys.argv[7])  # profil_campagne.txt du recu
_copie_resumes(recu)
with open(os.path.join(recu, "session.log"), "a", encoding="utf-8") as fh:
    fh.write("altere par un faux validateur\n")
print("campaign_status=verifie_non_decisionnel (FAUX)")
sys.exit(0)
EOF
D="$(fresh_copy validateur_alterant)"; run_reval "${D}" "${FAKEV}"
check_true "validateur alterant session.log puis rc=0 : le controle final DOMINE (rc 3, RECU ALTERE)" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RECU ALTERE PENDANT LA RE-VALIDATION'" _ "${REVAL_RC}" "${REVAL_OUT}"
# Variante : validateur qui AJOUTE un fichier au recu (SHA256SUMS des listes
# reste vrai) puis rend 0 : l'ensemble final differe => rc 3.
FAKEV2="${WORK}/faux_validateur_ajout.py"
cat > "${FAKEV2}" <<'EOF'
import os, shutil, sys
def _copie_resumes(recu):
    work = os.environ["V6_RESUMES_DIR"]
    for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
        src = os.path.join(recu, r + "_resume.txt")
        if os.path.exists(src):
            shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
recu = os.path.dirname(sys.argv[7])
_copie_resumes(recu)
with open(os.path.join(recu, "ajout_par_validateur.txt"), "w", encoding="utf-8") as fh:
    fh.write("x\n")
sys.exit(0)
EOF
D="$(fresh_copy validateur_ajoutant)"; run_reval "${D}" "${FAKEV2}"
check_true "validateur ajoutant un fichier au recu puis rc=0 : ensemble final different (rc 3)" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RECU ALTERE PENDANT LA RE-VALIDATION'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Validateur qui altere PUIS regenere SHA256SUMS (meme ensemble de
# noms, nouveaux hashes) puis rend 0 : le manifeste initial est lie par ses
# octets => rc 3.
FAKEV3="${WORK}/faux_validateur_rehash.py"
cat > "${FAKEV3}" <<'EOF'
import hashlib
import os, shutil, sys
def _copie_resumes(recu):
    work = os.environ["V6_RESUMES_DIR"]
    for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
        src = os.path.join(recu, r + "_resume.txt")
        if os.path.exists(src):
            shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
recu = os.path.dirname(sys.argv[7])
_copie_resumes(recu)
with open(os.path.join(recu, "session.log"), "a", encoding="utf-8") as fh:
    fh.write("altere puis rehash\n")
lines = []
for root, _dirs, files in os.walk(recu):
    for f in files:
        if f == "SHA256SUMS":
            continue
        path = os.path.join(root, f)
        rel = os.path.relpath(path, recu)
        h = hashlib.sha256(open(path, "rb").read()).hexdigest()
        lines.append(f"{h}  ./{rel}")
with open(os.path.join(recu, "SHA256SUMS"), "w", encoding="utf-8") as fh:
    fh.write("\n".join(sorted(lines)) + "\n")
sys.exit(0)
EOF
D="$(fresh_copy validateur_rehash)"; run_reval "${D}" "${FAKEV3}"
check_true "validateur alterant session.log PUIS regenerant SHA256SUMS, rc=0 : manifeste initial lie (rc 3)" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RECU ALTERE PENDANT LA RE-VALIDATION'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Validateur qui cree seulement un REPERTOIRE vide dans le recu puis
# rend 0 (§ 5.21) : l'inventaire des repertoires est recompare => rc 3.
FAKEV4="${WORK}/faux_validateur_repertoire.py"
cat > "${FAKEV4}" <<'EOF'
import os, shutil, sys
def _copie_resumes(recu):
    work = os.environ["V6_RESUMES_DIR"]
    for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
        src = os.path.join(recu, r + "_resume.txt")
        if os.path.exists(src):
            shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
recu = os.path.dirname(sys.argv[7])
_copie_resumes(recu)
os.mkdir(os.path.join(recu, "repertoire_vide_du_validateur"))
sys.exit(0)
EOF
D="$(fresh_copy validateur_repertoire)"; run_reval "${D}" "${FAKEV4}"
check_true "validateur creant un repertoire vide dans le recu puis rc=0 : inventaire des repertoires recompare (rc 3)" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RECU ALTERE PENDANT LA RE-VALIDATION'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Lien symbolique non hache a la racine : refus AVANT le validateur.
D="$(fresh_copy symlink)"; ln -s out/MANIFESTE_DISTANT.txt "${D}/lien_intrus"; run_reval "${D}"
check_true "lien symbolique ajoute au recu : REFUS entree non reguliere" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'entree non reguliere'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Repertoire inattendu.
D="$(fresh_copy repertoire)"; mkdir "${D}/autre"; echo x > "${D}/autre/x.txt"; run_reval "${D}"
check_true "repertoire inattendu dans le recu : REFUS" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'repertoire inattendu'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Entree dupliquee dans SHA256SUMS (hash exact) : l'ensemble n'est plus exact.
D="$(fresh_copy dup_entry)"; head -n 1 "${D}/SHA256SUMS" >> "${D}/SHA256SUMS"; run_reval "${D}"
check_true "entree dupliquee dans SHA256SUMS : REFUS ensemble exact" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'ensemble des fichiers du recu != SHA256SUMS'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Recu LEGACY (anterieur a la serie C : cinq resumes, sans axe matrice) :
# doit rester recevable quand il existe dans le depot.
LEGACY="${ROOT}/morsehgp3D_v6/receipts/session_g4_20260901_d98f47296d67_1788245493"
if [ -f "${LEGACY}/RECU_SESSION.txt" ] && ! grep -qE '^matrice_points=' "${LEGACY}/profil_campagne.txt"; then
  # Le validateur serie C juge ce recu anterieur `partial_or_failed` (axes
  # serie C absents de son profil) : ce qui est exige ici est que le WRAPPER
  # ne le refuse pas de lui-meme (aucun REFUS, pieces legacy acceptees) et
  # que le recu reste intact — le verdict du validateur est le sien.
  D="${WORK}/legacy"; rm -rf "${D}"; cp -r "${LEGACY}" "${D}"; run_reval "${D}"
  check_true "recu legacy (cinq resumes, sans axe matrice) : accepte par le wrapper (aucun REFUS), recu intact, verdict du validateur (rc 0 ou 1)" \
    bash -c "{ [ \"\$1\" -eq 0 ] || [ \"\$1\" -eq 1 ]; } && ! printf '%s' \"\$2\" | grep -q '^REFUS' && printf '%s' \"\$2\" | grep -q 'recu intact apres re-validation'" _ "${REVAL_RC}" "${REVAL_OUT}"
fi

# ---- Appel DIRECT du validateur avec V6_RESUMES_DIR dans le recu (ou un
# sous-repertoire) : garde permanente, refus 2, aucun resume ecrit.
D="$(fresh_copy resumes_dans_recu)"
COMMIT="$(sed -n 's/^source_commit=//p' "${D}/RECU_SESSION.txt")"
PAYLOAD="$(sed -n 's/^source_payload_sha256=//p' "${D}/RECU_SESSION.txt")"
MANIFEST="$(sed -n 's/^protocol_manifest_sha256=//p' "${D}/RECU_SESSION.txt")"
PROFIL_NOM="$(sed -n 's/^profil_canonique=//p' "${D}/profil_campagne.txt")"
git -C "${ROOT}" show "${COMMIT}:gcp-migration/profils/${PROFIL_NOM}.env" > "${WORK}/canon.env"
mkdir -p "${D}/out/sous"
rc=0; VOUT="$(cd "${ROOT}" && V6_RESUMES_DIR="${D}/out/sous" python3 "${VALIDATOR}" "${D}/out" "${COMMIT}" "${PAYLOAD}" "${MANIFEST}" 0 0 \
  "${D}/profil_campagne.txt" "${WORK}/canon.env" "${D}/manifest_revalide.txt" 2>&1)" || rc=$?
check_true "validateur direct, V6_RESUMES_DIR explicite DANS le recu : REFUS 2, aucun resume ecrit" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'HORS du recu' && [ ! -e \"\$3/out/sous/bench_resume.txt\" ]" _ "${rc}" "${VOUT}" "${D}"

# ---- § 5.22 : validateur MUET (fichier regulier rendant 0 sans ecrire de
# resume) => rc 3 « VALIDATEUR MUET », jamais « recu intact ».
FAKEV5="${WORK}/faux_validateur_muet.sh"; printf '#!/bin/sh\nexit 0\n' > "${FAKEV5}"; chmod +x "${FAKEV5}"
D="$(fresh_copy validateur_muet)"; run_reval "${D}" "${FAKEV5}"
check_true "validateur muet (rc 0, aucun resume ecrit) : REFUS rc 3 VALIDATEUR MUET" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'VALIDATEUR MUET' && ! printf '%s' \"\$2\" | grep -q 'recu intact'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- /dev/null comme validateur HORS mode selftest : refus 2 avant tout appel.
D="$(fresh_copy validateur_devnull)"; rc=0; VOUT="$(cd "${ROOT}" && bash "${REVALIDATE}" "${D}" /dev/null 2>&1)" || rc=$?
check_true "validateur /dev/null hors mode selftest : REFUS 2 (validateur non canonique)" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'non canonique'" _ "${rc}" "${VOUT}"
# ---- SHA256SUMS IMBRIQUE (out/SHA256SUMS) non liste : un fichier comme un autre => ensemble exact refuse.
D="$(fresh_copy sha_imbrique)"; printf 'deadbeef  x\n' > "${D}/out/SHA256SUMS"; run_reval "${D}"
check_true "out/SHA256SUMS imbrique non liste au manifeste : REFUS ensemble exact (seul le manifeste racine est special)" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'ensemble des fichiers du recu != SHA256SUMS'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Nom d'entree a saut de ligne (repertoire physique « out\nmarques ») : inventaire non injectif => refus.
D="$(fresh_copy nom_saut_de_ligne)"; mkdir "${D}/out
marques"; run_reval "${D}"
check_true "repertoire nomme avec un saut de ligne : REFUS (inventaire NUL injectif)" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'saut de ligne'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Mode change pendant la re-validation (validateur qui chmod un fichier) : « intact » couvre les modes.
FAKEV6="${WORK}/faux_validateur_chmod.py"
cat > "${FAKEV6}" <<'EOF'
import os, sys, shutil
recu = os.path.dirname(sys.argv[7])
os.chmod(os.path.join(recu, "session.log"), 0o755)
work = os.environ["V6_RESUMES_DIR"]
for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
    src = os.path.join(recu, r + "_resume.txt")
    if os.path.exists(src):
        shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
sys.exit(0)
EOF
D="$(fresh_copy validateur_chmod)"; run_reval "${D}" "${FAKEV6}"
check_true "validateur changeant un mode de fichier puis rc=0 (resumes recopies) : inventaire types/modes/noms different (rc 3)" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RECU ALTERE PENDANT LA RE-VALIDATION'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- § 5.22 (retour WIP) : resume re-produit DIFFERENT (un octet) => rc 3.
FAKEV7="${WORK}/faux_validateur_resume_altere.py"
cat > "${FAKEV7}" <<'EOF'
import os, shutil, sys
recu = os.path.dirname(sys.argv[7])
work = os.environ["V6_RESUMES_DIR"]
for r in ("bench", "queue", "sweep", "gpu", "frontier", "matrice", "gpuv6"):
    src = os.path.join(recu, r + "_resume.txt")
    if os.path.exists(src):
        shutil.copyfile(src, os.path.join(work, r + "_resume.txt"))
p = os.path.join(work, "matrice_resume.txt")
b = bytearray(open(p, "rb").read()); b[-2] ^= 1
open(p, "wb").write(bytes(b))
sys.exit(0)
EOF
D="$(fresh_copy resume_altere)"; run_reval "${D}" "${FAKEV7}"
check_true "validateur recopiant les resumes puis alterant un octet de l'un d'eux : RESUME DIFFERENT => rc 3" \
  bash -c "[ \"\$1\" -eq 3 ] && printf '%s' \"\$2\" | grep -q 'RESUME DIFFERENT'" _ "${REVAL_RC}" "${REVAL_OUT}"
# ---- Repertoire racine nomme « out marques » (espace) : l'allowlist NUL en Python le refuse.
D="$(fresh_copy nom_espace)"; mkdir "${D}/out marques"; run_reval "${D}"
check_true "repertoire racine « out marques » (espace) : REFUS repertoire inattendu (sequence NUL, jamais un for sur du texte)" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'repertoire inattendu'" _ "${REVAL_RC}" "${REVAL_OUT}"

# ---- Recu de reprise : jamais requalifie.
D="$(fresh_copy reprise)"; sed -i 's/^issue=.*/issue=reprise_partielle/' "${D}/RECU_SESSION.txt"; rehash "${D}"; run_reval "${D}"
check_true "recu de reprise (issue=reprise_*) : REFUS" \
  bash -c "[ \"\$1\" -eq 2 ] && printf '%s' \"\$2\" | grep -q 'recu de reprise'" _ "${REVAL_RC}" "${REVAL_OUT}"

if [ "${FAILS}" -ne 0 ]; then
  echo "selftest revalidate v6 : ${FAILS} echec(s)"
  exit 1
fi
echo "selftest revalidate v6 : re-jugement ferme contre son outil (ensemble exact, pieces exigees, rc uniques, controle final dominant, garde V6_RESUMES_DIR permanente, reprise refusee)"

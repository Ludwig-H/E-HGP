#!/usr/bin/env bash
# Lanceur de campagne LOCALE v6 — provenance d'executable LIEE de bout en
# bout (alerte auditeur « campagnes CPU », trois tours) :
#   - le dossier de campagne DOIT etre inexistant ; tout se construit dans
#     `<OUT>.partial` et le terminal est PUBLIE ATOMIQUEMENT (mv) — une
#     campagne interrompue ou INVALIDE reste en `.partial`, jamais publiee ;
#   - BIN_SOURCE=auto (mode DECISIONNEL) : le binaire est CONSTRUIT depuis
#     `git archive <pin>` dans le dossier de campagne — la provenance
#     binaire<-commit est PROUVEE, pas declaree ; un chemin explicite reste
#     possible pour l'exploratoire (grave comme tel) ;
#   - copie privee immuable (chmod 555), egalite copie/source verifiee,
#     hash avant/apres chaque tuple (HASHES.txt), INVALID sans DONE sur
#     divergence ;
#   - les COPIES EXACTES du lanceur, du validateur (bench/pentes.py), de
#     l'agregateur (bench/agregateur.py) et du profil sont ARCHIVEES dans
#     `protocole/` et leurs sha256 graves au META avant le premier tuple —
#     pentes.py les RECOUPE (liaison, pas seulement enregistrement) ;
#   - worktree des sources propre exige (CAMPAGNE_ALLOW_DIRTY=1 pour une
#     capture exploratoire assumee) ; charge concurrente gravee.
# Fixtures : tests/campagne_gate.py.
#
# Usage : campagne_locale.sh OUT_DIR BIN_SOURCE|auto "RECU" PROFIL_FICHIER
set -u
OUT="${1:?dossier de campagne requis}"
SRC="${2:?binaire source requis (ou 'auto' : build depuis le pin)}"
RECU="${3:?ligne recu= requise}"
PROFIL="${4:?fichier de profil requis (autorite de matrice)}"
THREADS="${THREADS:-8}"

if [ -e "${OUT}" ] || [ -e "${OUT}.partial" ]; then
  echo "REFUS : dossier de campagne preexistant (${OUT}[.partial]) — un reçu ne s'ecrit jamais en place" >&2
  exit 2
fi
[ -f "${PROFIL}" ] || { echo "REFUS : profil absent (${PROFIL})" >&2; exit 2; }
FAMILIES="$(sed -n 's/^familles=//p' "${PROFIL}" | head -1)"
NS="$(sed -n 's/^n=//p' "${PROFIL}" | head -1)"
SEEDS="$(sed -n 's/^graines=//p' "${PROFIL}" | head -1)"
if [ -z "${FAMILIES// /}" ] || [ -z "${NS// /}" ] || [ -z "${SEEDS// /}" ]; then
  echo "REFUS : profil incomplet (familles/n/graines requis, non vides)" >&2
  exit 2
fi

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${HERE}/../.." && pwd)"
PIN="$(git -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null || echo hors_depot)"
DIRTY="$(git -C "${REPO_ROOT}" status --porcelain -- \
  morsehgp3D_v7/src morsehgp3D_v7/cli morsehgp3D_v7/CMakeLists.txt 2>/dev/null | wc -l)"
if [ "${DIRTY}" -ne 0 ] && [ "${CAMPAGNE_ALLOW_DIRTY:-0}" != "1" ]; then
  echo "REFUS : ${DIRTY} fichier(s) source modifie(s) dans le worktree — committer d'abord (ou CAMPAGNE_ALLOW_DIRTY=1 pour une capture exploratoire assumee)" >&2
  exit 2
fi

WORKD="${OUT}.partial"
mkdir -p "${WORKD}/out" "${WORKD}/bin" "${WORKD}/protocole"

# Provenance binaire : mode DECISIONNEL `auto` = build depuis git archive du
# pin (binaire <- commit prouve) ; sinon chemin explicite, grave comme
# provenance declaree.
BIN_PROVENANCE=""
if [ "${SRC}" = "auto" ]; then
  [ "${PIN}" != "hors_depot" ] || { echo "REFUS : mode auto hors depot git" >&2; rm -rf "${WORKD}"; exit 2; }
  SRCDIR="${WORKD}/source_pin"
  mkdir -p "${SRCDIR}"
  git -C "${REPO_ROOT}" archive "${PIN}" morsehgp3D_v7 | tar -x -C "${SRCDIR}" || { echo "REFUS : git archive du pin impossible" >&2; rm -rf "${WORKD}"; exit 2; }
  cmake -S "${SRCDIR}/morsehgp3D_v7" -B "${WORKD}/build_pin" -DCMAKE_BUILD_TYPE=Release > "${WORKD}/build_pin.log" 2>&1 \
    && cmake --build "${WORKD}/build_pin" --parallel "${THREADS}" --target mhgp7 >> "${WORKD}/build_pin.log" 2>&1 \
    || { echo "REFUS : build du pin en echec (${WORKD}/build_pin.log)" >&2; exit 2; }
  SRC="${WORKD}/build_pin/mhgp7"
  BIN_PROVENANCE="construit depuis git archive ${PIN} (build_pin.log conserve) — liaison binaire<-commit PROUVEE"
else
  BIN_PROVENANCE="chemin explicite ${SRC} — provenance DECLAREE (mode exploratoire)"
fi

cp "${SRC}" "${WORKD}/bin/mhgp7" && chmod 555 "${WORKD}/bin/mhgp7" || exit 2
BIN="${WORKD}/bin/mhgp7"
H="$(sha256sum "${BIN}" | awk '{print $1}')"
H_SRC="$(sha256sum "${SRC}" | awk '{print $1}')"
if [ "${H}" != "${H_SRC}" ]; then
  echo "REFUS : la copie privee (${H}) differe de la source relue (${H_SRC}) — copie concurrente ou dechiree, aucun run" >&2
  exit 2
fi

# ARCHIVAGE du protocole exact (liaison par pentes.py, pas seulement META).
cp "${PROFIL}" "${WORKD}/PROFIL.txt"
cp "$0" "${WORKD}/protocole/campagne_locale.sh"
cp "${HERE}/pentes.py" "${WORKD}/protocole/pentes.py"
cp "${HERE}/agregateur.py" "${WORKD}/protocole/agregateur.py"
{
  echo "recu=${RECU}"
  echo "pin=${PIN}"
  echo "worktree_sources=${DIRTY} fichier(s) modifie(s) (0 exige hors CAMPAGNE_ALLOW_DIRTY)"
  echo "binaire_prive=bin/mhgp7 (copie immuable chmod 555, seule executee ; ${BIN_PROVENANCE})"
  echo "sha256_binaire_prive=${H}"
  echo "autorite_profil=PROFIL.txt sha256=$(sha256sum "${WORKD}/PROFIL.txt" | awk '{print $1}')"
  echo "sha256_lanceur=$(sha256sum "${WORKD}/protocole/campagne_locale.sh" | awk '{print $1}')"
  echo "sha256_validateur=$(sha256sum "${WORKD}/protocole/pentes.py" | awk '{print $1}')"
  echo "sha256_agregateur=$(sha256sum "${WORKD}/protocole/agregateur.py" | awk '{print $1}')"
  echo "toolchain=$(g++ --version 2>/dev/null | head -1 || echo inconnue)"
  echo "machine=locale partagee — les temps ne sont PAS des mesures, seuls les compteurs deterministes font foi"
  echo "charge_concurrente_au_lancement=$(uptime | sed 's/.*load average: //')"
  echo "commande=bin/mhgp7 --family=<fam> --n=<n> --seed=<s> --s=8 --smax=11 --threads=${THREADS} --digest (hash verifie avant et apres chaque tuple, HASHES.txt)"
  echo "familles=${FAMILIES} ; n=${NS} ; graines=${SEEDS}"
  echo "debut_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "${WORKD}/META.txt"
: > "${WORKD}/STATUS.txt"
: > "${WORKD}/HASHES.txt"
abort_invalid() {
  echo "INVALID $1" >> "${WORKD}/STATUS.txt"
  echo "campagne INVALIDE : $1" >> "${WORKD}/META.txt"
  echo "campagne INVALIDE, laissee en ${WORKD} (jamais publiee)" >&2
  exit 3
}
for fam in ${FAMILIES}; do
  for n in ${NS}; do
    for seed in ${SEEDS}; do
      hb="$(sha256sum "${BIN}" | awk '{print $1}')"
      [ "${hb}" = "${H}" ] || abort_invalid "hash avant ${fam}_${n}_s${seed} : ${hb} != ${H}"
      f="${WORKD}/out/${fam}_${n}_s${seed}.txt"
      t0=${SECONDS}
      "${BIN}" --family="${fam}" --n="${n}" --seed="${seed}" --s=8 --smax=11 --threads="${THREADS}" --digest \
        > "${f}" 2> "${f}.err"
      rc=$?
      ha="$(sha256sum "${BIN}" | awk '{print $1}')"
      [ "${ha}" = "${H}" ] || abort_invalid "hash apres ${fam}_${n}_s${seed} : ${ha} != ${H}"
      echo "avant=${hb} apres=${ha} run=${fam}_${n}_s${seed}" >> "${WORKD}/HASHES.txt"
      echo "code=${rc} fam=${fam} n=${n} seed=${seed} secs=$((SECONDS - t0))" >> "${WORKD}/STATUS.txt"
    done
  done
done
{
  echo "fin_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "charge_concurrente_a_la_fin=$(uptime | sed 's/.*load average: //')"
  ( cd "${WORKD}" && sha256sum out/*.txt )
} >> "${WORKD}/META.txt"
echo DONE >> "${WORKD}/STATUS.txt"
# Le build du pin n'entre pas dans le reçu publie (des Go d'objets) : seul
# son journal et le binaire prive restent.
rm -rf "${WORKD}/build_pin" "${WORKD}/source_pin"
# PUBLICATION ATOMIQUE du terminal.
mv "${WORKD}" "${OUT}"


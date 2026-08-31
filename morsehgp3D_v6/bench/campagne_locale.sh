#!/usr/bin/env bash
# Lanceur de campagne LOCALE v6 — provenance d'executable prouvee (alerte
# auditeur « campagne CPU mixte » du 31 aout, deux tours) :
#   - le binaire est COPIE dans un repertoire prive de campagne AVANT le
#     premier tuple, rendu non modifiable (chmod 555), et SEULE cette copie
#     est executee ; le SHA-256 de la copie doit EGALER celui de la source
#     relu APRES la copie (une copie dechiree ou concurrente est REFUSEE
#     avant le premier run — correction 4 de l'alerte) ;
#   - le WORKTREE des sources doit etre propre (src/cli/CMakeLists) — refus
#     sinon (CAMPAGNE_ALLOW_DIRTY=1 pour une capture exploratoire assumee) ;
#   - l'AUTORITE DE PROFIL (fichier externe familles/n/graines) et les
#     hashes du lanceur, du validateur (bench/pentes.py) et de l'agregateur
#     (bench/agregateur.py) sont graves au META AVANT le premier tuple
#     (correction 3) ;
#   - hash reverifie avant ET apres chaque tuple (HASHES.txt) ; toute
#     divergence ARRETE la campagne sans DONE (`INVALID <motif>`) —
#     l'agregateur et pentes.py refusent une campagne sans DONE ;
#   - charge concurrente gravee (les temps locaux ne sont jamais des
#     mesures).
# Fixtures : tests/campagne_gate.py (auto-alteration de la copie privee ->
# INVALID sans DONE ; REMPLACEMENT DE LA SOURCE apres la copie -> les tuples
# executent la MEME copie et finissent DONE ; desaccord copie/source a la
# copie -> refus avant le premier run).
#
# Usage : campagne_locale.sh OUT_DIR BIN_SOURCE "RECU" PROFIL_FICHIER
#   PROFIL_FICHIER : cle=valeur — familles=..., n=..., graines=...
set -u
OUT="${1:?dossier de campagne requis}"
SRC="${2:?binaire source requis}"
RECU="${3:?ligne recu= requise}"
PROFIL="${4:?fichier de profil requis (autorite de matrice)}"
THREADS="${THREADS:-8}"

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
DIRTY="$(git -C "${REPO_ROOT}" status --porcelain -- \
  morsehgp3D_v6/src morsehgp3D_v6/cli morsehgp3D_v6/CMakeLists.txt 2>/dev/null | wc -l)"
if [ "${DIRTY}" -ne 0 ] && [ "${CAMPAGNE_ALLOW_DIRTY:-0}" != "1" ]; then
  echo "REFUS : ${DIRTY} fichier(s) source modifie(s) dans le worktree — committer d'abord (ou CAMPAGNE_ALLOW_DIRTY=1 pour une capture exploratoire assumee)" >&2
  exit 2
fi

mkdir -p "${OUT}/out" "${OUT}/bin"
cp "${SRC}" "${OUT}/bin/mhgp6" && chmod 555 "${OUT}/bin/mhgp6" || exit 2
BIN="${OUT}/bin/mhgp6"
H="$(sha256sum "${BIN}" | awk '{print $1}')"
H_SRC="$(sha256sum "${SRC}" | awk '{print $1}')"
if [ "${H}" != "${H_SRC}" ]; then
  echo "REFUS : la copie privee (${H}) differe de la source relue (${H_SRC}) — copie concurrente ou dechiree, aucun run" >&2
  exit 2
fi
cp "${PROFIL}" "${OUT}/PROFIL.txt"
{
  echo "recu=${RECU}"
  echo "pin=$(git -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null || echo hors_depot)"
  echo "worktree_sources=${DIRTY} fichier(s) modifie(s) (0 exige hors CAMPAGNE_ALLOW_DIRTY)"
  echo "binaire_prive=bin/mhgp6 (copie immuable chmod 555, seule executee ; source ${SRC} copiee avant le premier tuple, egalite copie/source verifiee)"
  echo "sha256_binaire_prive=${H}"
  echo "autorite_profil=PROFIL.txt sha256=$(sha256sum "${OUT}/PROFIL.txt" | awk '{print $1}')"
  echo "sha256_lanceur=$(sha256sum "$0" | awk '{print $1}')"
  echo "sha256_validateur=$(sha256sum "${HERE}/pentes.py" | awk '{print $1}')"
  echo "sha256_agregateur=$(sha256sum "${HERE}/agregateur.py" | awk '{print $1}')"
  echo "toolchain=$(g++ --version 2>/dev/null | head -1 || echo inconnue)"
  echo "machine=locale partagee — les temps ne sont PAS des mesures, seuls les compteurs deterministes font foi"
  echo "charge_concurrente_au_lancement=$(uptime | sed 's/.*load average: //')"
  echo "commande=${BIN} --family=<fam> --n=<n> --seed=<s> --s=8 --smax=11 --threads=${THREADS} --digest (hash verifie avant et apres chaque tuple, HASHES.txt)"
  echo "familles=${FAMILIES} ; n=${NS} ; graines=${SEEDS}"
  echo "debut_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "${OUT}/META.txt"
: > "${OUT}/STATUS.txt"
: > "${OUT}/HASHES.txt"
abort_invalid() {
  echo "INVALID $1" >> "${OUT}/STATUS.txt"
  echo "campagne INVALIDE : $1" >> "${OUT}/META.txt"
  exit 3
}
for fam in ${FAMILIES}; do
  for n in ${NS}; do
    for seed in ${SEEDS}; do
      hb="$(sha256sum "${BIN}" | awk '{print $1}')"
      [ "${hb}" = "${H}" ] || abort_invalid "hash avant ${fam}_${n}_s${seed} : ${hb} != ${H}"
      f="${OUT}/out/${fam}_${n}_s${seed}.txt"
      t0=${SECONDS}
      "${BIN}" --family="${fam}" --n="${n}" --seed="${seed}" --s=8 --smax=11 --threads="${THREADS}" --digest \
        > "${f}" 2> "${f}.err"
      rc=$?
      ha="$(sha256sum "${BIN}" | awk '{print $1}')"
      [ "${ha}" = "${H}" ] || abort_invalid "hash apres ${fam}_${n}_s${seed} : ${ha} != ${H}"
      echo "avant=${hb} apres=${ha} run=${fam}_${n}_s${seed}" >> "${OUT}/HASHES.txt"
      echo "code=${rc} fam=${fam} n=${n} seed=${seed} secs=$((SECONDS - t0))" >> "${OUT}/STATUS.txt"
    done
  done
done
{
  echo "fin_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "charge_concurrente_a_la_fin=$(uptime | sed 's/.*load average: //')"
  ( cd "${OUT}" && sha256sum out/*.txt )
} >> "${OUT}/META.txt"
echo DONE >> "${OUT}/STATUS.txt"

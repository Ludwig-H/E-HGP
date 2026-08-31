#!/usr/bin/env bash
# Lanceur de campagne LOCALE v6 — provenance d'executable prouvee (alerte
# auditeur « campagne CPU mixte » du 31 aout) :
#   - le binaire est COPIE dans un repertoire prive de campagne AVANT le
#     premier tuple, rendu non modifiable (chmod 555), et SEULE cette copie
#     est executee — une reconstruction concurrente de la source ne peut
#     plus changer l'executable au milieu de la matrice ;
#   - son SHA-256 est grave au META et REVERIFIE avant ET apres chaque
#     tuple (HASHES.txt, une ligne avant/apres par run) ;
#   - toute divergence ARRETE la campagne sans DONE et grave le marqueur
#     `INVALID <motif>` dans STATUS.txt — l'agregateur (bench/pentes.py)
#     refuse une campagne sans DONE terminal ;
#   - la charge concurrente observee est gravee (les temps ne sont jamais
#     des mesures en local).
# Fixture de reconstruction concurrente : tests/campagne_gate.py (le faux
# binaire s'auto-altere -> INVALID sans DONE ; nominal -> DONE + hashes).
#
# Usage : campagne_locale.sh OUT_DIR BIN_SOURCE "RECU" "FAMILLES" "TAILLES" "GRAINES"
set -u
OUT="${1:?dossier de campagne requis}"
SRC="${2:?binaire source requis}"
RECU="${3:?ligne recu= requise}"
FAMILIES="${4:?familles requises}"
NS="${5:?tailles requises}"
SEEDS="${6:?graines requises}"
THREADS="${THREADS:-8}"

mkdir -p "${OUT}/out" "${OUT}/bin"
cp "${SRC}" "${OUT}/bin/mhgp6" && chmod 555 "${OUT}/bin/mhgp6" || exit 2
BIN="${OUT}/bin/mhgp6"
H="$(sha256sum "${BIN}" | awk '{print $1}')"
{
  echo "recu=${RECU}"
  echo "pin=$(git rev-parse HEAD 2>/dev/null || echo hors_depot)"
  echo "binaire_prive=bin/mhgp6 (copie immuable chmod 555, seule executee ; source ${SRC} copiee avant le premier tuple)"
  echo "sha256_binaire_prive=${H}"
  echo "sha256_source_a_la_copie=$(sha256sum "${SRC}" | awk '{print $1}')"
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
  sha256sum "${OUT}"/out/*.txt
} >> "${OUT}/META.txt"
echo DONE >> "${OUT}/STATUS.txt"

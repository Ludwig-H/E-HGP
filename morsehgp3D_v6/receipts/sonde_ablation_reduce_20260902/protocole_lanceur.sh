#!/usr/bin/env bash
# SONDE D'ABLATION du reduce (2 septembre 2026, arbre § 5.10 de
# REPONSE_AUDITEURS_MULTICPU_V6 : « decomposer la fenetre AVANT d'ecrire un
# palier ») — reçu local, JAMAIS une decision ni un mur :
#   - binaire de PROFIL sous MHGP6_TESTING (mhgp6_profile_sonde), fenetres
#     par K (profil_reduce), join=1 pour isoler l'etage B ; le binaire
#     produit non instrumente reste le seul mur de reference ;
#   - quatre bras : aucune | ablation-mat-sans-copie | ablation-mat-sans-tris
#     | ablation-post-cle-factice ; chaque ablation CHANGE l'objet (tuee code
#     4 par mhgp6_mutant_ablation-*) — leurs sorties ne valent que par leurs
#     fenetres ;
#   - ordre aller/retour entre repetitions (derive de charge contrebalancee),
#     taskset explicite, loadavg avant/apres chaque run, sha256 du binaire,
#     commit et proprete du worktree graves ; dossier terminal publie
#     atomiquement (mv) depuis `<OUT>.partial`.
# Usage : sonde_ablation_reduce.sh OUT_DIR BIN_SONDE [N_LIST="8000 16000 32000"] [REPS=3]
set -u
OUT="${1:?dossier de reçu requis}"
BIN="${2:?binaire mhgp6_profile_sonde requis}"
N_LIST="${3:-8000 16000 32000}"
REPS="${4:-3}"
THREADS="${THREADS:-8}"
CPUS="${CPUS:-0-7}"
FAMILY="${FAMILY:-uniform}"
ABLATIONS="aucune ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice"

if [ -e "${OUT}" ] || [ -e "${OUT}.partial" ]; then
  echo "REFUS : dossier de reçu preexistant (${OUT}[.partial]) — un reçu ne s'ecrit jamais en place" >&2
  exit 2
fi
[ -x "${BIN}" ] || { echo "REFUS : binaire absent ou non executable (${BIN})" >&2; exit 2; }
# Le binaire doit etre la cible de sonde : un nom inconnu est refuse (2) ;
# un binaire produit refuse --inject= tout court (2 aussi) — on distingue par
# l'acceptation d'un nom CONNU sur une entree minuscule.
if ! "${BIN}" --family=uniform --n=64 --threads=1 --inject=ablation-mat-sans-tris >/dev/null 2>&1; then
  echo "REFUS : ${BIN} n'accepte pas --inject=ablation-* (pas la cible mhgp6_profile_sonde ?)" >&2
  exit 2
fi

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${HERE}/../.." && pwd)"
PIN="$(git -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null || echo hors_depot)"
DIRTY="$(git -C "${REPO_ROOT}" status --porcelain -- \
  morsehgp3D_v6/src morsehgp3D_v6/cli morsehgp3D_v6/CMakeLists.txt morsehgp3D_v6/bench 2>/dev/null | wc -l)"

WORKD="${OUT}.partial"
mkdir -p "${WORKD}/out"
{
  echo "schema=e-hgp.sonde-ablation-reduce.v1"
  echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "commit=${PIN}"
  echo "worktree_sources_modifies=${DIRTY}"
  echo "binaire=${BIN}"
  echo "binaire_sha256=$(sha256sum "${BIN}" | awk '{print $1}')"
  echo "famille=${FAMILY} n_list=${N_LIST} reps=${REPS} threads=${THREADS} cpus=${CPUS} fold_inflight=2 fold_join=1 seed=3 s=8 smax=11"
  echo "ablations=${ABLATIONS}"
  echo "libstdcxx=$(readlink -f /usr/lib/x86_64-linux-gnu/libstdc++.so.6 2>/dev/null || echo inconnu)"
  echo "gcc=$(gcc -dumpfullversion 2>/dev/null || echo inconnu)"
  echo "hote=$(uname -srm)"
  echo "statut=sonde_locale_non_decisionnelle (attribution decomposee sur binaire instrumente ; jamais un mur)"
} > "${WORKD}/META.txt"
lscpu > "${WORKD}/lscpu.txt" 2>&1 || true
cp "${HERE}/sonde_ablation_reduce.sh" "${WORKD}/protocole_lanceur.sh"
cp "${HERE}/sonde_ablation_reduce.py" "${WORKD}/protocole_agregateur.py"

run_one() { # $1 = ablation, $2 = n, $3 = rep
  local abl="$1" n="$2" rep="$3"
  local tag="${abl}_n${n}_r${rep}"
  local inj=()
  [ "${abl}" = "aucune" ] || inj=("--inject=${abl}")
  local load_before load_after t0 t1 rc=0
  load_before="$(cut -d' ' -f1-3 /proc/loadavg)"
  t0="$(date +%s.%N)"
  taskset -c "${CPUS}" "${BIN}" "--family=${FAMILY}" "--n=${n}" --s=8 --smax=11 --seed=3 \
    "--threads=${THREADS}" --fold-inflight=2 --fold-join=1 "${inj[@]}" \
    > "${WORKD}/out/${tag}.txt" 2> "${WORKD}/out/${tag}.err" || rc=$?
  t1="$(date +%s.%N)"
  load_after="$(cut -d' ' -f1-3 /proc/loadavg)"
  {
    echo "ablation=${abl} n=${n} rep=${rep} code=${rc}"
    echo "commande=taskset -c ${CPUS} ${BIN} --family=${FAMILY} --n=${n} --s=8 --smax=11 --seed=3 --threads=${THREADS} --fold-inflight=2 --fold-join=1 ${inj[*]:-}"
    echo "duree_s=$(awk -v a="${t0}" -v b="${t1}" 'BEGIN{printf "%.3f", b - a}')"
    echo "loadavg_avant=${load_before}"
    echo "loadavg_apres=${load_after}"
  } > "${WORKD}/out/${tag}.status"
  echo "--- fini ${tag} (code=${rc}, $(sed -n 's/^duree_s=//p' "${WORKD}/out/${tag}.status")s)"
  return "${rc}"
}

echo "sonde ablation reduce : ${FAMILY} n=${N_LIST} reps=${REPS} bras=${ABLATIONS}"
ECHECS=0
for rep in $(seq 1 "${REPS}"); do
  ORDER="${ABLATIONS}"
  # Retour un rep sur deux (contrebalancement de la derive).
  if [ $((rep % 2)) -eq 0 ]; then
    ORDER="$(printf '%s\n' ${ABLATIONS} | tac | tr '\n' ' ')"
  fi
  for n in ${N_LIST}; do
    for abl in ${ORDER}; do
      run_one "${abl}" "${n}" "${rep}" || ECHECS=$((ECHECS + 1))
    done
  done
done
echo "echecs=${ECHECS}" >> "${WORKD}/META.txt"
python3 "${HERE}/sonde_ablation_reduce.py" "${WORKD}" > "${WORKD}/resume.txt" 2> "${WORKD}/resume.err" || {
  echo "INVALIDE : agregateur en echec (voir ${WORKD}/resume.err)" >&2
  exit 3
}
( cd "${WORKD}" && find . -type f ! -name SHA256SUMS -printf '%P\n' | sort | xargs -d '\n' sha256sum > SHA256SUMS )
if [ "${ECHECS}" -ne 0 ]; then
  echo "INVALIDE : ${ECHECS} run(s) en echec — reçu laisse en ${WORKD}, jamais publie" >&2
  exit 3
fi
mv "${WORKD}" "${OUT}"
echo "reçu publie : ${OUT}"
cat "${OUT}/resume.txt"

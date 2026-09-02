#!/usr/bin/env bash
# SONDE D'ABLATION du reduce — reçu FAIL-CLOSED (2 septembre 2026, arbre
# § 5.10 de REPONSE_AUDITEURS_MULTICPU_V6 « decomposer la fenetre AVANT
# d'ecrire un palier » ; fermeture minimale exigee par
# audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md, puis les cinq
# contre-fixtures encore vertes de la fin de son « Etat du WIP » et du § 5.21
# de REPONSE_AUDITEURS_MULTICPU_V6). Doctrine :
#   - BORNES EXPLORATOIRES NON CAUSALES (statut=exploratory_noncausal_upper_bounds) :
#     binaire de PROFIL sous MHGP6_TESTING (mhgp6_profile_sonde), fenetres
#     par K (profil_reduce), join=1 pour isoler l'etage B. Chaque ablation
#     CHANGE l'objet (tuee code 4 par mhgp6_mutant_ablation-*) : ses sorties
#     ne valent que par leurs fenetres — jamais un mur, jamais un benchmark,
#     jamais un choix de palier. Le bras ablation-post-cle-factice est une
#     BORNE COMPOSITE (lecture keys[] + tri de cles egales), pas « lecture
#     seule » : il change aussi la distribution du tri de la fenetre
#     materialisation_tri_copie.
#   - PLAN EQUILIBRE (carre de Williams 4x4 : ABCD BDAC CADB DCBA) : les
#     blocs sont les repetitions ; sur quatre blocs consecutifs chaque bras
#     occupe une fois chaque position et suit une fois chaque autre bras.
#     REPS doit etre un multiple de 4 (defaut 4), sinon REFUS ; le plan
#     (bloc, position, bras) est grave dans plan.txt AVANT le premier run et
#     la boucle l'execute tel quel. L'agregateur apparie PAR BLOC (bras −
#     aucune au sein du meme bloc et de la meme taille) puis publie
#     mediane/min/max des differences appariees ; les medianes brutes ne sont
#     qu'un second tableau.
#   - COPIE PRIVEE du binaire dans <OUT>.partial/bin/ (chmod 555), SEULE
#     executee (les sondes d'identite aussi) ; sha256 grave au META, verifie
#     AVANT et APRES chaque tuple (HASHES.txt) ; divergence => INVALIDE
#     (code 3), aucun mv de publication. Tout hash passe par la primitive
#     hash_de : exactement 64 hexadecimaux ou echec FATAL (un sha256sum muet
#     ne produit jamais une chaine vide « egale » a une autre chaine vide).
#   - FAIL-CLOSED : liste de tailles vide, taille DUPLIQUEE (deux tuples
#     porteraient le meme tag <bras>_n<n>_r<bloc> et s'ecraseraient),
#     REPS <= 0, REPS non multiple de 4, CPUS invalide, hash illisible,
#     binaire qui n'accepte pas --inject=ablation-* (binaire produit) ou qui
#     accepte un nom inconnu => REFUS 2 avant tout run (le .partial est
#     retire) ; un run a code non nul => INVALIDE 3 immediat ; l'agregateur
#     (copie archivee protocole_agregateur.py) exige l'ensemble EXACT bras x
#     tailles x repetitions, le triplet EXACT <tag>.txt|.err|.status par
#     tuple et rien d'autre dans out/, le plan de Williams (successions
#     ordonnees, pas seulement les positions), tout champ unique (META,
#     statut, plan, profil), la grammaire 64-hex de chaque hash, dix lignes
#     K, toutes les fenetres finies (jamais un zero substitue) et rend 1
#     sinon ; agregateur, generation de SHA256SUMS, INVENTAIRE EXACT du reçu
#     (les entrees du manifeste doivent etre exactement l'ensemble attendu :
#     fichiers fixes + triplets de out/ [+ diff de worktree]) puis
#     `sha256sum -c --strict` finale sont FATALS (3). Le manifeste couvre
#     TOUT fichier regulier sauf le seul ./SHA256SUMS racine (un
#     out/SHA256SUMS ou tout intrus est hache, donc visible, puis refuse par
#     l'inventaire). Le manifeste est le DERNIER fichier ecrit : apres sa
#     verification la seule operation restante est le `mv` de publication.
#     Un reçu INVALIDE reste en `<OUT>.partial`, jamais publie.
#   - Worktree : commit et nombre de fichiers modifies graves ; si non nul,
#     `git diff HEAD` embarque dans worktree_diff.patch (+ statut porcelain et
#     `git diff HEAD --summary --stat` dans worktree_diff_summary.txt), sinon
#     worktree_diff=aucun.
# Porte : tests/sonde_ablation_gate.py (faux binaire rapide, onze scenes,
# cas (a)–(n)).
# Usage : sonde_ablation_reduce.sh OUT_DIR BIN_SONDE [N_LIST="8000 16000 32000"] [REPS=4]
#   (un troisieme argument VIDE est une liste vide, donc un refus — pas le defaut)
#   ou  : sonde_ablation_reduce.sh --inventaire DIR
#   (lecture seule : liste, telle que le manifeste la couvrirait, des fichiers
#   reguliers de DIR — seul ./SHA256SUMS racine exclu ; sert a la porte)
set -u

# Inventaire du manifeste : tout fichier regulier, seul ./SHA256SUMS racine
# exclu (JAMAIS `! -name SHA256SUMS`, qui rendrait un out/SHA256SUMS invisible).
lister_fichiers() {
  find . -type f ! -path ./SHA256SUMS -printf '%P\n' | LC_ALL=C sort
}
if [ "${1-}" = "--inventaire" ]; then
  [ -d "${2-}" ] || { echo "REFUS : --inventaire exige un dossier existant" >&2; exit 2; }
  ( cd "$2" && lister_fichiers )
  exit $?
fi

OUT="${1:?dossier de reçu requis}"
BIN_SRC="${2:?binaire mhgp6_profile_sonde requis}"
N_LIST="${3-8000 16000 32000}"
REPS="${4-4}"
THREADS="${THREADS:-8}"
CPUS="${CPUS:-0-7}"
FAMILY="${FAMILY:-uniform}"
ABLATIONS="aucune ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice"
# Carre de Williams pour quatre traitements : chaque lettre une fois par
# position et chaque succession ordonnee (X puis Y) exactement une fois.
WILLIAMS=("A B C D" "B D A C" "C A D B" "D C B A")

refus() { echo "REFUS : $1" >&2; exit 2; }
bras_de() {
  case "$1" in
    A) echo aucune ;;
    B) echo ablation-mat-sans-copie ;;
    C) echo ablation-mat-sans-tris ;;
    D) echo ablation-post-cle-factice ;;
    *) refus "lettre de plan inconnue ($1)" ;;
  esac
}
# Primitive de hash FATALE : imprime le sha256 (64 hexadecimaux) ou rend 1.
# Jamais une chaine vide : un sha256sum muet, absent ou tronque est un echec.
hash_de() {
  local h
  h="$(sha256sum "$1" 2>/dev/null | awk '{print $1}')"
  [[ "${h}" =~ ^[0-9a-f]{64}$ ]] || return 1
  printf '%s\n' "${h}"
}

# --- Refus avant toute ecriture ------------------------------------------
if [ -e "${OUT}" ] || [ -e "${OUT}.partial" ]; then
  refus "dossier de reçu preexistant (${OUT}[.partial]) — un reçu ne s'ecrit jamais en place"
fi
# shellcheck disable=SC2086
N_LIST="$(printf '%s\n' ${N_LIST} | tr '\n' ' ' | sed 's/ *$//')"
[ -n "${N_LIST}" ] || refus "liste de tailles vide (N_LIST) — un reçu vide n'est pas un reçu"
NN=0
N_NORM=""
for n in ${N_LIST}; do
  case "${n}" in ''|*[!0-9]*) refus "taille non entiere (${n})" ;; esac
  [ "${#n}" -le 12 ] || refus "taille trop longue (${n})"
  n=$((10#${n}))   # forme canonique : 064 et 64 sont la meme taille (meme tag)
  [ "${n}" -gt 0 ] || refus "taille nulle"
  case " ${N_NORM} " in
    *" ${n} "*) refus "taille dupliquee dans N_LIST (${n}) — deux tuples porteraient le meme tag <bras>_n${n}_r<bloc> et s'ecraseraient" ;;
  esac
  N_NORM="${N_NORM}${N_NORM:+ }${n}"
  NN=$((NN + 1))
done
N_LIST="${N_NORM}"
case "${REPS}" in ''|*[!0-9]*) refus "REPS non entier (${REPS})" ;; esac
REPS=$((10#${REPS}))
[ "${REPS}" -gt 0 ] || refus "REPS <= 0 — un reçu vide n'est pas un reçu"
[ $((REPS % 4)) -eq 0 ] || refus "REPS=${REPS} n'est pas un multiple de 4 (carre de Williams 4x4 : chaque bras une fois a chaque position par groupe de quatre blocs)"
taskset -c "${CPUS}" true >/dev/null 2>&1 || refus "CPUS=${CPUS} invalide pour taskset"
[ -f "${BIN_SRC}" ] && [ -x "${BIN_SRC}" ] || refus "binaire absent, non regulier ou non executable (${BIN_SRC})"

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${HERE}/../.." && pwd)"
SRC_PATHS="morsehgp3D_v6/src morsehgp3D_v6/cli morsehgp3D_v6/CMakeLists.txt morsehgp3D_v6/bench"
PIN="$(git -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null || echo hors_depot)"
# shellcheck disable=SC2086
DIRTY="$(git -C "${REPO_ROOT}" status --porcelain -- ${SRC_PATHS} 2>/dev/null | wc -l)"

# --- Copie privee et sondes d'identite (sur la copie, jamais la source) ----
WORKD="${OUT}.partial"
mkdir -p "${WORKD}/out" "${WORKD}/bin" || refus "creation de ${WORKD} impossible"
BIN="${WORKD}/bin/mhgp6_profile_sonde"
cp "${BIN_SRC}" "${BIN}" && chmod 555 "${BIN}" || { rm -rf "${WORKD}"; refus "copie privee impossible"; }
H="$(hash_de "${BIN}")" || { rm -rf "${WORKD}"; refus "sha256 de la copie privee illisible (sha256sum muet ou hors grammaire 64-hex) — aucun hash vide n'est grave"; }
H_SRC="$(hash_de "${BIN_SRC}")" || { rm -rf "${WORKD}"; refus "sha256 de la source illisible (sha256sum muet ou hors grammaire 64-hex)"; }
[ "${H}" = "${H_SRC}" ] || { rm -rf "${WORKD}"; refus "copie privee (${H}) != source relue (${H_SRC}) — copie concurrente ou dechiree"; }
# Cible de sonde : accepte un nom d'ablation CONNU (un binaire produit refuse
# --inject= tout court, code 2) ET refuse un nom INCONNU (code 2 exact).
if ! "${BIN}" --family=uniform --n=64 --threads=1 --inject=ablation-mat-sans-tris >/dev/null 2>&1; then
  rm -rf "${WORKD}"
  refus "${BIN_SRC} n'accepte pas --inject=ablation-* (binaire produit, ou pas la cible mhgp6_profile_sonde)"
fi
rc=0
"${BIN}" --family=uniform --n=64 --threads=1 --inject=ablation-inconnue >/dev/null 2>&1 || rc=$?
if [ "${rc}" -ne 2 ]; then
  rm -rf "${WORKD}"
  refus "${BIN_SRC} ne refuse pas un --inject= inconnu par le code 2 (code ${rc}) : pas la cible mhgp6_profile_sonde"
fi

invalide() {
  echo "campagne INVALIDE : $1" >> "${WORKD}/META.txt"
  echo "INVALIDE : $1 — reçu laisse en ${WORKD}, jamais publie" >&2
  exit 3
}
H2="$(hash_de "${BIN}")" || invalide "sha256 de la copie privee illisible apres les sondes d'identite (sha256sum muet ou hors grammaire 64-hex)"
[ "${H2}" = "${H}" ] || invalide "copie privee alteree par les sondes d'identite (${H2} != ${H})"

# --- Plan equilibre grave AVANT le premier run ----------------------------
{
  echo "# plan equilibre : carre de Williams 4x4 (A=aucune B=ablation-mat-sans-copie C=ablation-mat-sans-tris D=ablation-post-cle-factice)"
  echo "# lignes : $(printf '%s | ' "${WILLIAMS[@]}")bloc b utilise la ligne ((b-1) mod 4) ; a l'interieur d'un bloc les tailles sont parcourues dans l'ordre n_list, et pour chaque taille les quatre bras dans l'ordre de la ligne"
  for rep in $(seq 1 "${REPS}"); do
    pos=0
    for lettre in ${WILLIAMS[$(((rep - 1) % 4))]}; do
      pos=$((pos + 1))
      echo "bloc=${rep} position=${pos} bras=$(bras_de "${lettre}")"
    done
  done
} > "${WORKD}/plan.txt"

cp "${HERE}/sonde_ablation_reduce.sh" "${WORKD}/protocole_lanceur.sh" || invalide "archivage du lanceur impossible"
cp "${HERE}/sonde_ablation_reduce.py" "${WORKD}/protocole_agregateur.py" || invalide "archivage de l'agregateur impossible"
H_LANCEUR="$(hash_de "${WORKD}/protocole_lanceur.sh")" || invalide "sha256 du lanceur archive illisible (hors grammaire 64-hex)"
H_AGREGATEUR="$(hash_de "${WORKD}/protocole_agregateur.py")" || invalide "sha256 de l'agregateur archive illisible (hors grammaire 64-hex)"
lscpu > "${WORKD}/lscpu.txt" 2>&1 || true

WT_DIFF="aucun"
if [ "${DIRTY}" -ne 0 ]; then
  # shellcheck disable=SC2086
  git -C "${REPO_ROOT}" diff HEAD -- ${SRC_PATHS} > "${WORKD}/worktree_diff.patch" 2>/dev/null || true
  {
    echo "# git status --porcelain (les fichiers non suivis '??' ne figurent pas dans le patch) :"
    # shellcheck disable=SC2086
    git -C "${REPO_ROOT}" status --porcelain -- ${SRC_PATHS} 2>/dev/null
    echo "# git diff HEAD --summary --stat (modes et volumes) :"
    # shellcheck disable=SC2086
    git -C "${REPO_ROOT}" diff HEAD --summary --stat -- ${SRC_PATHS} 2>/dev/null
  } > "${WORKD}/worktree_diff_summary.txt"
  WT_DIFF="worktree_diff.patch (git diff HEAD -- ${SRC_PATHS} ; resume : worktree_diff_summary.txt)"
fi

{
  echo "schema=e-hgp.sonde-ablation-reduce.v2"
  echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "commit=${PIN}"
  echo "worktree_sources_modifies=${DIRTY}"
  echo "worktree_diff=${WT_DIFF}"
  echo "binaire_source=${BIN_SRC}"
  echo "binaire_prive=bin/mhgp6_profile_sonde (copie immuable chmod 555, seule executee ; sha256 verifie avant et apres chaque tuple, HASHES.txt)"
  echo "binaire_sha256=${H}"
  echo "famille=${FAMILY}"
  echo "n_list=${N_LIST}"
  echo "reps=${REPS}"
  echo "parametres=threads=${THREADS} cpus=${CPUS} fold_inflight=2 fold_join=1 seed=3 s=8 smax=11"
  echo "ablations=${ABLATIONS}"
  echo "plan=williams_4x4 blocs=${REPS} (plan.txt grave avant le premier run ; appariement par bloc dans l'agregateur)"
  echo "etiquette_ablation-post-cle-factice=borne composite (lecture keys[] + tri de cles egales) — jamais « lecture seule »"
  echo "sha256_lanceur=${H_LANCEUR}"
  echo "sha256_agregateur=${H_AGREGATEUR}"
  echo "libstdcxx=$(readlink -f /usr/lib/x86_64-linux-gnu/libstdc++.so.6 2>/dev/null || echo inconnu)"
  echo "gcc=$(gcc -dumpfullversion 2>/dev/null || echo inconnu)"
  echo "hote=$(uname -srm)"
  echo "statut=exploratory_noncausal_upper_bounds (bornes exploratoires non causales sur binaire instrumente, join=1 : jamais un benchmark, jamais un mur, jamais un choix de palier)"
} > "${WORKD}/META.txt"
: > "${WORKD}/HASHES.txt"

run_one() { # $1 = ablation, $2 = n, $3 = bloc (repetition), $4 = position dans le bloc
  local abl="$1" n="$2" rep="$3" pos="$4"
  local tag="${abl}_n${n}_r${rep}"
  local inj=()
  [ "${abl}" = "aucune" ] || inj=("--inject=${abl}")
  local load_before load_after t0 t1 hb ha rc=0
  hb="$(hash_de "${BIN}")" || invalide "hash AVANT ${tag} illisible (sha256sum muet ou hors grammaire 64-hex) — jamais un hash vide"
  [ "${hb}" = "${H}" ] || invalide "hash AVANT ${tag} : ${hb} != ${H} (copie privee alteree)"
  load_before="$(cut -d' ' -f1-3 /proc/loadavg)"
  t0="$(date +%s.%N)"
  taskset -c "${CPUS}" "${BIN}" "--family=${FAMILY}" "--n=${n}" --s=8 --smax=11 --seed=3 \
    "--threads=${THREADS}" --fold-inflight=2 --fold-join=1 "${inj[@]}" \
    > "${WORKD}/out/${tag}.txt" 2> "${WORKD}/out/${tag}.err" || rc=$?
  t1="$(date +%s.%N)"
  load_after="$(cut -d' ' -f1-3 /proc/loadavg)"
  ha="$(hash_de "${BIN}")" || invalide "hash APRES ${tag} illisible (sha256sum muet ou hors grammaire 64-hex) — jamais un hash vide"
  {
    echo "ablation=${abl} n=${n} rep=${rep} position=${pos} code=${rc}"
    echo "commande=taskset -c ${CPUS} bin/mhgp6_profile_sonde --family=${FAMILY} --n=${n} --s=8 --smax=11 --seed=3 --threads=${THREADS} --fold-inflight=2 --fold-join=1 ${inj[*]:-}"
    echo "duree_s=$(awk -v a="${t0}" -v b="${t1}" 'BEGIN{printf "%.3f", b - a}')"
    echo "loadavg_avant=${load_before}"
    echo "loadavg_apres=${load_after}"
    echo "sha256_avant=${hb}"
    echo "sha256_apres=${ha}"
  } > "${WORKD}/out/${tag}.status"
  echo "${tag} avant=${hb} apres=${ha}" >> "${WORKD}/HASHES.txt"
  echo "--- fini ${tag} (bloc ${rep} position ${pos}, code=${rc}, $(sed -n 's/^duree_s=//p' "${WORKD}/out/${tag}.status")s)"
  [ "${ha}" = "${H}" ] || invalide "hash APRES ${tag} : ${ha} != ${H} (copie privee alteree pendant le tuple)"
  [ "${rc}" -eq 0 ] || invalide "run ${tag} en echec (code ${rc}) — matrice incomplete"
}

echo "sonde ablation reduce : ${FAMILY} n=${N_LIST} blocs=${REPS} (williams 4x4) bras=${ABLATIONS}"
NRUNS=0
for rep in $(seq 1 "${REPS}"); do
  for n in ${N_LIST}; do
    # La boucle EXECUTE le plan grave, elle ne le recalcule pas.
    while read -r l_bloc l_pos l_bras; do
      [ "${l_bloc}" = "bloc=${rep}" ] || invalide "plan.txt incoherent (${l_bloc} lu pour le bloc ${rep})"
      run_one "${l_bras#bras=}" "${n}" "${rep}" "${l_pos#position=}"
      NRUNS=$((NRUNS + 1))
    done < <(grep "^bloc=${rep} " "${WORKD}/plan.txt")
  done
done
{
  echo "runs_effectues=${NRUNS}"
  echo "runs_attendus=$((4 * NN * REPS))"
  echo "fin_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >> "${WORKD}/META.txt"
[ "${NRUNS}" -eq $((4 * NN * REPS)) ] || invalide "runs effectues ${NRUNS} != attendus $((4 * NN * REPS))"

# --- Agregation FATALE (copie archivee), puis manifeste = dernier fichier -----
python3 "${WORKD}/protocole_agregateur.py" "${WORKD}" > "${WORKD}/resume.txt" 2> "${WORKD}/resume.err" \
  || invalide "agregateur en echec (code $?, voir ${WORKD}/resume.err)"
[ -s "${WORKD}/resume.txt" ] || invalide "resume.txt vide"
SPECIAUX="$(cd "${WORKD}" && find . ! -type f ! -type d | wc -l)"
[ "${SPECIAUX}" -eq 0 ] || invalide "${SPECIAUX} entree(s) non reguliere(s) (lien ou special) dans le reçu"
( cd "${WORKD}" && set -o pipefail && lister_fichiers | xargs -d '\n' sha256sum > SHA256SUMS ) \
  || invalide "generation de SHA256SUMS en echec"
NFICHIERS="$(cd "${WORKD}" && lister_fichiers | wc -l)"
NSOMMES="$(wc -l < "${WORKD}/SHA256SUMS")"
[ "${NSOMMES}" -gt 0 ] && [ "${NFICHIERS}" -eq "${NSOMMES}" ] || invalide "manifeste incomplet (${NSOMMES} entrees pour ${NFICHIERS} fichiers)"
# Inventaire EXACT : les entrees du manifeste (ce qui a ete hache) doivent etre
# exactement l'ensemble attendu — tout fichier apparu entre l'agregateur et le
# manifeste (a la racine, dans out/, un out/SHA256SUMS...) est refuse ici.
inventaire_attendu() {
  local n rep abl ext
  printf '%s\n' HASHES.txt META.txt bin/mhgp6_profile_sonde lscpu.txt plan.txt \
    protocole_agregateur.py protocole_lanceur.sh resume.err resume.txt
  [ "${DIRTY}" -eq 0 ] || printf '%s\n' worktree_diff.patch worktree_diff_summary.txt
  for n in ${N_LIST}; do
    for rep in $(seq 1 "${REPS}"); do
      for abl in ${ABLATIONS}; do
        for ext in txt err status; do
          printf 'out/%s_n%s_r%s.%s\n' "${abl}" "${n}" "${rep}" "${ext}"
        done
      done
    done
  done
}
ECART="$(LC_ALL=C diff <(inventaire_attendu | LC_ALL=C sort) \
                       <(sed -n 's/^[0-9a-f]\{64\}  //p' "${WORKD}/SHA256SUMS" | LC_ALL=C sort) \
         | grep '^[<>]' | head -6 | tr '\n' ' ')"
[ -z "${ECART}" ] || invalide "inventaire du reçu != ensemble attendu (< attendu absent, > fichier inattendu hache) : ${ECART}"
( cd "${WORKD}" && sha256sum -c --quiet --strict SHA256SUMS ) >/dev/null 2>&1 \
  || invalide "verification finale sha256sum -c en echec"
mv "${WORKD}" "${OUT}" || invalide "publication (mv) impossible"
echo "reçu publie : ${OUT}"
cat "${OUT}/resume.txt"

#!/usr/bin/env bash
# MorseHGP3D v5 — RECU LOCAL d'une campagne de mesure CPU.
#
# Raison d'etre : trois campagnes locales successives ont ete REQUALIFIEES par
# l'audit pour la meme cause — sorties restees dans un scratch, sans commande,
# sans pin, sans hash de binaire, sans digest. Ce script rend cette faute
# impossible : il refuse de mesurer un arbre sale sur les chemins construits,
# il grave la commande exacte, le commit, le sha256 du binaire et les sorties
# brutes, et il compare les digests lui-meme.
#
# Il ne touche JAMAIS GCP : c'est une mesure locale, la machine est declaree
# telle quelle dans le recu. Un ecart de mur mesure ici sous ~20 % n'est pas
# concluant (machine partagee) ; les compteurs, eux, sont exacts.
#
# Usage :
#   bench/recu_local.sh <nom_campagne> --n=8000,16000 --familles=uniform,terrain \
#       --bras="off:--cover-envelope=0" --bras="on:--cover-envelope=1" [--repetitions=1]
#
# Chaque bras est `nom:flags`. Les bras sont joues en ordre ALTERNE (A B B A)
# pour qu'une derive de la machine ne se confonde pas avec un effet de bras.
set -euo pipefail

RACINE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$RACINE"

nom=""; ns="8000"; familles="uniform"; repetitions=1; cible="mhgp5"
# --entrees-differentes : les bras changent le NUAGE D'ENTREE (graine, plafond
# de canopee...) et non l'algorithme. La comparaison d'objet entre bras n'a
# alors aucun sens — une divergence est ATTENDUE, pas une faute. Toutes les
# autres gardes (pin propre, sha256 du binaire, commande gravee, sorties
# brutes, refus sur run vide) restent en vigueur.
entrees_differentes=0
declare -a bras_noms=() bras_flags=()
for arg in "$@"; do
  case "$arg" in
    --n=*)           ns="${arg#--n=}" ;;
    --familles=*)    familles="${arg#--familles=}" ;;
    --repetitions=*) repetitions="${arg#--repetitions=}" ;;
    --cible=*)       cible="${arg#--cible=}" ;;
    --entrees-differentes) entrees_differentes=1 ;;
    --bras=*)        v="${arg#--bras=}"; bras_noms+=("${v%%:*}"); bras_flags+=("${v#*:}") ;;
    --*)             echo "option inconnue : $arg" >&2; exit 2 ;;
    *)               nom="$arg" ;;
  esac
done
[ -n "$nom" ] || { echo "nom de campagne manquant" >&2; exit 2; }
[ "${#bras_noms[@]}" -ge 1 ] || { echo "au moins un --bras=nom:flags" >&2; exit 2; }
[[ "$nom" =~ ^[a-z0-9][a-z0-9_.-]{0,63}$ ]] || { echo "nom de campagne invalide" >&2; exit 2; }
[[ "$repetitions" =~ ^[1-9][0-9]*$ ]] && [ "$repetitions" -le 100 ] || {
  echo "repetitions hors domaine [1,100]" >&2; exit 2;
}
case "$cible" in
  mhgp5|mhgp5_cover_envelope_probe) ;;
  # Probe d'etages q4 : pas de digest (il ne construit pas la foret), donc la
  # comparaison d'objet du recu porte sur les COMPTEURS bruts. Le pin, le
  # sha256 du binaire et les sorties brutes restent exiges a l'identique.
  mhgp5_q4_stage_probe) ;;
  *) echo "cible non autorisee (produit attendu) : $cible" >&2; exit 2 ;;
esac
IFS=',' read -r -a n_values <<< "$ns"
IFS=',' read -r -a family_values <<< "$familles"
for n in "${n_values[@]}"; do
  [[ "$n" =~ ^[0-9]+$ ]] && [ "$n" -ge 2 ] || { echo "taille invalide : $n" >&2; exit 2; }
done
for fam in "${family_values[@]}"; do
  case "$fam" in
    uniform|terrain|scanline_single_pass|scanline_overlap_multiecho|eight_clusters|two_lines) ;;
    *) echo "famille invalide : $fam" >&2; exit 2 ;;
  esac
done
declare -A bras_vus=()
for i in "${!bras_noms[@]}"; do
  bn="${bras_noms[$i]}"; bf="${bras_flags[$i]}"
  [[ "$bn" =~ ^[a-z0-9][a-z0-9_.-]{0,31}$ ]] || { echo "nom de bras invalide : $bn" >&2; exit 2; }
  [ -z "${bras_vus[$bn]:-}" ] || { echo "nom de bras duplique : $bn" >&2; exit 2; }
  bras_vus[$bn]=1
  [[ "$bf" != *$'\n'* && ( -z "$bf" || "$bf" == --* ) ]] || { echo "flags de bras invalides : $bn" >&2; exit 2; }
done

# --- PIN : refus sur arbre sale pour les chemins qui entrent dans le binaire.
chemins="morsehgp3D_v5/src morsehgp3D_v5/cli morsehgp3D_v5/CMakeLists.txt morsehgp3D_v5/cmake morsehgp3D_v5/bench/recu_local.sh"
sale="$(git status --porcelain -- $chemins || true)"
if [ -n "$sale" ]; then
  echo "REFUS : l'arbre est sale sur les chemins construits — un recu ancre a un" >&2
  echo "        worktree non commite n'est pas reproductible." >&2
  echo "$sale" >&2
  exit 2
fi
commit="$(git rev-parse HEAD)"

# --- BUILD de la cible PRODUIT (jamais une cible de test).
build="$RACINE/build/recu_$nom"
cmake -S morsehgp3D_v5 -B "$build" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$build" --parallel --target "$cible" > /dev/null
bin="$build/$cible"
[ -x "$bin" ] || { echo "cible $cible introuvable dans $build" >&2; exit 2; }
bin_sha="$(sha256sum "$bin" | cut -d' ' -f1)"
compilo="$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "$build/CMakeCache.txt" | head -1)"
compilo="$compilo ($("$compilo" --version 2>/dev/null | head -1))"

sortie="$RACINE/morsehgp3D_v5/receipts/$nom"
[ ! -e "$sortie" ] || { echo "REFUS : destination de recu deja existante : $sortie" >&2; exit 2; }
mkdir "$sortie"
mkdir "$sortie/out"
: > "$sortie/session.log"

# --- MATRICE, en ordre alterne.
declare -a lignes=() ordre_joue=()
runs_non_nuls=0
# DENT 1 : une interruption doit laisser un statut TERMINAL explicite, jamais un
# repertoire de recu muet qu'un lecteur prendrait pour une campagne complete.
statut_terminal() {
  local code=$?
  if [ ! -s "$sortie/RECU.txt" ]; then
    { echo "# MorseHGP3D v5 — campagne locale : $nom"; echo "statut=interrompu"; echo "code_sortie=$code";
      echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%S+00:00)"; echo "source_commit=$commit";
      echo "note=campagne interrompue avant ecriture du recu — AUCUNE conclusion n'en sort"; } > "$sortie/RECU.txt"
  fi
}
trap statut_terminal EXIT INT TERM
for n in "${n_values[@]}"; do
  for fam in "${family_values[@]}"; do
    for r in $(seq 1 "$repetitions"); do
      # ordre A B au tour impair, B A au tour pair (AB/BA)
      idx=($(seq 0 $((${#bras_noms[@]} - 1))))
      if [ $((r % 2)) -eq 0 ]; then
        rev=(); for ((i=${#idx[@]}-1; i>=0; i--)); do rev+=("${idx[$i]}"); done; idx=("${rev[@]}")
      fi
      for i in "${idx[@]}"; do
        bn="${bras_noms[$i]}"; bf="${bras_flags[$i]}"
        read -r -a run_flags <<< "$bf"
        run="${fam}_n${n}_${bn}_r${r}"
        declare -a digest_flag=()
        [ "$cible" = "mhgp5_q4_stage_probe" ] || digest_flag=(--digest)
        cmd="$bin --family=$fam --n=$n ${run_flags[*]} ${digest_flag[*]}"
        echo "\$ $cmd" >> "$sortie/session.log"
        t0="${EPOCHREALTIME/,/.}"
        set +e
        "$bin" --family="$fam" --n="$n" "${run_flags[@]}" "${digest_flag[@]}" > "$sortie/out/$run.txt" 2> "$sortie/out/$run.err"
        code=$?
        set -e
        t1="${EPOCHREALTIME/,/.}"
        duree="$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.3f", b-a}')"
        # RSS : le CLI publie lui-meme `rss_max_kb=` ; pas de dependance a
        # /usr/bin/time, absent de ce conteneur (il rendait un code 127 muet).
        rss="$(sed -n 's/^rss_max_kb=\([0-9]*\).*/\1/p' "$sortie/out/$run.txt" | tail -1)"
        [ -n "$rss" ] || rss=0
        lignes+=("$run $code $duree $rss")
        [ "$code" -eq 0 ] || runs_non_nuls=$((runs_non_nuls + 1))
        ordre_joue+=("$run")
        echo "  code=$code duree=${duree}s rss=${rss}kb" >> "$sortie/session.log"
      done
    done
  done
done

# --- SIGNATURE DE L'OBJET : catalogue, forets et cardinalites, par run.
vides=0
for f in "$sortie/out"/*.txt; do
  lignes_objet="$(grep -E "^famille=|^cardinalites |^digest_balls=|^digest_forest_K|^digest_all=|^masses_q4 |^seeds_q4 " "$f" || true)"
  if [ -z "$lignes_objet" ]; then
    echo "VIDE" > "${f%.txt}.objet"; vides=$((vides + 1))
  else
    printf '%s\n' "$lignes_objet" | sha256sum | cut -d' ' -f1 > "${f%.txt}.objet"
  fi
done

# Comparaison AUTORITAIRE des bras pour chaque (famille, n, repetition).
desaccords=0
for n in "${n_values[@]}"; do
  for fam in "${family_values[@]}"; do
    for r in $(seq 1 "$repetitions"); do
      [ "$entrees_differentes" -eq 1 ] && continue
      reference="$sortie/out/${fam}_n${n}_${bras_noms[0]}_r${r}.objet"
      ref_sig="$(cat "$reference")"
      for bn in "${bras_noms[@]:1}"; do
        candidate="$sortie/out/${fam}_n${n}_${bn}_r${r}.objet"
        if [ "$(cat "$candidate")" != "$ref_sig" ]; then
          echo "DESACCORD objet : ${fam} n=${n} repetition=${r}, ${bras_noms[0]} != ${bn}" >> "$sortie/session.log"
          desaccords=$((desaccords + 1))
        fi
      done
    done
  done
done

# --- STATUT TERMINAL (dent 1) : un run non nul invalide la campagne, meme s'il
# a imprime des lignes d'objet.
statut=complete
[ "$vides" -eq 0 ] || statut=invalid
[ "$desaccords" -eq 0 ] || statut=failed
[ "$runs_non_nuls" -eq 0 ] || statut=failed

# --- RECU.
{
  echo "# MorseHGP3D v5 — campagne locale : $nom"
  echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%S+00:00)"
  echo "source_commit=$commit"
  echo "arbre_construit=propre (refus sinon)"
  echo "cible=$cible (cible PRODUIT, pas une cible MHGP5_TESTING)"
  echo "binaire_sha256=$bin_sha"
  echo "machine=$(uname -sr), $(nproc) fils logiques — MACHINE PARTAGEE, LOCALE"
  echo "avertissement=un ecart de mur sous ~20 % n'est pas concluant ici ; les compteurs sont exacts"
  if [ "${#bras_noms[@]}" -ge 2 ] && [ "$repetitions" -ge 2 ]; then
    echo "protocole=morsehgp3D_v5/bench/recu_local.sh, bras alternes AB/BA"
  else
    echo "protocole=morsehgp3D_v5/bench/recu_local.sh, ordre joue SANS alternance (un seul bras ou une seule repetition)"
  fi
  # DENT 3 : compilateur et configuration graves, sinon le pin ne suffit pas.
  echo "compilateur=$compilo"
  echo "cmake_build_type=Release"
  echo "cmake_version=$(cmake --version | head -1)"
  echo "statut=$statut"
  echo "runs_non_nuls=$runs_non_nuls"
  echo
  echo "# bras"
  for i in "${!bras_noms[@]}"; do echo "${bras_noms[$i]} = ${bras_flags[$i]}"; done
  echo
  if [ "$entrees_differentes" -eq 1 ]; then
    echo "comparaison_objet=sans_objet (les bras changent l'entree, pas l'algorithme)"
  else
    echo "comparaison_objet=$([ "$desaccords" -eq 0 ] && echo identique || echo DESACCORD)"
  fi
  echo
  echo "# signature de l'objet (sha256 catalogue + forets + cardinalites par K)"
  for f in "$sortie/out"/*.objet; do echo "$(basename "${f%.objet}") $(cat "$f")"; done
  echo
  echo "# run code duree_s peak_rss_kb"
  printf '%s\n' "${lignes[@]}" | sort
} > "$sortie/RECU.txt"

echo "recu ecrit : morsehgp3D_v5/receipts/$nom/RECU.txt"
echo "runs : ${#lignes[@]}"
if [ "$vides" -gt 0 ]; then
  echo "REFUS : $vides run(s) sans ligne d'objet — recu non concluant" >&2
  exit 3
fi
if [ "$desaccords" -gt 0 ]; then
  echo "REFUS : $desaccords comparaison(s) d'objet divergente(s)" >&2
  exit 3
fi
if [ "$runs_non_nuls" -gt 0 ]; then
  echo "REFUS : $runs_non_nuls run(s) de code non nul — statut=failed" >&2
  exit 3
fi

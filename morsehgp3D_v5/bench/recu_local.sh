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
declare -a bras_noms=() bras_flags=()
for arg in "$@"; do
  case "$arg" in
    --n=*)           ns="${arg#--n=}" ;;
    --familles=*)    familles="${arg#--familles=}" ;;
    --repetitions=*) repetitions="${arg#--repetitions=}" ;;
    --cible=*)       cible="${arg#--cible=}" ;;
    --bras=*)        v="${arg#--bras=}"; bras_noms+=("${v%%:*}"); bras_flags+=("${v#*:}") ;;
    --*)             echo "option inconnue : $arg" >&2; exit 2 ;;
    *)               nom="$arg" ;;
  esac
done
[ -n "$nom" ] || { echo "nom de campagne manquant" >&2; exit 2; }
[ "${#bras_noms[@]}" -ge 1 ] || { echo "au moins un --bras=nom:flags" >&2; exit 2; }

# --- PIN : refus sur arbre sale pour les chemins qui entrent dans le binaire.
chemins="morsehgp3D_v5/src morsehgp3D_v5/cli morsehgp3D_v5/CMakeLists.txt morsehgp3D_v5/cmake"
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

sortie="$RACINE/morsehgp3D_v5/receipts/$nom"
rm -rf "$sortie/out"          # jamais de sortie rance dans un recu
mkdir -p "$sortie/out"
: > "$sortie/session.log"

# --- MATRICE, en ordre alterne.
declare -a lignes=()
for n in ${ns//,/ }; do
  for fam in ${familles//,/ }; do
    for r in $(seq 1 "$repetitions"); do
      # ordre A B au tour impair, B A au tour pair (AB/BA)
      idx=($(seq 0 $((${#bras_noms[@]} - 1))))
      if [ $((r % 2)) -eq 0 ]; then
        rev=(); for ((i=${#idx[@]}-1; i>=0; i--)); do rev+=("${idx[$i]}"); done; idx=("${rev[@]}")
      fi
      for i in "${idx[@]}"; do
        bn="${bras_noms[$i]}"; bf="${bras_flags[$i]}"
        run="${fam}_n${n}_${bn}_r${r}"
        cmd="$bin --family=$fam --n=$n $bf"
        echo "\$ $cmd" >> "$sortie/session.log"
        t0=$(date +%s)
        set +e
        "$bin" --family="$fam" --n="$n" $bf > "$sortie/out/$run.txt" 2> "$sortie/out/$run.err"
        code=$?
        set -e
        t1=$(date +%s)
        # RSS : le CLI publie lui-meme `rss_max_kb=` ; pas de dependance a
        # /usr/bin/time, absent de ce conteneur (il rendait un code 127 muet).
        rss="$(sed -n 's/^rss_max_kb=\([0-9]*\).*/\1/p' "$sortie/out/$run.txt" | tail -1)"
        [ -n "$rss" ] || rss=0
        lignes+=("$run $code $((t1 - t0)) $rss")
        echo "  code=$code duree=$((t1 - t0))s rss=${rss}kb" >> "$sortie/session.log"
      done
    done
  done
done

# --- SIGNATURE DE L'OBJET : lignes agregees + cardinalites par K, par run.
vides=0
for f in "$sortie/out"/*.txt; do
  lignes_objet="$(grep -E "^famille=|^cardinalites " "$f" || true)"
  if [ -z "$lignes_objet" ]; then
    echo "VIDE" > "${f%.txt}.objet"; vides=$((vides + 1))
  else
    printf '%s\n' "$lignes_objet" | sha256sum | cut -d' ' -f1 > "${f%.txt}.objet"
  fi
done

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
  echo "protocole=morsehgp3D_v5/bench/recu_local.sh, bras alternes AB/BA"
  echo
  echo "# bras"
  for i in "${!bras_noms[@]}"; do echo "${bras_noms[$i]} = ${bras_flags[$i]}"; done
  echo
  echo "# signature de l'objet (sha256 de famille= + cardinalites par K)"
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

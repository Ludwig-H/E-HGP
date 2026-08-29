#!/usr/bin/env bash
# MorseHGP3D v5 — AUTO-FIXTURE de `recu_local.sh` (dent 3 de l'audit du 29 aout).
#
# Un harnais dont les refus ne sont pas testes est un harnais qui SEMBLE refuser.
# Cette fixture tue les quatre refus exiges : pin sale, destination existante,
# run de code non nul, digest divergent entre bras. Elle travaille dans un clone
# JETABLE : elle ne touche jamais le depot de travail, et n'ecrit aucun recu
# dans `morsehgp3D_v5/receipts/` du depot reel.
#
# Usage : bench/recu_local_selftest.sh [<repertoire de travail>]
# Codes : 0 les quatre refus sont tues ; 4 un refus attendu n'a pas eu lieu.
set -uo pipefail

RACINE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TRAVAIL="${1:-$(mktemp -d)}"
CLONE="$TRAVAIL/clone"
echec=0

attendu() {  # attendu <code> <intitule> -- <commande...>
  local code_attendu="$1" intitule="$2"; shift 3
  "$@" > "$TRAVAIL/sortie.txt" 2>&1
  local code=$?
  if [ "$code" -eq "$code_attendu" ]; then
    printf 'OK   %-42s code=%d\n' "$intitule" "$code"
  else
    printf 'ECHEC %-41s attendu=%d obtenu=%d\n' "$intitule" "$code_attendu" "$code"
    sed -n '1,4p' "$TRAVAIL/sortie.txt" | sed 's/^/       /'
    echec=$((echec + 1))
  fi
}

echo "auto-fixture recu_local : clone jetable dans $CLONE"
rm -rf "$CLONE"
git clone -q --local --no-hardlinks "$RACINE" "$CLONE" || { echo "clone impossible" >&2; exit 2; }
H="$CLONE/morsehgp3D_v5/bench/recu_local.sh"
# Le clone porte le HEAD ; on y injecte la version DE TRAVAIL du harnais, qui est
# le sujet du test, puis on la commite pour que le pin du clone soit propre.
cp "$RACINE/morsehgp3D_v5/bench/recu_local.sh" "$H"
git -C "$CLONE" add morsehgp3D_v5/bench/recu_local.sh
git -C "$CLONE" -c user.email=selftest@local -c user.name=selftest commit -qm "harnais sous test" || true

BRAS_OK='produit:--threads=1'

# 1. PIN SALE : une source modifiee doit interdire toute mesure.
echo '// sale' >> "$CLONE/morsehgp3D_v5/src/core/types.hpp"
attendu 2 "refus sur pin sale" -- "$H" f_pin_sale --n=400 --familles=uniform --bras="$BRAS_OK"
git -C "$CLONE" checkout -- morsehgp3D_v5/src/core/types.hpp

# 2. DESTINATION EXISTANTE : jamais ecraser ni completer un recu.
mkdir -p "$CLONE/morsehgp3D_v5/receipts/f_destination"
attendu 2 "refus sur destination existante" -- "$H" f_destination --n=400 --familles=uniform --bras="$BRAS_OK"

# 3. RUN DE CODE NON NUL : un refus du binaire invalide la campagne.
#    `--smax=99` depasse K_max <= 10 : le pipeline REFUSE avant calcul.
attendu 3 "refus sur run de code non nul" -- "$H" f_run_non_nul --n=400 --familles=uniform --bras='produit:--threads=1 --smax=99'

# 4. DIGEST DIVERGENT : deux bras qui ne calculent pas le meme objet.
#    `--smax=7` change l'objet ; les signatures doivent diverger.
attendu 3 "refus sur digest divergent" -- "$H" f_digest --n=400 --familles=uniform \
    --bras='a:--threads=1' --bras='b:--threads=1 --smax=7'

# 5. CONTROLE POSITIF : une campagne saine doit passer, sinon les refus
#    ci-dessus pourraient venir d'une panne generale et non du garde.
attendu 0 "campagne saine acceptee" -- "$H" f_saine --n=400 --familles=uniform --bras="$BRAS_OK"

echo
if [ "$echec" -eq 0 ]; then
  echo "auto-fixture : les quatre refus sont tues, le controle positif passe"
  exit 0
fi
echo "auto-fixture : $echec cas non conformes"
exit 4

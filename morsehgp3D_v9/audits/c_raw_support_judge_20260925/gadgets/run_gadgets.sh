#!/usr/bin/env bash
# Auditeur C, 25 septembre 2026 : gadgets des deux revues adverses du juge, codes attendus en v3.
# Usage : run_gadgets.sh <binaire du juge> <dossier de sortie>   (depuis ce dossier)
set -u
J=$1; OUT=$2; mkdir -p "$OUT"; bad=0
chk() {  # nom attendu args...
  local name=$1 want=$2; shift 2
  "$J" "$@" > "$OUT/$name.txt" 2>&1; local rc=$?
  echo "$name rc=$rc want=$want" | tee -a "$OUT/codes.txt"; [ "$rc" = "$want" ] || bad=1
}
: > "$OUT/codes.txt"
chk over12_k3_K3   0 file over12_k3.u32le 3 2 --unpinned     # coquille 13 hors fenetre : plus de faux SHELL_OVER_12
chk mine_over12_K3 3 file mine_over12.u32le 3 2 --unpinned   # 30 boules etendues trouvees ; VACUOUS q3crit_long
chk dom24_K3       0 file dom24.u32le 3 2 --seed=5 --unpinned   # coquille 24, q_min 3, non admissible
chk dom26_K3       0 file dom26.u32le 3 2 --seed=5 --unpinned   # coquille 26, non admissible : plus de faux SHELL_DOMAIN
chk dom26_K4       2 file dom26.u32le 4 2 --seed=5 --unpinned   # admissible : SHELL_OVER_12 puis refus chain_shell_above_12
chk domcap_K3      0 file domcap.u32le 3 2 --unpinned           # coquille 26 sans paire antipodale, p = 2
chk g28b_p2_K3     0 file g28b_p2.u32le 3 2 --seed=7 --unpinned # coquille 28, non admissible
chk g28b_p2_K4     2 file g28b_p2.u32le 4 2 --seed=7 --unpinned # admissible : SHELL_OVER_12 puis refus de la chaine
chk dedup1500_K5   0 file dedup1500.u32le 5 2 --unpinned        # ancres en collision : grille completee a 32
grep -q 'count=32' "$OUT/dedup1500_K5.txt" || { echo "dedup1500 grid not 32" | tee -a "$OUT/codes.txt"; bad=1; }
exit $bad

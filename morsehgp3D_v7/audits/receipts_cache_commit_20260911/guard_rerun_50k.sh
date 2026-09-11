#!/usr/bin/env bash
# Attend que les consommateurs mémoire lourds (CTest, rejeux, sondes front/corpus)
# soient terminés et que la mémoire libre dépasse 16 Gio, puis exécute l'observateur
# des quatre blocs nommés SEUL et enregistre son vrai résultat en _r2. Détaché.
set -u
A=/workspaces/E-HGP/morsehgp3D_v7/audits/receipts_cache_commit_20260911
W=/workspaces/E-HGP/build/v7-audit-20260911/named_blocks
log() { echo "[guard $(date -u +%H:%M:%S)] $*"; }
for i in $(seq 1 360); do
  busy=0
  for pat in 'bin/ctest' '/ctest ' 'rejouer_commit.sh' 'rejouer_front.sh' 'tower_corpus.py' 'witness_front_scale_probe' 'mhgp7_conformity' 'mhgp7_fold_csr' 'mhgp7_prefix' 'probe_family'; do
    if pgrep -f "$pat" >/dev/null 2>&1; then busy=1; break; fi
  done
  freem=$(free -m | awk '/^Mem:/{print $7}')
  if [ "$busy" -eq 0 ] && [ "$freem" -ge 16000 ]; then
    log "machine quiet (free ${freem} Mio) ; lancement observateur 50k seul"
    cd /workspaces/E-HGP
    python3 "$A/record.py" "$A/blocs_50k" pinned_50000_threads8_o2_r2 0 -- "$W/observer_o2" --pinned-50000 --threads=8
    rc=$?
    log "observateur terminé rc=$rc"
    exit $rc
  fi
  log "attente (busy=$busy free=${freem} Mio)"
  sleep 30
done
log "abandon : machine jamais quiet après 3 h"
exit 3

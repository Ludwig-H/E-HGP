#!/usr/bin/env bash
# Re-mesure P1 avec les sondes du verificateur (vrais SIGTERM) : r1 contre final, Python 3.12 puis 3.10.
# Attend la fin des campagnes de mutants (sentinelles) pour mesurer sur une machine moins chargee.
set -u
B=/tmp/mhgp10-r2/bancs
OUT=$B/preuves/g_vrais_signaux
until [ -f $B/preuves/a_signal/MUTANTS_FINI ] && [ -f $B/preuves/b_completude/MUTANTS_FINI ] && [ -f $B/preuves/d_differentiels/FINI ]; do sleep 10; done
uptime > $OUT/charge_debut.txt
PROBE=$B/sondes_verif/double_signal.py
PY312=/home/codespace/.python/current/bin/python3
PY310=/opt/conda/pkgs/python-3.10.21-h267e890_0_cpython/bin/python3.10
for py in $PY312 $PY310; do
  tag=$(basename $py)
  for who in r1 final; do
    if [ $who = r1 ]; then S=$B/src-r1/morsehgp3D_v10/bench/scaling/scale_run.py; else S=$B/gel/scale_run.py; fi
    for gap in $(seq 48 3 75); do
      W=/tmp/mhgp10-r2/bancs/travail_signaux/$tag/$who/bande_$gap; rm -rf $W
      timeout 900 $py -B $PROBE $S $W direct2 10 $gap > $OUT/${tag}_${who}_bande_gap$gap.jsonl 2>&1
    done
    W=/tmp/mhgp10-r2/bancs/travail_signaux/$tag/$who/rafale; rm -rf $W
    timeout 900 $py -B $PROBE $S $W rafale 32 20 > $OUT/${tag}_${who}_rafale_gap20.jsonl 2>&1
  done
  W=/tmp/mhgp10-r2/bancs/travail_signaux/$tag/final/groupe; rm -rf $W
  timeout 1800 $py -B $PROBE $B/gel/scale_run.py $W groupe 60 > $OUT/${tag}_final_groupe.jsonl 2>&1
  for who in r1 final; do
    if [ $who = r1 ]; then S=$B/src-r1/morsehgp3D_v10/bench/scaling/scale_run.py; else S=$B/gel/scale_run.py; fi
    timeout 600 $py -B $B/sondes_verif/signal_dans_close_group.py $S > $OUT/${tag}_${who}_simulation_close_group.txt 2>&1
    echo "code=$?" >> $OUT/${tag}_${who}_simulation_close_group.txt
  done
done
uptime > $OUT/charge_fin.txt
date -u +%FT%TZ > $OUT/FINI

#!/bin/bash
# Cas limites de la tour v10 (binaire Release du HEAD afb081774) : cout de l'etage local sur coquilles cospheriques et
# grilles, et plantages des CLI. Sorties : une ligne par cas.
cd /tmp/v11-audit/l06_code_tour/edge
B=/tmp/v11-audit/l06_code_tour/build-release
run(){ local f=$1 k=$2 to=$3; local t0=$(date +%s.%N); timeout $to $B/mhgp10_tower $f.u32le --k=$k --threads=2 --no-points > out_${f}_k$k.json 2>/dev/null; local rc=$?; local t1=$(date +%s.%N); python3 - "$f" "$k" "$rc" "$t0" "$t1" out_${f}_k$k.json <<'PY'
import json,sys
f,k,rc,t0,t1,p=sys.argv[1:7]
try:
    j=json.loads(open(p).read().strip().splitlines()[-1])
    st=j.get('stages',{})
    print(f,'K',k,'code',rc,'mur %.2f s'%(float(t1)-float(t0)),'statut',j.get('status'),j.get('reason',''),'n',j.get('n'),'boules',j.get('balls'),'catalogue_s',j.get('catalogue_s'),'tour_s',j.get('tower_s'),'t_local',st.get('t_local'),'t_resolve',st.get('t_resolve'))
except Exception as e:
    print(f,'K',k,'code',rc,'mur %.2f s'%(float(t1)-float(t0)),'(pas de JSON : delai ou signal)')
PY
}
run grid6 5 60; run grid6 8 60; run grid6 10 120; run grid6 11 120; run grid6 12 120; run grid10 5 120
run sphere12 5 60; run sphere12 10 60; run sphere16 10 120; run sphere24 5 180; run circle65 5 60; run circle325 5 120
run line20 5 30; run plane_grid12 5 60; run n1 5 10; run n2 5 10; run n3 5 10
echo "--- plantages"
$B/mhgp10_tower n3.u32le --k=2 --threads=1 --no-points --dump=n3_nopoints.dump > n3_nopoints.json 2>/dev/null; echo "mhgp10_tower n3 --k=2 --no-points --dump : code $? ; octets ecrits sur stdout $(stat -c %s n3_nopoints.json) ; dump $(stat -c %s n3_nopoints.dump) octets"
$B/mhgp10_cluster n3.u32le out_n3.i32 --k=5 --mcs=2 --threads=1 > n3_cluster.json 2>/dev/null; echo "mhgp10_cluster n3 (3 points) --k=5 --mcs=2 : code $?"
$B/mhgp10_cluster n2.u32le out_n2.i32 --k=3 --mcs=2 --threads=1 > n2_cluster.json 2>/dev/null; echo "mhgp10_cluster n2 (2 points) --k=3 --mcs=2 : code $?"
$B/mhgp10_tower grid6.u32le --k=13 --threads=1 > k13.json 2>/dev/null; echo "mhgp10_tower grid6 --k=13 : code $? ; $(cut -c1-80 k13.json)"
rm -f core core.*

#!/bin/bash
# Differentiel final : catalogue 8b8d66f6e (assemblage sequentiel) contre le binaire final (assemblage parallele).
OLD=/workspaces/E-HGP/build/v10-bench-8b8d66f6e/build/mhgp10_catalogue
NEW=./mhgp10_catalogue.new3
IN=/workspaces/E-HGP/build/v10-scale-inputs
for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg syn_shells_density_x2 syn_filaments_space_x4; do
  for K in 5 10; do
    $OLD $IN/$f.u32le --k=$K --threads=8 --dump=o.dump > /dev/null
    $NEW $IN/$f.u32le --k=$K --threads=8 --dump=n8.dump > n8.json
    $NEW $IN/$f.u32le --k=$K --threads=3 --dump=n3.dump > /dev/null
    a=$(sha256sum o.dump | cut -c1-16); b=$(sha256sum n8.dump | cut -c1-16); c=$(sha256sum n3.dump | cut -c1-16)
    if [ "$a" = "$b" ] && [ "$b" = "$c" ]; then r=IDENTIQUES; else r=DIFFERENTS; fi
    echo "$f K=$K ancien=$a nouveau8=$b nouveau3=$c $r"
    rm -f o.dump n8.dump n3.dump
  done
done
echo FIN

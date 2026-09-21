# Trois mutations compilées du raccord global q3/q4

Capture close : [compiled_5slu1ek1](compiled_5slu1ek1/COMPLETION.json).
Le produit, la porte et le build Release d'origine n'ont pas été modifiés.
Les copies compilées sont isolées dans un répertoire temporaire ; sources
originales, sources modifiées, patches, commandes et sorties brutes sont
conservés dans la capture. Il s'agit de trois altérations du produit jugées
par une porte indépendante, pas de trois oracles indépendants.

Le baseline passe11 440 contrôles,166 appels mono/globaux et46 appels
parallèles. Les trois mutations sont tuées au premier essai :

| Mutation | Conséquence vérifiée |
| --- | --- |
| q3 utilise puissance≤0 comme intérieur | Des contacts de coquille deviennent à tort des crédits stricts. |
| q4 exige une émission q3 sur son arête lorsque q3 est active | Une boule q4 valide disparaît malgré l'indépendance des deux census. |
| En masque6, q4 est supprimée lorsque le front a rejeté q3 | Un rejet propre à q3 détruit une sortie q4 qui doit survivre. |

Chaque mutant sort1 avec exactement le diagnostic géométrique
`global q34 stream differs from independent rational support/depth/shell oracle`,
avant les contrôles du ledger. Les compilations et éditions de liens passent.
Un crash, un échec de compilation ou un simple compteur incorrect n'est pas
accepté comme preuve causale par le runner.

La fermeture confirme196 sources et4 artefacts du build inchangés, ainsi
que les hashes du helper et du compilateur. Le
[readback persistant](MUTANTS_READBACK.json) reprend les quatre lectures
normal/−O, historique/live : quatre succès,229 entrées inchangées avant/après,
aucune erreur de fermeture. `readback.py` est un auxiliaire de reçu extérieur
aux196 sources, et son propre hash fait partie de ces229 entrées.

Reproduction, dans un build Release neuf ou le build d'origine non modifié :

```sh
python3 -B morsehgp3D_v8/tests/wspd_q34_mutations.py run --build build/v8_lidar_global_20260921
python3 -B morsehgp3D_v8/tests/wspd_q34_mutations.py read morsehgp3D_v8/receipts/lidar_global_20260921/mutations/compiled_5slu1ek1 --check-live
python3 -B -O morsehgp3D_v8/tests/wspd_q34_mutations.py read morsehgp3D_v8/receipts/lidar_global_20260921/mutations/compiled_5slu1ek1
```

Ces contrôles ne qualifient ni FULL, ni le contrat50k, ni un GPU.
GCP non utilisé pour cette capture.

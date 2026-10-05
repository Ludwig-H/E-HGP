# S10 — correction locale de la borne signée

Source : `076d9142b724623d4ee4c6deebd125781fa8ba72` + correctif WIP capturé
deux fois, octets stables, à 19:50:43 UTC. `SOURCE.json` épingle les quatre
fichiers modifiés ; `wip.patch` les restitue depuis ce pin. Le constat du
reçu `audit_s10_wip_20261005` reste applicable aux trois blobs causaux de la
publication `076d9142b`, identiques à ses empreintes. Ce supplément juge la
correction locale, sans qualification native ni publication de son code.

La garde de `score.cpp` précède toutes les conversions signées et contrôle
chacune des racines utilisées : au-delà de `L=2^100`, le plateau devient
`open` et le calcul passe au repli exact si une comparaison en a besoin.
Les valeurs admises ont `rt+rm-rq` dans `[-L,2L]`, donc les bornes
`sum-1` et `sum+2` dans `[-L-1,2L+2]`, largement dans `i128` ; les
intermédiaires et `rt+1` y tiennent également. Le témoin original
`R=2^127-1` est maintenant détourné avant sa conversion. Aucune restriction
du contrat abstrait n'est introduite : l'absence de filtre laisse le signe
exact décider ou refuser selon son budget déclaré.

Le nouveau test natif `huge` a des racines fixes
`2^126`, `3·2^125`, `3·2^126`. Ses deux premières additions `R+1` étaient
déjà sûres. Sous conversion modulaire sur 128 bits, la dernière racine
devient `-2^126`, puis passe déjà en `open`, sans l'addition qui débordait
dans le témoin initial. Son assertion `unbracketed>=1` ne distingue donc pas
à elle seule l'ancien filtre. Les labels et l'égalité de cet arbre restent
cohérents, mais ne pas attribuer à cette porte la causalité d'une détection
de l'ancien débordement. Le témoin exact `R=2^127-1`, avec ses quatre sites
et ses deux racines, peut servir de porte UBSan ciblée ; pas de campagne
supplémentaire proposée. Le manifeste WIP ajouté ensuite contient bien
`racine_non_bornee` (garde remplacée par `if(false)`, porte
`mhgp11_head_unit_huge`), mais ne change pas ce témoin ni son seuil de compte.
`MUTANT_SOURCE.json` et `head_mutants_wip.json` figent cette lecture tardive ;
aucun verdict de mutant exécuté n'est revendiqué.

`check.py` vérifie seulement les bornes entières, les huit coins de la somme
à trois racines et cette distinction de portée du test. Il n'exécute aucun
code natif ni la condensation. Les sources u24 géométriques restent sous la
nouvelle garde ; aucune autre anomalie mathématique n'a été trouvée dans ce
correctif. Les anciens tests de réciproques et d'EOM ne sont pas répétés.

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B replay.py --repo /workspaces/E-HGP
```

Le rejeu portable reconstruit les quatre sources par Git puis applique le
patch ; `check.py` vérifie les quatre empreintes. La consolidation a rejoué cette reconstruction en normal et `-O` :
codes 0 et sorties identiques au modèle capturé. Le dossier `snapshot/`
reste omis car reconstructible ; aucun compilateur ni binaire natif appelé.

Deux exécutions du modèle, normal et `-O` : codes 0, stderr vides et sorties
identiques. Aucun essai en échec ; le premier script est préservé sous
`attempts/initial_check.py`. `executions.json` conserve les commandes exactes.

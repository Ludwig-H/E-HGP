# Tour candidate mono : comparaison C/D du 5 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Deux exécutions terminées : **225,75 s pour C, 172,67 s pour D**, avec
les mêmes dix digests, cardinalités et compteurs de travail. La baisse
arithmétique observée est de **23,51 %**, sur une seule paire ordonnée
C puis D et un hôte partagé. Ce n'est ni un gain statistique qualifié,
ni une réussite du contrat 50k, ni une preuve d'exactitude HGP globale.
Les [reçus de build, protocole et paire](../receipts/meb_mono_20260905/README.md)
séparent les constructions, observations et relectures.

## Périmètre réellement mesuré

Nuage uniforme, n=8 000, `coord=65536`, seed=3, s=8, toute la tour
K=1..10 (`smax=11`), `--complete-incidences`, CSR et digest activé,
sans export d'archive. Le payload est
`normalized_horizontal_h0_candidate`, pas `verified_events_only`.
Les applications verticales et les poids ne sont pas fournis par ce run.

Les deux commandes demandent `threads=1`, `fold-inflight=1`, `fold-join=1`,
avec affinité sur le CPU logique 6. Les drapeaux et compteurs d'ouvriers
publiés sont vérifiés ; l'absence de création de threads est qualifiée
séparément par les [portes intégrées mono](../receipts/meb_lazy_integrated_20260905/README.md),
pas par l'affinité seule. Le CLI D de ces portes Release est identique
octet pour octet au CLI D de la mesure, construit séparément.

Machine observée : AMD EPYC 7763, environnement exposant huit CPU logiques,
33 657 716 736 octets de RAM et noyau Linux 6.8.0-1052-azure. L'hôte
est partagé. Aucun autre build ou test moteur de cette équipe ne tourne
pendant la paire ; les lectures et petits contrôles documentaires ne
sont pas une isolation de machine. Les 323 portes complètes sont lancées
après le terminal de cette paire, hors de ses chronos.

Plafond par processus : 600 s et RLIMIT_AS de 26 GiB, qui ne borne pas
directement le RSS physique. Le proxy nommé de payload vaut 16 GiB.
Les plafonds silencieux sont identiques : 8 M records de cœur, 2 M pas,
2 M cofaces ajoutées, 1 milliard de visites et de supports MEB par ordre.
Ils ne sont pas désactivés pour obtenir le résultat.

## Identités conservées

C est le binaire `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`,
lié à son ancien reçu Release C, pas aux sources courantes de D.
D est `127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`.
Le [delta relu](OPTIMISATION_MEB_DIFFEREE.md) diffère seulement la
matérialisation q3/q4 de la MEB locale, sans changer l'ordre des supports,
les caps, les représentations des niveaux ni les contrôles de coquille.

Les deux runs publient 4 384 229 événements et 26 434 998 facettes cumulées
sur les dix ordres. Les sommes ne sont pas des pics de résidence.
La complétion conserve exactement 802 125 328 supports MEB examinés,
581 904 257 visites d'index, 1 270 848 ajouts et une chaîne maximale de
longueur 18. L'égalité est vérifiée aussi par ordre, pas seulement sur
ces sommes. Le digest chaîné commun est
`4c3ceb0498990bafa41a9e43d0bffe25a3fee579b12b5d34365f3578f526a0e7`.

Les sources, helpers, binaires et reçus de build sont stables aux frontières
des deux exécutions. HEAD et les changements documentaires du worktree
sont enregistrés séparément. Ce n'est pas une attestation hermétique.

## Temps et résidence observés

| Mesure | C | D |
| --- | ---: | ---: |
| Processus complet, secondes | 225,747536 | 172,674571 |
| Pipeline, secondes | 225,7199 | 172,6230 |
| Génération, secondes | 56,8169 | 55,6927 |
| Préfiltre, secondes | 9,5617 | 9,3185 |
| Census, secondes | 7,6725 | 7,4454 |
| Complétion silencieuse, secondes | 116,615421 | 65,973963 |
| Fold mural A+B, secondes | 149,2804 | 97,8473 |
| Fold B cumulé, secondes | 29,1378 | 28,3638 |
| Digest, secondes | 3,5972 | 3,5914 |
| RSS maximal externe, KiB | 2 302 712 | 2 301 540 |

Ces lignes **ne sont pas toutes additives** : le fold mural contient
notamment la complétion et les opérations par ordre. La baisse observée
de la complétion est de 43,43 % ; ce chrono n'isole pas le seul noyau MEB.
Le digest reste inclus dans les coûts complets, sans soustraction cachée.
La paire se déroule de 00:06:26 à 00:13:05 UTC ; les durées de construction
et de qualification sont exclues des durées moteur et conservées ailleurs.

## Conclusions bornées et suite

Le changement diminue le coût des candidats rejetés, pas le nombre de
supports contractuellement examinés. Après D, la complétion et la
génération restent les deux plus gros postes de cette observation.
Le faible écart de RSS ne qualifie pas une optimisation mémoire.

Cette entrée étendue diffère de la grille `coord=200` de la
[comparaison s=8/10/12 précédente](RESULTATS_MONO_20260904.md), qui mesurait
l'objet Gabriel. Aucun gain n'est calculé entre ces deux campagnes et
aucun temps s=10 ou s=12 n'est inventé pour D complété. Le défaut s=8
reste inchangé ; la comparaison précédente ne départageait pas ces valeurs
avec une robustesse statistique suffisante.

Les contrats 50k sous une seconde, puis 100 ms, et plusieurs dizaines de
millions sur G4 restent ouverts. Aucun résultat D/GPU n'est déduit de
cette paire CPU locale. La tour complète industrielle requiert encore
ses certificats globaux, sa verticale, ses poids, le traitement des
dégénérescences non prises en charge et une résidence compatible avec
le massif. Aucun oracle exhaustif ni mosaïque globale n'a été introduit.

# Échelle locale de la phase statique CPU — 8k, 16k, 32k

11 septembre 2026. Trois exécutions closes, successives, d'un même binaire
CPU figé ; aucun nouveau moteur ni calcul GPU n'est produit par ce paquet.
À chaque taille, les **35 champs communs hors temps/cache/travail géométrique**
sont identiques au nominal historique, dont entrée, payload, parents,
verticale, contributions et compteurs de calendrier. Les appels MEB et
supports effectivement payés diminuent ; les chronométrages sur hôte partagé
ne permettent pas de revendiquer une accélération.

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Configuration : uniforme u16, seed 3,
coordonnées dans [0,65535], WSPD s=8, **toute la tour K=1..10**, un thread
amont et quatre workers statiques demandés. K1 reste sur le chemin nominal.

## Travail physique et mémoire

| n | MEB statique / nominal | Supports statique / nominal | RSS maximal (KiB) |
| --- | ---: | ---: | ---: |
| 8 000 | 4 185 184 / 6 227 265 | 364 590 166 / 573 011 617 | 2 137 980 |
| 16 000 | 8 779 465 / 13 252 780 | 767 853 710 / 1 227 441 406 | 4 428 148 |
| 32 000 | 18 244 853 / 27 711 509 | 1 600 773 173 / 2 577 959 005 | 9 109 204 |

| n | Demandes R | Facettes uniques U | Uniques résolues par semis | Capacité statique retenue (octets) |
| --- | ---: | ---: | ---: | ---: |
| 8 000 | 10 396 562 | 5 176 885 | 2 396 646 | 307 936 444 |
| 16 000 | 21 827 082 | 10 862 732 | 5 010 402 | 617 052 880 |
| 32 000 | 45 208 799 | 22 503 000 | 10 348 964 | 1 235 849 528 |

R/U/semis sont les sommes des neuf ordres statiques K2..10 ; la ligne K1
est intentionnellement nulle. L'identité MEB payées = anchor_hits +
intruder_queries est vérifiée. Les capacités sont un maximum échantillonné
des buffers simultanément retenus, **ni le pic transitoire d'allocation ni
le RSS**. Les RSS sont mesurés séparément par `/usr/bin/time -v` ; aucun gain
RSS n'est revendiqué.

| Doublement | MEB | Supports | Demandes | Uniques | Capacité statique | RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8k→16k | 2,09775 | 2,10607 | 2,09945 | 2,09831 | 2,00383 | 2,07118 |
| 16k→32k | 2,07813 | 2,08474 | 2,07123 | 2,07158 | 2,00283 | 2,05711 |

Ces observations sur trois nuages d'une seule famille ne prouvent **aucune
borne sous-quadratique universelle**, ni la faisabilité à plusieurs dizaines
de millions de points. Il n'y a pas de nouveau plafond algorithmique sur n
ou la durée. ROOT a lancé le 32k après fermeture du 16k ; les autres calculs
de l'hôte, dont s10/s12 à 8k, restent concurrents.

## Temps bruts, sans promotion

| n | Toute la chaîne, digest inclus (s) | Composant FULL (s) |
| --- | ---: | ---: |
| 8 000 | 466,761055 | 150,412324 |
| 16 000 | 686,048712 | 123,976271 |
| 32 000 | 802,278328 | 217,109716 |

Ce ne sont pas des speedups appariés : les charges CPU varient fortement,
et la référence a été mesurée un autre jour. Affinité, charge et mémoire de
l'hôte sont conservées avant/après, avec suivi de progression et stderr brut.
Les contrats 50k/1 s, puis 100 ms, et les dizaines de millions sur G4 ne sont
pas acquis ici. GCP non utilisé.

## Provenance et lecture autonome

Le binaire déjà compilé et fermé est épinglé à
`f71f31190f5d50ac70f8332f969c6baa50549536bd08836702e6ba01ae7fcdce` ;
le header statique CPU est
`33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209`.
Le snapshot complet consommé est copié une seule fois dans `source/` et
vérifié contre les pins avant/après des trois runs. Les fichiers GPU/oracle
éventuellement présents dans ce snapshot ne signifient pas qu'ils ont été
exécutés. Aucun ELF, dépendance Boost ou oracle nouveau n'est embarqué.
La commande de compilation historique est conservée, pas rejouée.

Chaque dossier `nN_s8_static4/` conserve sans réécriture stdout, stderr,
commande, intention, reçu de fermeture, pins, hôte, progression et la version
du recorder réellement exécutée. Le recorder 8k diffère de celui des deux
tailles suivantes uniquement par le contrôle de fermeture de son groupe de
processus en cas d'exception ou d'arrêt explicite ; binaire et arguments
restent identiques. Aucun timeout automatique n'est introduit.

Les références stdout sont copiées brutes du
[paquet nominal historique](../full_ball_scale_gpu_20260910/README.md),
manifeste `c9913094b6479a61a0395be2dc0f09ef2d25672f170e19d3c6e01c584ba08b2c`,
copié lui aussi et vérifié sans lire ce paquet voisin. Les listes exactes des
35 champs sont dans les trois `comparison.json`. Les exclusions sont
explicites dans le lecteur figé ; des compteurs non temporels de l'historique
inférieur sont même conservés, au-delà du seul payload.

La [confrontation indépendante R_K/U_K](initial_key_comparison/README.md)
est copiée **octet pour octet**, avec son lecteur et son manifeste
`9167657c29c3ffb36292f2b830475d537d47072831740e5bb0b7bb937caa2c77`.
Ses neuf couples K2..10 concordent avec la capture 8k jointe. C'est une
égalité des comptages, pas du flux intégral des clés statiques non exporté,
et cela ne constitue pas un nouvel oracle géométrique.

Le lecteur public vérifie l'inventaire, l'absence d'ELF, les sources et
références épinglées, les trois configurations et leur ordre chronologique ;
puis il exécute uniquement les deux lecteurs figés inclus. Il recalcule les
comparaisons, planchers et ratios et rattache le reçu R/U au stdout 8k exact.
`source_map.json` conserve le mapping vérifié des originaux, dont le Markdown
historique stocké comme `.source` ; `results.json` est reconstitué par lecture.
Aucun accès aux chemins absolus historiques, moteur, compilation, réseau ou
GCP n'est nécessaire pour ces commandes, depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/static_resolution_scale_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/static_resolution_scale_20260911/verify.py
```

La qualification mathématique et causale reste celle des paquets séparés
[CPU statique](../static_resolution_cpu_20260911/README.md),
[six mutants physiques](../static_resolver_mutants_20260911/README.md) et
[échec après admission d'un worker](../static_worker_failure_20260911/README.md).
Les comparaisons s=8/10/12 ont leur
[reçu distinct](../static_s_factors_20260911/README.md). Ce paquet d'échelle
n'ajoute aucun mutant C++, sanitizer, certificat de complétude WSPD ou
claim industriel FULL ; l'autorité demeure relative au census fourni.

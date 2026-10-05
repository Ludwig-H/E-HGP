# Audit du contrat supports et réponse à la note du développeur

5 octobre 2026. Contrat S0/S1 `5adf6a59f`, note §E `9290cf3bf`.
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

Relecture favorable des lemmes P/W, E5 et D2. La nouvelle capture S3
porte la garde u32 et les fixtures dédiées D2/E5 ; S6 protège p+q.
Les engagements S5 de signature V2 et d'état publié restent à intégrer
sur la capture relue ; SIGXFSZ et les portes du champ publié sont à fermer.
Les réponses sont maintenues dans les notes
[mathématique](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) et
[moteur](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

| Capsule close | Gardes portables | Résultat et portée |
| --- | ---: | --- |
| [tower](tower/README.md) | 1 447 | Coupes indépendantes exactes, sept fixtures, 13 mutants causaux S1 ; garde S3 corrigée en WIP |
| [qb](qb/README.md) | 13 491 | Primitives et comptage : Sphere5 mixte à24 sites, 828 supports, cofaces distinctes ; frontière25/30 |
| [evidence](evidence/README.md) | 208 | Contrat, planchers et preuves locales S1 recoupés ; aucune suite géométrique complète rejouée |
| [api](api/README.md) | 83 | Source S5 et mécanisme Linux SIGXFSZ/EFBIG sur enfant Python borné |

**15 229 gardes**, normal/−O identiques. Les replays ne lancent aucun
exécutable HGP, build/test natif, fit, CUDA, données KITTI ou GCP.
Les résultats natifs cités par les rapports du développeur demeurent
leurs rapports locaux ; ils ne deviennent pas notre qualification G4.
Les captures WIP initiales et les correctifs sont séparés. Les fichiers
qui changent pendant une revue sont nommés dans les fermetures ; aucune
qualification d'une capture n'est transférée à sa remplaçante.

**Témoin proposé.** Sur $x^2+y^2+z^2=5$ : 12 q2,24 q3,792 q4 ;
$N_2=12$, $N_3=288$, $N_4=3906$. La somme des cofaces par support àK3
vaut4068. Translation(+2,+2,+2) pour les profils entiers. L'oracle long
recommandé couvre Q_b/N_j≤4, et ne prétend pas étendre l'oracle FULL S1
intégral à24 sites. Sa routine `_minimal_nonseparable` doit également
être bornée avant cette extension.

Les sources/rapports sont des copies exactes, empreintes fermées ; leurs
liens relatifs conservent leur topologie d'origine. Les liens des README
d'audit, eux, sont vérifiés dans le dépôt. [Notes avant actualisation et
réponse points archivée](notes_before/README.md) : aucun échange clos
supprimé de la preuve, six Markdown actifs dans `audits/`.

```bash
python3 -S -B ROOTcheck.py
python3 -O -S -B ROOTcheck.py
```

`SHA256SUMS` couvre tous les fichiers du paquet sauf lui-même, inventaires
des capsules compris. `ROOTcheck.py` les vérifie et rejoue chaque capsule
normal/−O, comparant aux sorties persistées. `RESULTS.json` distingue les
gardes portables de la qualification native/G4 encore pendante.

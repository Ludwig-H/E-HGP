# G4 — adaptateur q34 résident, S2 uniquement

État : **completed**. Sources : `af369c44efa75236fa98e25e8f1bc4708b128fc4`.
Autorité LIVE : snapshot et originaux privés, sorties VM, résumé recalculé
et arrêt de la génération exacte. Aucune tour FULL n'est calculée ici.

Trame entière sans sol ng00, grille 1 mm/u18, 39885 sites, K5/s8.
Même résultat natif ordonné : P=23686751, E=9122704, S=2043612.
Une mesure W4 puis une mesure W48, dans deux processus. Les largeurs concernent
l'arène CPU seulement ; front W1 et référence W4 inchangés. Pas de répétition,
de gain stable, de borne de croissance ni de qualification multi-scènes.

| Étape | W4 (ms) | W48 (ms) |
|---|---:|---:|
| Lecture entrée | 0.786031 | 0.790550 |
| Index CPU | 12.497599 | 12.986350 |
| Front CPU W1 | 2111.371751 | 2112.022234 |
| Ouverture, filtre rectangle et compaction | 325.035122 | 319.929720 |
| Dont init CUDA | 161.392160 | 155.936425 |
| Dont noyau rectangle GPU | 82.298651 | 81.369397 |
| Construction arène CPU | 202.425061 | 83.112827 |
| Consommation GPU + tri CPU + libérations internes | 97.493874 | 98.509047 |
| Dont vagues + compteurs D2H | 34.303014 | 34.540880 |
| Dont transfert survivantes + croissance hôte | 20.401587 | 21.029079 |
| Dont tri/conversion CPU | 36.217384 | 36.354418 |
| Fermeture des propriétaires | 2.544304 | 2.771530 |
| Adaptateur S2 complet | 627.503421 | 504.327994 |
| Référence native W4 de contrôle | 10114.517347 | 10067.708077 |
| Destructions finales groupées | 2.455420 | 2.259310 |
| Harnais complet mesuré | 12892.161879 | 12723.081204 |
| Front + adaptateur | 2738.875172 | 2616.350228 |

Les sous-phases « dont » sont incluses dans leur phase extérieure.
L'adaptateur inclut l'initialisation CUDA et la libération de ses propriétaires
Prepared/Session/Decision et buffers propres. S et les R masques de sortie restent retenus après ce temps.
Aucune soustraction d'initialisation ni estimation « warm » n'est publiée ;
le contexte CUDA global est hors contrat de destruction.
Lecture, index et front amont sont explicitement hors adaptateur.
Le total du harnais comprend aussi oracle natif, comparaison et destructions finales :
ce n'est pas le coût du seul candidat. Aucun temps FULL ou contrat 100 ms acquis.

Gate : 85 cas, 255 appels portables et 194 appels CUDA du corpus.
portable_runs/cuda_runs comptent les appels one() du corpus, pas tous les appels device. queries/survivors et les compteurs accumules dans one() agregent portable + CUDA. pool_lane_reduced est calcule par oracle une fois par cas. ownership_gate ajoute un appel valide hors de ces compteurs de runs/travail ; rejections inclut aussi ses refus. CUDA utilise Q7/Q257 par cas, plus Q1 si E<=4.

La comparaison avec le GPU précédent reste non acquise : source, préparation et
périmètre temporel ont changé ; ce lecteur ne calcule aucun facteur d'accélération.

Allocation observée : 191.901 s ; aucun montant facturé estimé.

Relecture LIVE, normale puis avec `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/readback.py --readback CHEMIN_DU_RECU
```

Le snapshot et les originaux privés liés dans `PRIVATE_LINKS.json` restent nécessaires.
`SHA256SUMS` couvre toutes les pièces sauf lui-même. Ni archive, clé SSH,
nouvelle donnée KITTI, réponse OS Login brute ou état GCE non expurgé ne sont publiés.

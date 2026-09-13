# Préparation partagée et filtre axial — captures du 13 septembre 2026

Cadre `exploration_v8_hors_registre`, CPU référence mono, entrée u16,
`implementation_v8_p0`, `public_status=not_claimed`. GCP non utilisé.
Le [contrat](../../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md) décrit les objets,
preuves et limites ; le [protocole](../../bench/P0_PROBE.md) décrit les sondes.
La [qualification](QUALIFICATION.json) épingle 31 sources de produit,
juges et outils, les commandes, les deux suites de **21 CTests réussis**
et les relectures normal/−O. Les XML Release et ASan/UBSan finaux sont
conservés ici ; les captures préliminaires ont un motif d'exclusion explicite.

## Périmètre

Une mesure batch compare trois appels séparés et leur préparation partagée,
sur un seul propriétaire de rectangle. Une mesure axe compare le préfiltre
q2 Pool et le nouveau filtre axial, aux résidus différents. Les deux
ordres d'exécution sont conservés séparément. Aucun développement des
paires, census, parent ou export FULL n'est inclus. s8/10/12 vérifient la
séparation des mêmes rectangles : ce n'est pas une comparaison de WSPD.

La campagne principale couvre 8k/16k/32k, Kmax5/10 et s8/10/12. Les
familles sont `grid`, `sheet`, `skew` ; `sheet_full` s'ajoute pour le
filtre axial. La contre-fixture `rails` est séparée, à n2718. Une
répétition par tuple et par ordre couvre la matrice ; des campagnes de
focus ajoutent trois répétitions par ordre à Kmax10/s8. Lorsque les deux
campagnes couvrent le même tuple, le résumé dispose donc de quatre
répétitions, pas de quatre graines aléatoires. Les entrées sont déterministes.

GCC 13.3 Release, un processus de mesure à la fois, aucun échauffement
caché ; les commandes, options, état Git, hashes, sorties brutes et
compteurs sont dans chaque triplet MANIFEST/MEASURES/COMPLETION. Machine
locale partagée AMD EPYC 9V74, huit processeurs logiques exposés. Les
petites fluctuations de fréquence et de charge ne sont pas une preuve
de gain à elles seules ; les compteurs de travail complètent les temps.

## Rejeu

Après récupération des sources exactes épinglées dans les manifestes :

```bash
python3 -B morsehgp3D_v8/bench/check_paired_campaign.py morsehgp3D_v8/receipts/shared_axis_20260913 --summary
python3 -B -O morsehgp3D_v8/bench/check_paired_campaign.py morsehgp3D_v8/receipts/shared_axis_20260913
```

Le lecteur refuse les matrices vides/incomplètes, les identités
discordantes, les sources manquantes et les provenances déclarées
hétérogènes. Les captures préliminaires sont conservées à part avec leur
raison d'exclusion ; elles ne sont pas ajoutées aux répétitions mesurées.
Les anciennes captures P0 r3 restent dans leur dossier historique et
ne sont pas requalifiées par ces nouvelles mesures.

## Résultats de composants

Les cinq campagnes sont closes : **594 mesures, 504 configurations en
distinguant l'ordre**. Le partage conserve physiquement les trois plans.
À n32k/Kmax10, les préparations Tubes passent de 96 000 à 32 000 records ;
les comparaisons de tri sont exactement divisées par trois, sans changer
les requêtes de chaque voie ni leurs résidus.

Les temps ci-dessous sont les médianes de quatre répétitions à s8/Kmax10.
L'intervalle relie les médianes des deux ordres d'exécution ; ce n'est
**pas** un intervalle de confiance. Seuls les plans sont chronométrés
dans ce premier tableau, hors génération et propriétaire commun.

| Famille, n32k | Trois appels Tubes (ms) | Préparation partagée (ms) |
| --- | ---: | ---: |
| Grilles 3D équilibrées | 11,55–12,50 | 4,69–5,83 |
| Nappes tronquées | 9,08–10,33 | 3,05–4,46 |
| Grilles déséquilibrées | 11,66–12,90 | 4,29–6,15 |

Le gain reste visible avec génération et propriétaire inclus : pour
les grilles équilibrées, 14,24–15,19 ms deviennent 7,38–8,52 ms.
Les effets d'ordre sont importants, d'où leur publication séparée.
Ce batch n'a ni census ni hiérarchies K.

Pour le **filtre axial q2 sur grilles planes complètes**, même protocole :

| n | Paires initiales | Paires conservées | Descripteurs | Visites de requête | Filtre axial (ms) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 16 000 000 | 1 475 800 | 171 878 | 764 148 | 12,79–12,87 |
| 16 000 | 64 000 000 | 3 124 300 | 371 932 | 1 686 564 | 26,96–27,31 |
| 32 000 | 256 000 000 | 6 483 670 | 766 418 | 3 529 456 | 56,68–57,20 |

La construction de l'index ajoute respectivement 100 960, 217 920 et
463 232 visites de points, déjà incluses dans le temps du filtre.
Le temps n32k avec génération/propriétaire est 59,02–59,53 ms. Pool
calcule son préfiltre en environ 2,35–2,43 ms mais conserve toutes ses
paires : la réduction du résidu coûte ici du travail, dont le bénéfice
final dépend encore du consommateur aval. Lorsque n double, M est
multiplié par environ 2,1 et les visites par environ 2,1–2,2, pas par
quatre. La borne de cette famille complète explique cette observation ;
ce n'est pas une extrapolation à tous les nuages.

La nappe tronquée historique donne 1 486 480, 3 128 160 et 6 484 060
candidates aux trois tailles ; son filtre coûte environ 12,5–12,8,
27,3–27,5 et 56,4–57,5 ms. Ces entrées restent distinctes de `sheet_full`.

## Limites et décision suivante

Le filtre axial **ne remplace pas Pool partout**. À n32k/Kmax10/s8, la
grille 3D équilibrée conserve 44 078 400 paires avec l'axe, contre
378 840 avec Pool ; la grille déséquilibrée, 5 459 500 contre 144 298.
Une rotation peut aussi supprimer les alignements et rendre le résidu
quadratique. Les deux préfiltres pourraient se combiner par intersection
de résidus sûrs ; pas par addition aveugle de crédits recouvrants.

L'addition des **colonnes exactes disjointes**, les queues/fenêtres A/B
des auditeurs et la suppression de quelques allocations temporaires
sont les prochaines améliorations bornées. Ensuite, un index global
de comptage strict doit payer le census q2 réel, y compris les sites
extérieurs à A∪B ; ne pas recompter le cœur après l'avoir crédité.

P0 reste ouverte. q3/q4 sur nappes conservent leur résidu quadratique ;
le nuage n'est pas encore validé une seule fois pour toute une WSPD.
Mémoire de pointe, aval complet, multi-CPU, GPU, tour 50k sous une seconde
et dizaines de millions sur G4 **ne sont pas qualifiés**. Aucun temps
ci-dessus ne doit être présenté comme une réalisation de ces contrats.

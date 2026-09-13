# Census q2 individuel et partagé — campagne mono

13 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Quatre campagnes closes : **204 mesures appariées**, soit 408 bras
chronométrés et 164 configurations distinctes, ordre compris. Les lecteurs
normal/−O passent ; les 18 pins source de capture et le binaire sont conservés.
Le lecteur corrigé a une filiation distincte, explicitée ci-dessous.
Les [31 CTests Release et Clang ASan/UBSan](QUALIFICATION.json) passent.
GCC 13.3, C++20 strict, mono sur AMD EPYC 9V74, huit CPU logiques visibles.

## Résultat principal : le coût aval départage enfin les préfiltres

À n32k/Kmax10/s8, temps complets en millisecondes. Chaque intervalle est
la plage des **deux médianes par ordre**, trois répétitions par ordre ;
ce n'est ni un intervalle de confiance ni l'étendue de tous les essais.
Les trois préfiltres donnent exactement le même digest final à entrée/K fixés.

| Famille | Préfiltre | Candidates | Census individuel | Census partagé |
| --- | --- | ---: | ---: | ---: |
| Grilles 3D | Axial indépendant | 44 078 400 | 8 701–8 708 | 6 096–6 270 |
| Grilles 3D | Axial additif | 13 762 732 | 3 742–3 789 | 3 399–3 429 |
| Grilles 3D | Addition ∩ Pool | 114 716 | 130,6–131,9 | 146,0–146,2 |
| Nappes complètes | Axial indépendant | 6 483 670 | 3 257–3 349 | 3 505–3 542 |
| Nappes complètes | Axial additif | 3 928 390 | 2 504–2 524 | 2 602–2 656 |
| Nappes complètes | Addition ∩ Pool | 3 928 390 | 2 474–2 498 | 2 589–2 603 |

Les grilles émettent 16 183 supports ; les nappes 455 418. L'intersection
réduit fortement le coût complet des grilles par rapport aux deux autres
préfiltres testés. Sur les nappes, l'addition paie finalement son surcoût
de sélection en économisant du census. Pool n'y réduit pas davantage le
résidu : les petites différences Additive/intersection ne démontrent pas
un bénéfice propre à Pool. **Pool seul sans filtre axial n'est pas consommé
par cette API** ; ces chiffres ne le départagent donc pas encore.

Le partage du census gagne sur les grilles avec les préfiltres moins sélectifs,
mais perd après l'intersection et sur les nappes. Il n'est pas installé comme
vainqueur automatique. Exemple explicatif dans `main_matrix`, nappe32k/K10,
ordre individuel d'abord : 189,65 → 146,04 millions de classifications,
mais 1 650 → 1 825 ms de comptage. Il ajoute 13,83 millions de visites de
couverture B et 2,92 millions de subdivisions. Seules 1 758 paires sont
rejetées par une décision terminale portant sur plusieurs B, sur 3,93 millions.
La préparation de l'arbre B ne prend que 0,70 ms ; elle n'explique pas l'écart.
Les classifications de groupes coûtent plus que celles d'une boule fixée.
Ces compteurs localisent du travail, sans attribuer précisément son temps
à chaque opération. La collecte conserve 44,74 millions de visites dans
les deux modes et représente ici environ 0,44–0,47 s.

## Croissance à 8k, 16k, 32k

La matrice principale emploie Addition ∩ Pool, Kmax5/10, s8/10/12, deux
ordres et une répétition par tuple. Les rapports du **temps complet** à
s8, tous K et ordres de cette matrice confondus dans leur plage, sont :

| Famille | 8k → 16k | 16k → 32k |
| --- | ---: | ---: |
| Grilles 3D | environ ×1,89–2,09 | environ ×1,65–1,92 |
| Nappes complètes | environ ×2,03–2,18 | environ ×1,98–2,17 |
| Facteurs déséquilibrés | environ ×1,55–2,01 | environ ×1,91–2,00 |

Ces croissances sont inférieures au ×4 quadratique sur les familles testées.
Les visites de comptage croissent au plus d'environ ×2,20 par doublement.
**Ce n'est pas une preuve sous-quadratique générale** : rotations, vraie
WSPD, q3/q4 et volume de sortie FULL restent hors de cette campagne.
Les fluctuations entre ordres, visibles dans le [résumé chiffré](SUMMARY.json),
interdisent de surinterpréter quelques pour cent. Les répétitions renforcées
à 32k portent sur la comparaison des préfiltres, pas sur toute la matrice.

## Essais à 50 000 sites : composant q2, pas contrat de tour

Addition ∩ Pool, s8, trois répétitions par ordre. Même convention de plage
des médianes ; temps complets en millisecondes.

| Famille | Kmax | Supports émis | Individuel | Partagé |
| --- | ---: | ---: | ---: | ---: |
| Grilles 3D | 5 | 16 256 | 90,0–101,2 | 99,3–102,7 |
| Grilles 3D | 10 | 22 233 | 175,8–186,2 | 194,3–204,8 |
| Nappes complètes | 5 | 517 870 | 1 548,2–1 548,3 | 1 628,5–1 633,7 |
| Nappes complètes | 10 | 713 970 | 4 052,7–4 085,8 | 4 186,8–4 199,9 |

La fenêtre Kmax10 de la nappe garde 6 206 110 candidates ; le comptage
individuel visite 308 502 496 nœuds, le partagé 236 337 076. Aucun de ces
temps ne qualifie la tour 1..10 ou 1..5, et le jalon 100 ms n'est pas acquis.
Aucun essai multi-CPU, GPU ou multi-millions n'est inclus.

## Périmètre mesuré

Un rectangle séparé et tous les sites de son propriétaire : génération et
hash de la fixture, copie/validation, index global, préfiltre, census q2,
collecte des intérieurs stricts et de toute la coquille, digest des supports
émis et destruction. Les deux parcours partagent index et résidu ; les coûts
communs sont comptés une fois dans le total de chaque bras. Le bras Shared
paie aussi son index de requêtes B et la couverture des plages.

`count_ms` est un résidu incluant l'instrumentation, pas un chronomètre de
kernel nu. L'inspection qui compare les deux bras est mesurée à part.
Les temps n'incluent ni parsing/sérialisation du JSON ni capture externe.
Le flux conserve les supports et leurs clés, sans déduplication globale
des boules. WSPD complète, q3/q4 et tour FULL ne sont pas exécutés.

Les recettes restent les deux grilles 3D (`grid`, `skew`) et les nappes
entières (`sheet_full`), avec IDs et hashes fixes. s8/10/12 vérifie leur
séparation, sans produire trois WSPD. Kmax est le seuil du census et du
préfiltre ; ce n'est pas une tour construite. Tous les timings sont mono,
sur machine partagée, sans garantie d'isolement du système.

## Matrices, vérification et rejeu

- `main_matrix` : 108 lignes, trois familles, n8k/16k/32k, K5/10,
  s8/10/12, intersection, deux ordres, une répétition.
- `prefilter_comparison` : 48 lignes, grilles et nappes aux trois tailles,
  K5/10, s8, indépendant/additif, deux ordres, une répétition.
- `focus` : 24 lignes supplémentaires à n32k/K10/s8, trois préfiltres,
  grilles/nappes, deux ordres, deux répétitions supplémentaires.
- `50k_component` : 24 lignes, grilles/nappes, K5/10, s8,
  intersection, deux ordres, trois répétitions.

Les captures emploient le [runner spécialisé](../../bench/run_q2_census_matrix.py).
Chaque manifeste conserve commandes, état Git, sources/binaire, compilateur,
CMake et machine. Les sorties brutes et leurs encodages sont conservés ;
la fermeture vérifie les hashes. Les lecteurs refusent une matrice incomplète,
des comptes incohérents ou des sorties finales différentes entre préfiltres.
Les médianes sont séparées par ordre d'exécution. Aucun digest ne remplace
le juge géométrique indépendant sur petites entrées.

Deux défauts du lecteur initial ont été corrigés après capture : il ne
vérifiait pas les comptes exacts de construction B (`2*|B|-1` nœuds,
`|B|` visites) et pouvait ignorer un frère échoué avant création de son
manifeste. Le lecteur courant découvre les trois types de fichiers de
campagne, rejette les triplets incomplets et vérifie ces comptes B. Les
204 lignes authentiques satisfont les nouvelles conditions. Les tests
exercent 15 mutants de capture, 17 de lecture, quatre d'archive, ainsi
qu'un véritable échec initial à côté d'une capture réussie, normal/−O.
Un plancher de travail rejette aussi le faux Shared dont tous les compteurs
de comptage sont nuls malgré un résidu non vide, même si index et payload
sont conservés. Les seules égalités entre compteurs auraient accepté ce
cas artificiel `0=0` ; les 204 lignes réelles satisfont aussi ce plancher.

Les captures restent épinglées au runner `311fce7f…`, dont les octets sont
conservés dans [CAPTURE_RUNNER.json](CAPTURE_RUNNER.json), jamais exécuté
comme moteur alternatif. Le lecteur `892bd3ae…` accepte uniquement cette
filiation historique authentifiée, en plus de sa propre version pour de
nouvelles captures. Les 17 autres pins restent strictement égaux aux
sources courantes. `SUMMARY.json` distingue `capture_runner_sha256` et
`current_reader_sha256`. Aucun chronométrage n'a été refait pour ces deux
correctifs de validation ; aucun brut ni hash historique n'a été remplacé.

Le [reçu de qualification](QUALIFICATION.json) conserve les deux XML des
31 CTests et les pins des sources/juges. Le gate census confronte 8 376 paires
à 259 780 évaluations indépendantes, vérifie 72 permutations/isométries,
130 subdivisions après crédit, 10 modèles faux et 10 rejets d'API. Une
coquille de 30 sites à profondeur nulle et trois index profonds u16 sont
exercés. Les portes CLI et reçus passent aussi sous Python −O. Les grandes
sorties ne sont comparées que par digests, pas par un oracle exhaustif.
Ni RSS de pointe ni VRAM ne sont mesurées.

```bash
python3 -B morsehgp3D_v8/bench/run_q2_census_matrix.py check morsehgp3D_v8/receipts/q2_census_20260913 --summary
python3 -B -O morsehgp3D_v8/bench/run_q2_census_matrix.py check morsehgp3D_v8/receipts/q2_census_20260913
```

Ces commandes demandent les mêmes sources moteur/probe que la capture,
avec cette seule exception documentée de filiation du lecteur ; elles
n'acceptent pas une future version modifiée du moteur.
Lire le [contrat et ses bornes](../../docs/P0_CENSUS_Q2_PARTAGE.md)
et le [protocole](../../bench/P0_PROBE.md). Suite ciblée : préparer les
constantes géométriques une fois par tâche, puis juger l'économie sur le
temps complet et ne pas imposer Shared aux petits résidus. Cette piste
n'est pas encore implémentée. GCP non utilisé.

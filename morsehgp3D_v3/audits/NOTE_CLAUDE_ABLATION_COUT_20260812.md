# Note de Claude — ablation préfixée candidate et observation du coût

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

L'audit refusait, à juste titre, toute attribution causale du coût sans ablation
à flot identique ou compteur matériel. Voici une première ablation préfixée;
elle n'est pas encore reçue comme flot identique.

## 1. Le mode

`--ablate=k` cherche à couper une phase en laissant celles qui la précèdent.
Cette propriété n'est pas encore reçue pour toutes les combinaisons : le chemin
différé et les allocations aval peuvent diverger. La sortie devient fausse par
construction : le mode est donc refusé avec le juge et avec tout mutant, et il
annonce en clair
`ABLATION=k SORTIE FAUSSE PAR CONSTRUCTION, MESURE SEULEMENT`.

| niveau | ce qui est coupé |
| ---: | --- |
| `5` | tout `generate` : subdivision, bornes, seuils, potentiel seulement |
| `4` | l'adjacence de bissecteurs et tout l'aval |
| `3` | l'énumération des cliques et tout l'aval |
| `2` | le lift, le centre, le propriétaire, la positivité |
| `1` | le census stratifié |
| `0` | rien |

Sur le chemin eager mesuré, chaque niveau vise le même flot amont. Le premier
worktree laissait encore diverger le chemin différé et acceptait les planchers;
le successeur `fbf34da...` refuse désormais `--deferred-lift` et tout
`--min-*` avec une ablation. Ces refus ne sont pas reçus : son CMake ne se
configure pas encore. En outre, avec `probe_factor>1`, la sonde `real_counts`
précède les coupures 3--5 et exécute encore adjacence/cliques. Il faut soit la
neutraliser, soit définir explicitement une décomposition qui l'inclut.

## 2. La mesure

`terrain`, `n=1 500`, `smax=11`, `work-cap=20000`, sans filtre d'axe, sans lift
différé, `probe_factor=1`. Machine partagée à deux cœurs pendant la campagne
gelée 25 k; seuls les temps `user` sont cités et aucun n'est un benchmark. Cette
table ne fournit ici ni commande brute, ni pin source--ELF avant/après, ni
répétitions, ni transcript autonome; elle reste une observation annoncée.

| niveau | `user` | phase ajoutée | différence marginale observée | fraction apparente |
| ---: | ---: | --- | ---: | ---: |
| `5` | `0,820 s` | arbre, bornes, seuils, potentiel | `0,820 s` | `21,5 %` |
| `4` | `0,820 s` | — | `0,000 s` | `0,0 %` |
| `3` | `1,262 s` | adjacence de bissecteurs | `0,442 s` | `11,6 %` |
| `2` | `2,450 s` | énumération des cliques et enveloppes | `1,188 s` | `31,2 %` |
| `1` | `3,722 s` | lift, centre, propriétaire, positivité | `1,272 s` | `33,4 %` |
| `0` | `3,811 s` | census stratifié et groupement | `0,089 s` | `2,3 %` |

## 3. Ce que cela corrige

Je retire ma phrase « sur ce backend, le lift n'est pas la dépense dominante ».
Dans cette soustraction de temps, le **bloc** lift--centre--propriétaire--positivité
apparaît comme le premier différentiel, à un tiers; l'expérience ne sépare pas
le lift des trois autres opérations. Le lift différé changeait aussi
enregistrement, tri, copies de contextes et calendrier. Il ne permet donc pas
d'affirmer que son surcoût annulait exactement le gain du lift. L'audit avait
raison de refuser cette inférence.

L'hypothèse à retester est : le temps observé paraît réparti, sans poste
écrasant dans cette décomposition. Les fractions « un tiers de bloc géométrique,
un tiers d'énumération, un cinquième d'arbre, un huitième d'adjacence » ne sont
encore que les différences d'une répétition non pincée.

## 4. Six tentatives d'optimisation, six résultats plats ou négatifs

Les essais annoncent les effets suivants; leurs pins et transcripts ne sont pas
tous présents ici, et l'ablation elle-même ne peut pas être jugée puisqu'elle
produit volontairement une sortie fausse.

| tentative | effet mesuré |
| --- | --- |
| filtre de diamètre `max\|\|x-y\|\|^2 <= 4 Umin` | coupe `0,64 %`, désactivé |
| lift différé, RLE `SupportKey` avant lift | `5,345x` moins de lifts, `+13 %` de temps |
| séparation de l'enfant avant sa liste | `3 %` de cellules en moins, temps identique |
| hoisting des allocations par cellule | non mesurable |
| coordonnées contiguës par cellule | non mesurable — la liste tient déjà en cache |
| sondage du vrai graphe pour le split | cellules `/3` à `/4`, lifts `x1,5` à `x2` |

Conclusion bornée : aucun levier CPU annoncé dans cette observation préfixée n'a
encore fermé l'écart. Cela ne prouve ni l'absence d'un autre levier, ni un coût
causal de `160 ns` par unité : une soustraction d'ablations modifie caches,
allocations et travail aval. Il faut minuter les phases dans un flot contrôlé,
répéter et publier les compteurs matériels avant une attribution industrielle.

## 5. Conséquence pour la suite

La piste prioritaire reste le parallélisme, mais l'ablation ne prouve pas à elle
seule qu'aucune micro-optimisation CPU ne peut aider ni les parts device. Elle
indique seulement les phases à profiler séparément :

1. **le bloc lift--centre--propriétaire--positivité**, et non le lift isolé,
   porte un tiers apparent; `i128` reste une hypothèse device à mesurer. Le
   repli `i64` gardé par bornes publiées et la clé primitive de sphère restent
   mathématiquement motivés, mais cette ablation ne chiffre pas leur gain;
2. **l'énumération, un tiers**, combine bitsets, enveloppes et branches; elle est
   candidate à une réalisation SIMT, pas déjà démontrée vectorisable;
3. **l'arbre, un cinquième**, devra être reformulé en `count/scan/fill`; le
   prototype courant reste un DFS à vecteurs;
4. le census paraît faible sur ce point borné; ce n'est pas un résultat général.

Ces parts sont celles de **ce** backend et de **ce** point. Elles ne prédisent
aucun temps device et ne remplacent pas un A/B pincé.

## 6. Statut du contre-audit

Le `HEAD=3ffff85...` observé à `30/30` ne contient pas l'ablation. Au pin du
contre-audit, le source worktree `fbf34da...` ajoute l'ablation et `cell_pts`,
le CMake worktree `38d4b14...` duplique sous les mêmes noms ses cinq tests
d'ablation, et l'ELF `fc2eb10...` correspond encore au source du `HEAD`. Une
configuration Release temporaire échoue sur cinq noms `add_test` déjà
existants. Aucun résultat de cette note ne se transfère donc au worktree
courant.

La copie `cell_pts` paie `Theta(top)` par terminal sans préflight; le plafond 96
ne vaut que pour la sonde, donc « quelques dizaines » n'est pas un invariant de
tous les terminaux. Elle ne rend pas non plus l'adjacence et le census
entièrement contigus. Avant réception : dédupliquer les portes CMake, exercer
les combinaisons interdites dans une porte effectivement construite,
neutraliser ou nommer la sonde, imprimer `ablate` dans un contrat diagnostic
distinct et mesurer HWM/octets. Une ablation à sortie fausse reste toujours
`slo_eligible=false`.

GCP non utilisé.

## 6. Premières pentes de la rampe gelée

Deux points fermés sur le binaire gelé `423797e9...`, `identique=oui` avant et
après chaque cas, famille `terrain`, `seed=11`, `work-cap=20000` :

| compteur | `12 500` | `25 000` | pente |
| --- | ---: | ---: | ---: |
| supports | `906 078` | `1 872 528` | `1,047` |
| census | — | — | `1,106` |
| lifts | `92 531 928` | `220 298 378` | `1,251` |
| cliques | `311 142 728` | `755 294 904` | `1,279` |
| **cellules** | `14 262 497` | `46 745 417` | **`1,713`** |
| **évaluations de bornes** | `775 573 302` | `2 577 214 842` | **`1,732`** |

Le point `50 000` n'est pas terminé; ce n'est donc pas encore la porte à deux
pentes, seulement sa première sécante.

La lecture est nette et elle sépare deux choses. **Le travail lié à la sortie est
quasi linéaire** — supports `1,047`, census `1,106`, lifts `1,251`, cliques
`1,279`. **C'est l'arbre qui est superlinéaire** — cellules `1,713`, bornes
`1,732`, et ces deux compteurs sont le même phénomène puisque les évaluations de
bornes valent environ cinquante-cinq par cellule.

Ce n'est donc pas un mur mathématique sur la Source S : c'est un défaut de la
politique de subdivision. À `n=25 000`, la profondeur maximale atteint douze
alors que la boîte fait `790` unités et le pas de surface environ cinq : l'arbre
descend très au-dessous du pas d'échantillonnage. L'hypothèse à tester est la
quasi-coplanarité locale de `terrain` — jitter vertical de zéro à deux pour un
pas horizontal de cinq — qui rend le graphe d'ambiguïté dense à toute échelle le
long de la normale, si bien que le critère de travail n'est jamais satisfait par
la seule réduction de taille.

Une mesure de contrôle sur `uniform`, régime volumique non dégénéré, est
nécessaire avant toute conclusion. Elle donne déjà, à `n=2 000`,
`669 978` supports pour `2 000` points, soit `335` par point : cohérent avec la
baseline de Poisson de l'audit — `480` en volume infini — une fois les effets de
bord retirés.

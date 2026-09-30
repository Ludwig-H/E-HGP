# Audit indépendant : tête, bancs et qualification des résultats

29 septembre 2026. Base examinée : `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`.
Les sondes compilent les sources du snapshot indépendant
`/workspaces/E-HGP/build/v10-audit-independent-20260929/source/morsehgp3D_v10` et utilisent son build Release neuf.
Les fichiers examinés dans le checkout vivant ont été comparés à cette base : aucun écart dans les sources et
documents cités ci-dessous. Aucun changement du produit, des builds historiques ou d'une VM. GCP non utilisé.
Les seules exécutions sont de petites fixtures manuelles et des contrôles de reçus ; aucune scène de test n'a été
régénérée et aucune campagne longue n'a été lancée.

## Conclusion utile au développeur

La tête condense en N-aire, conserve la masse et utilise la bonne règle EOM `somme enfants > stabilité parent`.
Le banc appelle bien `sklearn.cluster.HDBSCAN` tel quel pour son adversaire principal ; le témoin MR commun est
explicitement distingué. Le lot C conservé porte exactement les 30 720 couples préenregistrés, sans doublon,
manquant ni refus. Les résultats publiés du lot C ne sont pas invalidés par les défauts de garde trouvés ici.

Les corrections prioritaires sont le validateur public de dendrogramme et la complétude de la décision du banc.
Deux autres défauts sont circonscrits aux paramètres non exercés dans les campagnes : `--allow-single` et les
exposants non finis ou non positifs. Le travail de qualification LiDAR doit rester distinct des excellents chronos
sur les trois trames de conception.

| ID | Gravité | Constat | Action proposée |
| --- | --- | --- | --- |
| H1 | P1, sûreté API | `validate(d)` accepte un rang de point hors tableau ; ASan prouve une lecture hors bornes | fermer les bornes et la cohérence parents/CSR avant consommation |
| E1 | P1, porte de preuve | `decide.py` décide sur une seule scène sur 960, même `complete=false` ; fusion accepte une scène hors plan | valider le produit exact manifeste × méthodes avant décision |
| H2 | P2, comportement exposé | `--allow-single` classe aussi les points sortis trop tôt de la racine | appliquer le seuil d'appartenance de la racine ou déclarer une autre sémantique |
| H3 | P2, domaine numérique | `--z=nan`, `inf`, `0`, `-1` rendent `status=ok` | valider les paramètres dans CLI, configs et API commune |
| H4 | P2, coût aval | les remontées aux ancêtres de la tête sont quadratiques sur un peigne valide | propager l'ancêtre sélectionné en un passage |
| C1 | P1, qualification | une conclusion de reçu dit le contrat tenu sur les seules trois trames de conception | conserver le chrono exploratoire ; qualifier le contrat multi-séquences séparément |
| C2 | P2, portée mathématique | ≈ 460 boules/site est formulé comme une borne géométrique | annoncer un ratio observé et une hypothèse de dimensionnement |

## H1 — Le validateur public ne rend pas son consommateur sûr

Sources : `src/points/dendrogram.cpp:24`, `src/points/dendrogram.cpp:28`,
`src/head/head.cpp:76`, contrat de `src/points/dendrogram.hpp:38`.

Fixture minimale : un nœud racine, `level={1}`, `node_rank={0}`, `parent={kNone}`, `child_off={0,0}`,
un point attaché à ce nœud avec `point_rank={1}` et poids 1. `validate` retourne `ok` : le test vérifie seulement
que le rang du point est au moins celui du nœud ; la borne supérieure est absente pour un point attaché à la
racine. `cluster` lit ensuite `d.level[1]`. ASan rapporte `heap-buffer-overflow`, lecture de 8 octets, à
`head.cpp:76`. La sonde est [evidence_head_validate.cpp](../../receipts/audit_independant_20260929/probes/evidence_head_validate.cpp), mode `rank` ; le résultat
brut est conservé dans [evidence_head_validate_results.json](../../receipts/audit_independant_20260929/evidence_head_validate_results.json).

Deux autres trous sont positivement exercés :

- `parent={1,kNone}` avec un CSR sans aucune arête et un point attaché à 0 est accepté. Le point disparaît du
  parcours de la racine et devient du bruit, même avec `allow_single_cluster=true`. La cohérence vérifiée
  enfant→parent ne couvre pas parent→enfant, ni l'unicité d'apparition dans le CSR.
- un dendrogramme à niveau unique `NaN` est accepté, car la stricte croissance ne compare rien ; la tête produit
  une stabilité `NaN`. Une borne de parent invalide peut aussi être déréférencée par le validateur avant un refus.

Il s'agit d'une frontière API défectueuse, pas d'une preuve que le producteur courant fabrique ces structures.
Correction peu coûteuse : `point_rank < level.size()`, niveaux finis/non négatifs, tous les parents bornés et
strictement postérieurs à leurs enfants, et exactement une occurrence CSR de chaque non-racine. Le contrôle du
CSR peut garder son coût linéaire. La consommation doit passer par cette validation, ou une vue validée possédée.

Les premières exécutions ASan+UBSan ont aussi montré que LSan ne fonctionne pas sous le ptrace du sandbox ; cet
échec de collecteur est conservé comme limite et n'est pas un défaut produit. Les reprises de la sonde désactivent
uniquement LSan (`ASAN_OPTIONS=detect_leaks=0`), en conservant ASan et UBSan.

## E1 — La décision ne prouve pas qu'elle couvre le plan enregistré

Sources : `bench/synthetic/decide.py:35`, `decide.py:45`, `decide.py:135`,
`bench/g4/merge_sessions.py:42`, `merge_sessions.py:51`, `merge_sessions.py:61`.

`decide.load` cherche les méthodes manquantes seulement parmi les scènes présentes. Il écrase silencieusement
un couple dupliqué. `main` compare le hash du préenregistrement, puis décide sans contrôler `complete`, le nombre
annoncé de scènes, le digest du plan, l'identité des unités attendues ou les métadonnées des lignes.
`merge_sessions` vérifie correctement les doublons entre sessions et le nombre de méthodes par unité, mais
compare seulement les digests des sessions entre eux et le nombre des unités à `segments[0]['scenes']`.

[evidence_bench_completeness.py](../../receipts/audit_independant_20260929/probes/evidence_bench_completeness.py) démontre les deux admissions :

```text
partial_decide: expected_scenes=960, present_scenes=1, declared_complete=false,
               exit_code=0, written_decision_scenes=1
merge_unknown_unit: accepted_unit=outside_the_preregistered_plan, exit_code=0
```

La sonde copie le préenregistrement en répertoire temporaire, conserve son plan de 960 scènes et réduit seulement
les tailles Monte-Carlo à 8 pour éviter tout calcul long. Elle ne génère aucune scène. Une session partielle est
normale pour la fusion ; elle doit pouvoir être écrite, mais ne doit jamais autoriser la décision confirmatoire.
Proposition : reconstruire le manifeste canonique, vérifier son digest, l'ensemble exact de couples, leur unicité,
les métadonnées et les valeurs finies. Appliquer le même garde au `--resume` avant de conserver des lignes.

**Qualification du reçu existant.** Le contrôle indépendant trouve pour le lot C : 30 720 couples attendus,
30 720 lignes, 0 manquant, 0 ajouté, 0 doublon et 0 refus ; digest
`f3b015333df0fae3dcf930a1bab6fe68d15d8a60b415ba39fac256f74d630575` conforme au plan. Les
`SHA256SUMS` des lots A/C et des sessions G4 4/5 passent respectivement 9, 67, 157 et 16 fichiers, sans écart.
Le préenregistrement C a été commité à 10:10:15 UTC (`bc413ff56`), son amendement à 10:39:38 (`65ea6b222`),
avant les débuts des sessions G4 enregistrés. L'absence de lecture du run local interrompu avant amendement est
une déclaration conservée dans le reçu ; le dépôt seul ne démontre pas une absence de consultation humaine.

## H2 — `allow_single_cluster` perd la règle d'appartenance de la racine

Sources : `src/head/head.cpp:122`, `head.cpp:161`,
`tests/head/test_condensation_vs_sklearn.py:62`. La porte existante ne teste jamais `allow_single_cluster=true`.

Fixture entière : `(x,0,0)` pour `x=0,1,2,3,4,5,6,7,50,100`, `K=1`, `mcs=3`, `--allow-single`.
Le produit et le témoin MR rendent tous deux dix labels 0. `sklearn 1.9.1`, `kd_tree`, mêmes K/mcs et option,
rend huit labels 0, puis `-1,-1`. Ces deux points ont quitté la racine avant le seuil retenu ; la tête v10 les
récupère en suivant simplement l'ancêtre sélectionné. La différence existe à K1, où la géométrie des arbres est
identique à un changement global d'échelle près : elle est donc un défaut de la règle de tête annoncée.

Sonde [evidence_head_semantics.py](../../receipts/audit_independant_20260929/probes/evidence_head_semantics.py). Ce défaut ne touche pas les résultats A/C,
dont les méthodes fixent `allow_single=false`. Ajouter une fixture positive avec amas unique et bruit, ainsi
qu'une fixture où la racine reste exclue ; limiter la correction à la règle spéciale de la racine sélectionnée.

## H3 — Le domaine de l'échelle n'est pas contrôlé

Sources : `cli/mhgp10_cluster.cpp:49`, `mhgp10_cluster.cpp:142`, `src/head/head.cpp:11`.
La fixture de dix points précédente accepte `--z=nan`, `--z=inf`, `--z=0` et `--z=-1`, écrit les labels et
annonce `status=ok`. Le mode `nan-z` de la sonde C++ produit directement une stabilité `NaN`.
Un z négatif renverse le sens de λ et rend les contributions de stabilité potentiellement négatives.

Valider au minimum z fini et strictement positif à la frontière commune, également pour `--configs` ; contrôler
que les λ et stabilités obtenus sont utilisables ou définir un domaine supérieur garantissant l'absence de
débordement/sous-flux. Aucun z fixé dans les campagnes A/C n'est hors domaine.

## H4 — Un peigne valide fait remonter Θ(C²) ancêtres

Sources : `src/head/head.cpp:135`, `head.cpp:155`, `head.cpp:163`.
Le condensé crée ses parents avant leurs enfants, mais cherche ensuite leur premier ancêtre sélectionné
séparément pour chaque cluster et point. Un peigne dont chaque feuille porte deux points, `mcs=2`, sélection
`leaf`, a Θ(C) clusters internes non sélectionnés et une profondeur Θ(C). Chaque interne remonte jusqu'à la
racine non sélectionnée. Le seul passage `cluster_label` est donc quadratique ; le nettoyage EOM peut lui aussi
remonter les mêmes chaînes.

La petite sonde [evidence_head_complexity.cpp](../../receipts/audit_independant_20260929/probes/evidence_head_complexity.cpp) ne mesure pas un chrono ; elle
compte exactement les visites imposées par ce passage sur une structure validée :

| Points | Clusters condensés | Visites des ancêtres pour `cluster_label` |
| ---: | ---: | ---: |
| 130 | 129 | 2 145 |
| 258 | 257 | 8 385 |
| 514 | 513 | 33 153 |

Propagation possible : une passe parent→enfant suffit à transmettre l'ancêtre sélectionné ; une passe similaire
transmet le fait qu'un ancêtre est déjà choisi. Les labels des points peuvent ensuite lire `cluster_label`.
Ce constat concerne le coût de la tête, sans prétendre que les nuages LiDAR actuels produisent un tel peigne.

## C1 — Les performances observées et le contrat adopté ont des périmètres différents

Le périmètre entier u18 est autorisé : `AGENTS.md`, ouverture v9, conserve la décision datée du 22 septembre de
poursuivre l'entier 18 bits et de laisser le float32 hors contrat temps. La v10 n'a donc pas à être jugée comme si
elle avait silencieusement remplacé une exigence active de chrono float32.

`docs/conception/CONCEPTION_V10.md:162` adopte 30 trames test sur dix séquences, avec p95 de médianes, maximum,
digests de préfixes et juges d'échelle. Ses lignes 164–165 distinguent explicitement ces trames des trois trames
de conception 08/000000, 000100, 000200. Le dossier `perf/FRAMES_v2.toml` annoncé n'existe pas à la base auditée.
Les reçus G4 2–5 ne qualifient que les trois trames de conception ou leurs secteurs. Le reçu G4 2, ligne 47, dit
pourtant « le contrat ... est tenu ». Préférer « jalon de temps observé sur les trois trames de conception » et
laisser C(K,T) ouvert jusqu'au protocole multi-séquences. Les secteurs restent un diagnostic de croissance.

Le calcul mesuré est bien **la tour 1..K**, et comprend les verticales : les stdout de G4 4 portent les ordres
1..5 et 1..10. `--no-points` retire les attaches de clustering, pas les ordres bas. Le backend CPU seul est bien
annoncé ; interroger la carte NVIDIA en G4 5 n'est pas présenté comme une exécution GPU.

Les sommes publiées G4 4 sont les dernières passes chaudes. Recalcul des médianes des trois sommes appariées :

| Trame | K5, dernière / médiane (s) | K10, dernière / médiane (s) |
| --- | ---: | ---: |
| 08/000000 | 0,2520 / 0,2562 | 1,1246 / 1,1246 |
| 08/000100 | 0,2042 / 0,2143 | 0,8614 / 0,8698 |
| 08/000200 | 0,2536 / 0,2603 | 1,0243 / 1,0392 |

Le code `cli/mhgp10_tower.cpp:77` prépare nuage, pool et index avant les passes ; ses 6,6–8,3 ms de préparation
ne sont pas incluses dans catalogue+tour. La lecture du fichier et la segmentation/quantification sont encore
d'autres périmètres. Les temps restent convaincants, mais un reçu de contrat doit fermer exactement B2 et
publier B0 et C, sans appeler cette somme un coût total brut→tour.

## C2 — Les ratios géométriques mesurés ne sont pas une borne générale

`PASSATION.md:130` et `receipts/g4_session5_scale_20260929/README.md:61` formulent « les boules par site sont
bornées par la géométrie : ≈ 460 en 3D ». La campagne démontre des ratios de sortie et des coûts par boule sur ses
familles, pas un plafond mathématique par site. La spécification, §7, et la conception intégrée, ligne 182,
laissent expressément ouverte la borne de pire cas. L'audit v9 cite une famille à sortie quadratique à K fixé,
prouvée dans `morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md`.

Cette famille rationnelle requiert une précision croissante ; elle ne devient pas, par citation, une suite
asymptotique d'entrées u18. Elle réfute une formulation géométrique indépendante de la précision, et ne fournit
pas non plus une borne de 460 pour l'univers fini u18. Le dimensionnement « 1,4 M sites LiDAR tiennent » doit
donc être annoncé sous l'hypothèse du ratio ≈120 observé et du coût mémoire mesuré, avec marge, pas comme une
garantie pour toute géométrie LiDAR.

## Limites et suite ciblée

Les petites portes actuelles établissent davantage que de simples comptes d'amas ; la tête à K1/K2 est jugée
contre sklearn, les attaches de couverture contre l'oracle, les appels groupés contre les appels séparés. Je n'ai
pas relancé le test de collision de 8 000 points, conformément à la consigne de calcul long sur G4. Son code et
`tower.cpp:1839` documentent que deux niveaux exacts indiscernables en double partagent un rang dans le
dendrogramme de points : cette approximation doit rester une propriété de la tête, sans être attribuée aux niveaux
rationnels de la tour FULL. Aucune divergence de la tour FULL n'est déduite ici.

`docs/SPEC_V10.md` se dit autonome mais décrit encore seulement l'entrée `core` et dit que G4 et la supériorité du
banc ne sont pas mesurés, alors que les reçus du 29 existent. Une mise à jour courte de cette carte réduira les
confusions entre conception normative, produit livré et jalons exploratoires. Pour avancer sans nouvelle campagne
lourde : fermer H1/E1, ajouter les petites fixtures H2/H3, puis remplacer les trois remontées de H4 par les passes
linéaires avant de mesurer les familles de qualification supplémentaires.

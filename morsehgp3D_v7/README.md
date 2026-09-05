# MorseHGP3D v7

Cadre ouvert le 4 septembre 2026 à la demande explicite de l'utilisateur :

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La porte d'entrée exploratoire est satisfaite par la lecture intégrale des
parties I (pages PDF 35–76) et II (77–134) du manuscrit, l'audit des contrats
v6 et la déclaration des réserves dans [l'état courant](audits/ETAT_COURANT.md).
Cette ouverture n'est pas une promotion du registre officiel.

La reprise du code v6 est explicitement autorisée. Le point de départ est
`de69851e3820781145f859a08a993f15f2f9e738` avec un worktree v6 modifié :
quatre fichiers suivis modifiés et trois fichiers nouveaux. Le port épingle
les octets consommés dans [V6_SOURCE_SNAPSHOT.json](docs/V6_SOURCE_SNAPSHOT.json)
et requalifie ses propres tests.
La v6 reste intacte. Aucun reçu historique ne constitue une mesure v7.

Objectif : une hiérarchie HGP exacte et exploitable industriellement,
sans construire la mosaïque de Delaunay d'ordre supérieur ni Gamma exhaustif.
Les campagnes locales demandées portent sur 8 000, 16 000 et 32 000 points.
Les contrats de 50 000 points et de plusieurs dizaines de millions restent
des critères de qualification distincts, pas des résultats annoncés.
Le [contrat courant de performance](docs/CONTRAT_PERFORMANCE.md) fixe
l'ordre mono-thread, multi-CPU, GPU : toute la tour K=1 à 10 sur 50k
en moins d'une seconde, repli 1 à 5 si nécessaire, puis objectif 100 ms
après validation du premier jalon. La cible massive est GCP G4.

Attention : le flot Gabriel v6 est `verified_events_only`. La fixture
`gabriel-point-set-counterexample-5-points-v1` démontre qu'il ne suffit pas
à reconstruire Gamma. La complétion silencieuse et le fold réduit sont
implémentés en option. Le [certificat horizontal réduit sur E](audits/CERTIFICAT_HORIZONTAL_COURANT.md)
ferme leur composition sur le domaine CPU régulier déclaré, sans couvrir
les plateaux généraux, la verticale ni le passage à grande échelle.
La conservation du delta F et sa qualification propre restent distinctes.

L'[audit des niveaux et du certificat suffisant](docs/AUDIT_NIVEAUX_GABRIEL_20260905.md)
distingue désormais cette insuffisance du graphe brut de la suffisance des
niveaux Gabriel. Sous régularité, HGP complet peut se représenter par les
minima Gabriel de cardinal K et les vraies fusions de cardinal K+1, avec
leurs parents correctement résolus. Ce certificat n'exige pas Gamma
exhaustif. La qualification générale du constructeur, les plateaux hors
domaine régulier et le supplément pondéré restent ouverts. Aucun succès
50k n'en est déduit ; le premier composant régulier est décrit ci-dessous.

Le premier [composant FULL compact](docs/CONTRAT_CERTIFICAT_FULL.md) est
maintenant implémenté séparément : validation transactionnelle de minima
et multifusions déjà décidés, parents CSR, coupes exactes et unions de
feuilles sous plafonds. Ses [deux portes dédiées](receipts/full_certificate_20260905/README.md)
passent en Release et sous ASan/UBSan. Son autorité reste strictement
structurelle : aucun portail calculé, aucune complétude géométrique
certifiée, aucune activation dans la CLI ou le pipeline F.

Le [producteur horizontal FULL](docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
ajoute maintenant la décision des parents depuis deux catalogues Gabriel
fournis, avec portails paresseux et normalisation des anciennes ancres.
Il conserve les minima isolés et K=n, sans construire le cœur Gamma F.
Son succès est **relatif à des catalogues complets, exacts et réguliers**,
pas une authentification de leur complétude. Les nouvelles portes confrontent
les coupes à un oracle Gamma indépendant borné, avec refus et pannes mémoire.
Les [sept CTests ciblés](receipts/full_gabriel_20260905/README.md) passent
en Release puis sous ASan/UBSan ; la première tentative SAN échouée pour
LeakSanitizer/ptrace reste archivée, sans désactivation du détecteur.
La verticale, l'archive et la CLI FULL restent à raccorder ; aucune mesure
de ce seul composant ne vaut contrat 50k pour toute la tour.
Le [pont indépendant](audits/PRODUCTEUR_FULL_GABRIEL_COURANT.md) qualifie
ensuite les mêmes octets sur ses propres catalogues rationnels bornés.
Les [mesures mono FULL](docs/RESULTATS_MONO_FULL_20260905.md) séparent
les dix ordres terminés à 8k des refus de budget d'alias à 16k et 32k.

## Construire et tester

```bash
cmake -S morsehgp3D_v7 -B build/v7 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v7 --parallel 2
ctest --test-dir build/v7 --output-on-failure
```

Dans l'atelier courant, `build/v7/mhgp7` est conservé comme témoin C
historique. Le binaire F qualifié est `build/v7_f_qualification/mhgp7` ;
pour une reconstruction indépendante, choisir un répertoire de build neuf
au lieu d'écraser un témoin utilisé par les comparaisons.

La suite de portes bornées se sélectionne avec `-L '^gate$'`. Les tests
`scale8000`, `scale16000` et `scale32000` sont des campagnes plus longues,
séparées des petites portes. Les nouveaux refus, mutants et contre-fixtures
restent exécutables sans CUDA. Les cibles GPU exigent une qualification
matérielle propre ; les tests `stub` ne sont jamais des mesures GPU.

Le [workflow CPU v7](../.github/workflows/morsehgp3d-v7.yml) reprend ces
portes et les tests des runners. Il n'a ni identifiants cloud ni permission
d'écriture. Les reçus locaux et les exécutions GitHub restent deux preuves
distinctes ; leurs résultats courants sont dans [la passation](PASSATION.md).

## Entrée réelle et archive transactionnelle

L'entrée texte contient une ligne `PointId x y z` par point : identité u32
unique, coordonnées entières entre 0 et 65535. Les lignes vides et celles
commençant par `#` sont permises. Aucun arrondi de flottants ni quantification
silencieuse. Les identités et l'ordre physique sont conservés à l'export.

```bash
build/v7/mhgp7 --input=scan.u16.txt --output=sortie-v7 --threads=8 --layout=csr
```

Par défaut, la sémantique est `verified_events_only`, compatible avec
l'objet v6 : ce n'est pas Gamma complet. `--output` crée un répertoire neuf,
avec entrée, forêts par ordre et manifeste SHA-256. Il ne remplace jamais
une destination existante. Aucun préfixe K1..Kj n'est publié si un ordre
suivant échoue. En revanche, les callbacks de l'API C++ sont provisoires :
un client doit attendre le statut final ou utiliser un sink transactionnel.

L'archive n'est ni un checkpoint du moteur ni une garantie de reprise après
coupure électrique. Le diagnostic `archive_directory_sync=unconfirmed`
signale un échec de synchronisation du parent après publication atomique.

## Route horizontale normalisée candidate

```bash
build/v7/mhgp7 --input=scan.u16.txt --output=sortie-normalisee --threads=8 --layout=csr --complete-incidences
```

Cette option ajoute les incidences silencieuses nécessaires sur son domaine
régulier, puis distingue les carriers latents des composantes déjà enracinées.
Elle porte `normalized_horizontal_h0_candidate`, pas `exact`. Elle refuse
les extra-shells pertinents ou rencontrés par la descente et toute limite
de ressources atteinte. `--require-exact` refuse tant que la qualification
globale manque. Aucun repli automatique vers l'objet Gabriel n'est effectué.

Les cinq plafonds de travail sont exposés par `--silent-core-records`,
`--silent-chain-steps`, `--silent-cofaces`, `--silent-query-nodes` et
`--silent-meb-supports`. Une valeur zéro est un plafond nul, pas une
désactivation. Ces options exigent `--complete-incidences`. Le budget
`--mem-budget` reste un proxy partiel de payload, pas un plafond RSS.

Dans ce mode, `born` signifie **première matérialisation dans le sous-flot**,
pas naissance géométrique exhaustive. Les facettes sont les feuilles ;
leur projection vers les points peut se recouvrir. Ni les applications
verticales ni les scores d'incidence du vote pondéré ne sont exportés.

## Preuves et mesures

Lire [l'état de livraison](PASSATION.md),
[Lecture et contrats](docs/LECTURE_ET_CONTRATS.md),
[Incidences silencieuses](docs/INCIDENCES_SILENCIEUSES.md),
[Composition horizontale conditionnelle](docs/PREUVE_HORIZONTALE_COMPOSITION.md),
[Qualification des primitives S1](docs/QUALIFICATION_S1_PRIMITIVES.md),
[les portes arithmétiques intégrées](receipts/arithmetic_gates_20260904/README.md),
[le mode mono](docs/MODE_MONO.md),
[la première comparaison s=8/10/12](docs/RESULTATS_MONO_20260904.md),
[la comparaison mono de la MEB différée](docs/RESULTATS_MONO_MEB_20260905.md),
[le prétest q2 et ses trois paires mono](docs/RESULTATS_MONO_Q2_20260905.md),
[la qualification E : 33/33/324 portes](receipts/meb_q2_integrated_20260905/README.md),
[la pile locale F et ses invariants](docs/OPTIMISATION_PILE_TEMOINS.md),
[la qualification F : 48/48/339 portes](receipts/witness_stack_integrated_20260905/README.md),
[les trois paires E/F et les paliers 16k/32k](docs/RESULTATS_MONO_F_20260905.md),
[la qualification locale du MEB à double budget](docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md),
[son coût natif et les régressions observées](docs/RESULTATS_COUT_MEB_20260905.md),
[les résultats G4 à 50k et les primitives GPU](docs/RESULTATS_G4_20260904.md),
[les limites de résidence et la voie massive](docs/RESIDENCE_MASSIVE.md),
[l'état courant](audits/ETAT_COURANT.md) et
[la réponse aux audits indépendants](audits/REPONSE_CONSTRUCTEUR_20260904.md).
Les mesures et les refus futurs doivent être liés à leurs sources et
binaires, jamais transférés depuis un reçu historique.

Les campagnes `bench/compare_v6_v7.py` et `bench/incidence_campaign.py`
séparent la compatibilité avec la v6 des observations de la nouvelle route,
refus compris. Un succès de campagne n'est pas une qualification exacte,
un p95 industriel ou un contrat satisfait à plusieurs millions de points.

Dernière campagne F close : les trois paires 8k à s=8/10/12 conservent les
objets et comptes, sans gain de temps robuste. La tour candidate 1..10
termine à 16k en 413,82 s ; à 32k elle refuse à K=9 sur le plafond de
records du cœur. Ce refus n'est ni un timeout ni un succès partiel publié.
Les contrats 50k/1 s puis 100 ms et la cible massive G4 restent ouverts.

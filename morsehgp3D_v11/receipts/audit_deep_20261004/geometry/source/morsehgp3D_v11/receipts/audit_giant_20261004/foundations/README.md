# Fondations v11 — relecture indépendante du 4 octobre 2026

Source examinée : `0f5e8a207f2974e262cd40a8882b97af1da396af`. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Aucun build, exécutable natif, fit, workflow ou appel GCP exécuté par cet audit.

La relecture de `core`, `cloud`, `sched`, des modules CMake et des deux lecteurs
binaires de banc ne trouve aucun nouveau défaut causal dans ces fondations.
La [matrice source](source_matrix.json) distingue les assertions contrôlées,
les préconditions et les composants encore absents. L’[inventaire](module_inventory.json)
est exhaustif pour les dossiers concernés. `src/io`, `src/api`, `src/parallel`
et le CLI public ne sont pas présents au pin : la Session et la transaction
CLI finale demeurent un contrat de livraison.

## Preuves existantes recoupées

La qualification native close exécutait `c40f40798375a0fc37917499401f16876cccbd2a`,
avec GCC 11.4. La [parité des blobs](proof/source_parity_c40.json) confirme
l’identité des sources et tests des fondations, du CMake et des helpers IO
avec le pin examiné. Dans ce périmètre, seules README et PROVENANCE ont évolué.
Cette capsule relit des résultats conservés ; elle ne rejoue pas la qualification.
Elle ne recopie pas le paquet source de 32 Mo déjà conservé dans le reçu publié.

| Configuration effectivement jouée | Profil | Core | Cloud | Sched | Total |
| --- | ---: | ---: | ---: | ---: | ---: |
| GCC Release | 18 | 29 | 20 | 13 | 62 |
| GCC Release | 21 | 29 | 20 | 13 | 62 |
| GCC Release | 24 | 29 | 20 | 13 | 62 |
| GCC ASan/UBSan | 24 | 29 | 20 | 13 | 62 |
| GCC TSan | 21 | 29 | 20 | 13 | 62 |
| GCC poison | 21 | 30 | 20 | 13 | 63 |

Les **373 exécutions de portes** ci-dessus passent sans saut. Ce nombre inclut
les portes d’inventaire et de refus ; ce ne sont pas 373 fixtures indépendantes.
Clang est explicitement absent dans cette prise. Le supplément ASan18 n’est
pas ajouté à ce compte des fondations.

Les campagnes de mutants déclarent core 78, cloud 16 et sched 8, tous tués :
102 au total, dont deux refus de construction dans core, zéro signal et zéro
délai. Le manifeste core inclut également CMake et les harnais : ces compteurs
ne sont pas 102 mutations géométriques du produit. L’archive native originale
(777552 octets, SHA `dd41dd607e502133155ebae8ae0cea0dab9ee6420c59be2435e1536ac2c8b136`)
est conservée avec ses 115 payloads inventoriés et rehachés. Le reçu sélectionné
conserve le commit, le hash du paquet, l’identité de génération et l’arrêt
ciblé certifié ; aucune qualification GPU n’en découle.

## Point pertinent transmis à l’auditeur du pipeline

Dans [forest_vertical.cpp](source/morsehgp3D_v11/src/tower/forest_vertical.cpp),
`follow` contrôle `low.abandoned` dans le corps de `while(!follow_lower_ready(...))`
(lignes 202–204). Après le réveil `low.block()`, une publication abandonnée peut
avoir `closed=kNone`, `done=false`, `abandoned=true` : `finish` publie cette
sentinelle ([forest_internal.hpp](source/morsehgp3D_v11/src/tower/forest_internal.hpp),
lignes 39–43). Le prédicat `level<closed` devient vrai ; le corps contenant le
contrôle d’abandon est alors sauté. Les appels `advance`, `birth_image`, `visit`
restent accessibles aux lignes 206–208 sur ce chemin abandonné.

C’est une rupture certaine du protocole de lecture, établie par le contrôle
source. Le Pool continue les tâches après un refus, et les autres résolveurs
peuvent donc rester actifs. `RegularVerticalSeeds::find` lit un tableau non
atomique alors que `remember` l’écrit ; l’invariant de publication du rang est
nécessaire à cette lecture. Le tableau est bien initialisé à `kNone` : cet audit
ne revendique ni lecture non initialisée, ni race TSan reproduite, ni succès
FULL incorrect. L’auditeur du pipeline prépare séparément l’ordonnancement et
le contrôle causal. Une garde d’abandon après la boucle d’attente fermerait
précisément ce chemin. Ces fichiers du pipeline diffèrent de c40 : la preuve
des fondations ci-dessus ne qualifie pas ce nouveau raccord.

## Limites à conserver dans la suite

`MemoryBudget` borne les Buffer, pas le RSS ni les piles système du Pool.
`admit` ne réserve pas : sa promesse avant calcul suppose le pilote unique et
l’absence d’allocations étrangères pendant l’étage. `restart_peak` exige une
frontière quiescente. Cloud exige des entrées stables pendant l’appel, puis
possède ses copies privées. Le Pool doit survivre à tous ses appels et callbacks.
Ces clauses sont cohérentes avec les chemins lus ; elles ne justifient aucune
nouvelle réserve sur les anciens défauts d’alias ou de TLS déjà fermés.

`python3 review.py` et `python3 -O review.py` réussissent les mêmes **431 contrôles**
d’intégrité, inventaires et métadonnées, avec sorties identiques. La seule
transition Python du pipeline constate le prédicat sur la sentinelle : elle
ne simule pas une exécution C++. [COMMANDS.json](COMMANDS.json) fixe ce périmètre.
Les 66 sources exactes sont inventoriées avant lecture et après relecture ;
aucune capture source n’a dérivé. `LEDGER.json` et `SHA256SUMS` ferment tous
les fichiers de cette capsule, sans exclure d’inventaire imbriqué.

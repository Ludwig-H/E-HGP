# Contre-audit B — port v3, saturation q4 et tri des niveaux

23 septembre 2026. Lecture indépendante du commit moteur **`e6405952`**
sur `main` (même patch observé brièvement sous un hash pré-rebase), sans
nouveau GCP. Les 27 portes de générateur visibles dans le worktree du
développeur ne font **pas partie de ce commit** et ne sont pas transférées
à son statut. Le reçu G4 R1 reste épinglé à l'ancien `e28296bb`.

## Un gain plausible, encore non qualifié

`ChainOptions::atlas_saturate_deep` raccorde enfin l'arrêt du fragment
q4 dès le certificat strict `inside≥K−1` de la v8. Ce terminal est un
**rejet profond distinct**, jamais un fragment exact tronqué ; le même
minorant peut aussi rejeter q3 au seuil `K−1`. Le port ne crée donc pas,
à lui seul, un contre-exemple d'exactitude. En revanche, son booléen est
initialisé à **`true`** dans `tower_chain.hpp:51`, alors que le commentaire
immédiat dit « désactivée par défaut » et que `Q4LocalOptions` ainsi que la
reprise u18 v8 gardent `false` par défaut. La sonde accepte désormais
`--no-saturate-deep` et publie la valeur, mais **le chemin par défaut a
changé**. Il faut choisir explicitement le contrat v9, corriger le
commentaire et garder un essai off/on apparié (mêmes octets, K5/K10,
s8/10/12, W1/W48) avec catalogue, clés, ordres et digest identiques.
Un simple chrono ou compte de supports ne démontre pas cette identité.

Le tri FULL des niveaux remplace le `stable_sort` exact par un `sort`
filtré seulement sous `FE_TONEAREST`. La représentation validée a
`num∈[0,2^192)` et `0<den<2^127`; ses doubles restent finis et normaux.
Les additions positives et multiplications par `2^64` limitent l'erreur
relative bien en deçà de la marge `2^-46`. Les paires proches retournent
à `compare_exact_level` U320 ; en cas d'égalité exacte, le rang préalable
`by_key` reproduit la stabilité. **Aucune erreur certaine trouvée dans
ce filtre** sous les préconditions annoncées, et la porte arithmétique
ajoute des paires qui le traversent. Une gate de permutation complète
contre le tri exact, sur niveaux égaux de représentations distinctes et
les quatre arrondis/FTZ/DAZ, reste préférable avant de lui attribuer un
gain industriel. Aucun temps G4 de ce port n'est publié.

## Protocole v3 : pas prêt pour une session facturée

Le passage conjoint à `mhgp9_tower_probe_v3` et l'ajout de
`options.atlas_saturate_deep` ferment le premier décalage de champ, mais
**la vraie sonde est toujours rejetée**. Elle émet dans `tower_work`
`meb_accounting` (chaîne) et `meb_supports_by_size` (tableau), tandis que
`tower_worker_v9.py:309–311` exige encore `_count` entier pour **toutes**
les valeurs. Contre-fixture fidèle au schéma réel :
`ValueError: probe counters tower_work`. Le faux résultat du selftest v3
omet ces deux champs ; son succès ne juge pas le raccord.

La nouvelle option n'est contrôlée que comme booléen, sans égalité avec
une valeur prévue : ni `CASE_KEYS`, ni `expected_probe_tail` ne portent
le choix de saturation. Un faux `atlas_saturate_deep=false` est accepté
comme `complete_relative` même si le plan implicite exécute le nouveau
défaut `true`. Épingler le mode dans le plan/argv, ou exiger expressément
une constante documentée, puis comparer au JSON. Une prochaine campagne
off/on ne doit pas être silencieusement confondue avec R1.

Les trois autres verrous du [contre-audit de protocole](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md)
demeurent dans ce commit : le lecteur de session ne recertifie pas les
blobs du commit annoncé ni la provenance du reçu invité, accepte encore
`group_closed=false` pour un cas tué classé `partial`, et ne confronte
pas les sous-chronos à la durée externe. Aucun de ces défauts n'invalide
le reçu historique R1, lié indépendamment à ses sources ; tous empêchent
de promouvoir la nouvelle voie v3 en preuve de contrat. La prochaine
porte est une **vraie petite sortie v3** jugée par le worker, puis des
mutants de schéma/mode/provenance/fermeture/temps, avant G4.

## Suite `0b29b6c3` : défaut FULL statique, mais pas dans le plan G4

Le commit suivant donne à l'API `ChainOptions::tower_static_threads=-1`
le sens « utiliser W fils statiques si W>1 », puis publie dans la sonde
le nombre **effectif**. C'est une politique locale plausible, distincte
de la preuve de gain sur G4. Le worker G4 conserve
`expected_probe_tail(case) = ... --static=<case.static_threads>` et ses
cas standards portent `static_threads=0`. Cette option explicite
**désactive le nouveau défaut** : relancer le plan inchangé mesurerait
encore la voie temporelle, sauf le cas additionnel statique prévu à 48.
Pour mesurer le nouveau défaut sur les cas contractuels, épingler dans
chaque cas la voie statique voulue (ou omettre explicitement l'argument
et vérifier le nombre effectif), puis comparer même catalogue et digest
par ordre. Une ligne locale non versionnée `scene00/K5/W8` sous les
nouveaux défauts annonce bien `tower_static_threads=8` et le digest
historique `67450c64611075b1`, mais elle ne constitue ni reçu apparié,
ni G4, ni preuve générale d'identité.

Le cast `static_cast<int>(W)` dans ce chemin auto demande une garde
représentationnelle `W≤INT_MAX` avant conversion pour l'API C++ directe ;
la CLI borne déjà W à 4096. Il ne s'agit pas de réintroduire un quota de
recherche ou de points.

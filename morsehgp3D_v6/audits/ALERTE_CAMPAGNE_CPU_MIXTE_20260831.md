# Audit des campagnes CPU v6 — confirmation hors échantillon bornée

Date de constat : 2026-08-31

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict courant

Quatre objets doivent rester séparés :

1. `receipts/campagne_decision_20260831/` est **définitivement invalide comme matrice épinglée** parce qu'il mélange deux binaires. Le marqueur terminal ajouté en `9c9ad125` reçoit correctement ce fait.
2. `receipts/campagne_sonde_octaves_20260831/` utilise une copie privée du binaire, avec hash avant/après chaque tuple. Elle s'est terminée par 36/36 codes 0, `DONE`, 36 hashes de sorties vérifiés et 36 stderr vides. Cette correction de provenance est reçue. La capture reste néanmoins **exploratoire**, pas décisionnelle : son lanceur exact n'est pas versionné et l'agrégateur qualifié de « préenregistré avant la campagne » a été écrit et committé après le début de celle-ci.
3. `receipts/campagne_decisionnelle_20260831/`, lancée après `a30c3a98`, est prospective vis-à-vis des scripts mais **pas vis-à-vis des données** : elle rejoue le même binaire privé, les mêmes familles, les mêmes tailles et les mêmes graines que la sonde dont `T_lourde` et le seuil d'octave ont été dérivés. Elle est une réplication de reproductibilité, pas un échantillon confirmatoire indépendant. Son nom et son META ne peuvent donc pas lui conférer un statut décisionnel.
4. `receipts/campagne_confirmation_20260831/` est un vrai échantillon hors
   tailles/graines initiales. Son cœur brut est mécaniquement reçu, mais son
   statut reste **`confirmation_candidate`** : la porte exécutable ne
   déclenche E6 sur aucune famille stationnaire, tandis que les autorités
   écrites fixent encore une autre matrice et que le dossier a été enrichi
   après sa publication terminale.

Il n'était pas utile d'interrompre une seconde fois la sonde : ses compteurs forment un échantillon exploratoire propre du binaire privé. Il n'est pas utile non plus d'interrompre la réplication `a30c3a98`, qui peut vérifier la reproductibilité des compteurs. En revanche, ne produire un verdict E6 confirmatoire que sur un profil hors-échantillon fixé avant lecture, avec de nouvelles graines et/ou tailles qui n'ont servi ni à choisir `T_lourde`, ni à choisir l'octave 10, ni à régler le seuil.

## Ancienne matrice mixte — invalidation reçue, portée diagnostique étroite

Le reçu `cfaf6b41` et `STATUT_TERMINAL.txt` corrigent le décompte : 32 sorties d'ancien schéma, puis 4 sorties de nouveau schéma. La matrice de 36 runs n'est ni une preuve de pente, ni une mesure de coût, ni une preuve de déterminisme.

La relecture post-hoc de 27 sorties homogènes (`terrain_stationnaire`, `scanline_stationnaire`, `uniform`) reste matériellement utile :

- 36/36 hashes stdout du META concordent et les 36 stderr sont vides ;
- les 36 statuts forment exactement la matrice annoncée et rendent tous le code 0 ;
- les 27 sorties retenues ont un schéma unique, des identités fichier/contenu cohérentes et des vecteurs de 16 cases ;
- sur ces 27 sorties, la somme du vecteur `w1` égale `tests_coeur` et la somme du vecteur `seeds` égale le compteur de seeds q4.

La provenance des 32 sorties vers le premier binaire est fortement soutenue par la chronologie et la bascule de schéma, mais elle n'est pas prouvée cryptographiquement par run : l'ancien exécutable n'a pas été archivé et aucun hash binaire individuel n'accompagne ces tuples. La requalification `baseline_sonde_partielle` vit dans `RECU.md`, tandis que le META conserve l'intitulé « campagne de décision E6 » et le STATUS finit par `DONE` ; tout consommateur doit donc exiger aussi `STATUT_TERMINAL.txt`.

## Conclusions post-hoc à réduire

Les calculs numériques affichés dans `ANALYSE_OCTAVES_20260831.txt` ont été recomputés indépendamment et concordent. Deux formulations dépassent cependant les données :

- « uniform 0,97–1,14 partout » omet sans seuil déclaré des octaves non nulles, notamment o9 dont les pentes par graine valent environ 1,659, 0,681 et 2,142. Leur masse est négligeable, mais ce plancher doit être fixé avant la lecture qui le juge.
- « queue superquadratique sur les trois graines » n'est vraie que pour certains bins isolés. Pour scanline o12, le pas 16000→32000 vaut bien environ 4,30 / 10,12 / 5,02. En revanche, la queue agrégée o11+o12 vaut environ 1,61 / 3,87 / 4,43 et la queue terrain o10+o11 2,40 / 1,75 / 3,16. Le coût W1 total est encore moins uniforme. Le cas scanline graine 4/o12 passe de seulement 43 ancres à 2927 : c'est aussi une émergence ou migration de bin, pas encore une loi d'échelle stable.

Formulation défendable : **une amplification superquadratique du bin o12 scanline est observée dans les trois graines sur le seul pas 16000→32000**. L'indépendance à la graine d'une queue agrégée reste à tester.

## Nouvelle capture — binaire privé reçu, préenregistrement réfuté

La nouvelle capture commence à `2026-08-31T13:36:39Z`. Son processus parent est le script non versionné :

```text
/tmp/claude-1000/.../scratchpad/camp_oct2.sh
```

Ce script copie `build/v6/mhgp6` vers `receipts/campagne_sonde_octaves_20260831/bin/mhgp6`, le passe en mode 555 et exécute uniquement cette copie. Le SHA-256 privé `4bbb257c...3359` concorde avec la source au moment de la copie ; `HASHES.txt` le recoupe avant et après chaque tuple. Les portes synthétiques ajoutées en `9c9ad125` passent localement et tuent une altération de la copie privée. Cela ferme la contamination par un rebuild concurrent du chemin partagé.

La fixture nommée « reconstruction concurrente » altère en réalité `$0`, donc la copie privée elle-même ; elle ne remplace jamais la source pendant un tuple. Elle prouve le détecteur de tamper privé, pas causalement l'isolation face à un rebuild de `BIN_SOURCE`. Ajouter le scénario réellement annoncé : remplacer la source après la copie, puis exiger que deux tuples exécutent la même copie et terminent avec les mêmes hashes.

Deux limites empêchent encore le statut décisionnel :

- le lanceur réellement actif est le script de scratch ci-dessus, différent de `bench/campagne_locale.sh` committé à `13:38:48Z`, après le début de la capture ; son contenu et son hash ne figurent pas dans le reçu ;
- `bench/agregateur.py` a été écrit vers `13:41:09Z` et committé en `8157c65d` à `13:42:23Z`, alors que la capture avait commencé six minutes plus tôt et avait déjà produit des sorties. Il ne peut donc pas être qualifié de préenregistré **pour cette capture**, même si son seuil provient de la campagne antérieure.
- le lanceur versionné enregistre séparément les hashes de la source et de la copie sans exiger leur égalité, n'enregistre pas l'état du worktree et dérive le pin du répertoire courant ; une copie prise pendant un rebuild ou issue d'un autre état peut donc finir homogène avec `DONE`.

La porte synthétique `agregateur_gate.py` passe, mais elle démontre l'algorithme sur ses fixtures, pas l'antériorité temporelle du protocole.

## Réplication `a30c3a98` — protocole antérieur, échantillon réutilisé

Le commit `a30c3a98` précède bien la nouvelle capture : commit à
`14:27:21Z`, début gravé à `14:27:31Z`. Il ferme plusieurs défauts de la
première sonde : profil copié, hashes des trois scripts annoncés avant le
premier tuple, copie/source égales, statut terminal invalide refusé, liens et
sous-répertoires rejetés, hashes du binaire et des sorties recoupés. Les trois
portes Python passent indépendamment.

Cette antériorité ne crée toutefois pas un jeu de confirmation. Le profil
`locale_decision_v1` reprend exactement les quatre familles, les tailles
8000/16000/32000 et les graines 3/4/5 de la capture exploratoire ; le binaire
privé porte le même SHA-256 `4bbb257c...3359`. Sur les treize premiers tuples
terminés contrôlés à cette coupe, 13/13 `digest_all` et toutes les lignes hors
temps/RSS sont identiques octet pour octet à la sonde. C'est le comportement
attendu d'un programme déterministe, pas treize observations nouvelles.

Si la capture ferme, le statut honnête est `replication_complete`. Elle peut
prouver que l'isolation du binaire rend les compteurs reproductibles. Elle ne
peut ni confirmer la stabilité de l'octave 10 choisie sur les mêmes sorties,
ni transformer rétrospectivement la médiane de `T_lourde` en verdict E6.

## Réplication terminée — `replication_complete` reçu

La campagne `a30c3a98` s'est terminée naturellement à 15:08:49Z. La
vérification indépendante ferme exactement :

- matrice 4 × 3 × 3, 36/36 codes 0 et un unique `DONE` terminal ;
- 72/72 fichiers réguliers attendus, 36 stdout non vides et 36 stderr vides ;
- binaire privé SHA-256 `4bbb257c...3359`, 36 hashes avant/après homogènes et
  36 hashes de stdout du META recalculés ;
- 36/36 sorties identiques à la sonde après retrait des seules lignes de
  temps et RSS ; tous les digests et compteurs déterministes coïncident.

Le reçu `320299df` porte donc correctement `replication_complete`. Il établit
la reproductibilité de ce binaire sur la même machine ; il ne confirme aucune
hypothèse choisie sur ces mêmes familles, tailles et graines.

## Rectifications du reçu exploratoire à conserver

L'intégrité mécanique de `campagne_sonde_octaves_20260831` est reçue, mais son
`RECU.md` doit encore éviter quatre raccourcis :

- le META grave le HEAD d'exécution `cca9a2d5`, tandis que le reçu annonce
  `cfaf6b41` ; les entrées produit sont équivalentes, pas les arbres Git ;
- les charges 3,91/7,46/9,33 au départ et 9,28/9,91/10,10 à la fin décrivent
  une machine partagée chargée, pas « au repos » ;
- la part passe 2 exacte vaut 0,0425–0,3411 %, donc 0,0–0,3 % à un chiffre ;
  « < 0,3 % » est faux pour certains tuples ;
- `w1/seed_lourde=22–77` divise par toutes les seeds, y compris celles tuées
  avant le scan cœur. Les 31–54 % sont une fraction de population, pas une
  fraction de `W_sweep1`. Il faut ventiler W1 par issue avant toute attribution
  causale.

Ajouter aussi les cinq champs du cadre v6 au reçu lors de sa prochaine mise à
jour documentaire.

## `99bf6723` — progrès réel, porte décisionnelle encore incomplète

Le mode `auto` construit bien le binaire depuis `git archive <pin>` ; les
copies exactes du protocole sont archivées ; les hashes de sorties forment un
ensemble exact ; le profil 10k/20k/40k, graines 6/7/8, est réellement disjoint
de l'échantillon initial. Ces progrès sont reçus.

Ils ne suffisent pas à faire du validateur une autorité décisionnelle :

1. Le mode `auto` n'est pas exigé par `pentes.py`. Un faux binaire, un profil
   mono-famille et un pin artificiel peuvent encore passer la porte
   structurelle.
2. Les copies archivées sont recoupées avec le META, pas avec
   `git show <pin>:<chemin>`. L'agrégateur exécuté n'est pas comparé à sa copie
   archivée.
3. Toute suite de trois doublements est acceptée, y compris 2/4/8, alors que
   `REGIMES.md` interdit une pente à n ≤ 2000. Les doublons de clés et
   l'absence d'un nom de profil canonique ne sont pas refusés.
4. `REGIMES.md` et `PLAN_DE_TESTS.md` réservent encore les pentes à
   8k/16k/32k ; `GRAND_LIVRE.md` et l'en-tête de l'agrégateur parlent encore
   du pas 16k→32k. Le nouveau profil est préenregistré, mais les autorités se
   contredisent.
5. Une émergence produit encore `E6_active=non` ou
   `garde_fou_borne_viole=non`. Le verdict doit être tri-valué : `oui`, `non`
   ou `indetermine`.
6. `mkdir -p OUT.partial`, le `mv` final, le hash puis la relecture des
   sorties, et l'écriture directe d'`AGREGAT.txt` laissent des courses. Un run
   non nul peut aussi recevoir `DONE` et être publié avant son refus tardif.

## Confirmation hors échantillon terminée — cœur reçu, promotion refusée

La capture s'est terminée naturellement à 16:12:41Z. Un snapshot pris juste
après publication permet de fermer mécaniquement :

- matrice 4 × 3 × 3 exacte, 36/36 codes 0, `DONE`, 36 stdout non vides et 36
  stderr vides ;
- 36 hashes avant/après identiques au binaire privé
  `f74a8759c3e5...67ae07e` et 36 hashes de stdout recalculés ;
- profil, lanceur, validateur et agrégateur archivés identiques à leurs blobs
  au pin `320299df` ; le profil existe depuis `99bf6723`, antérieur au début
  15:10:15Z ;
- reconstruction indépendante depuis `git archive 320299df` octet-identique
  au binaire privé ;
- rejeu du validateur et de l'agrégateur archivés sur copie : codes 0, sans
  émergence.

Le cœur brut audité contient 81 fichiers. Le SHA-256 de son manifeste
déterministe — chemins et hashes, en excluant seulement les dérivés
postérieurs `PENTES.txt`, `AGREGAT.txt` et `protocole/__pycache__` — vaut
`a05b9b72ea0150859a6c612fab98a4c7e266ce3a812a7430be6f19dc751f7b93`.

La règle exécutable applique son second pas réel 20k→40k. Les médianes
`W_sweep1 / M_anchor[q4] / T_lourde` sont :

| famille | médianes du pas 20k→40k | verdict exécutable borné |
|---|---:|---|
| terrain stationnaire | 1,220 / 1,325 / 1,308 | `E6_active=non` |
| scanline stationnaire | 1,160 / 1,056 / 1,773 | `E6_active=non` |
| uniform | 1,061 / 1,046 / indéfinie (zéro) | garde bornée non violée |
| eight clusters | 1,423 / 1,317 / 1,505 | garde bornée non violée |

Ainsi, **la campagne ne confirme pas le déclencheur E6 préenregistré**. Ce
résultat négatif interdit de promouvoir E6 à partir de cette hypothèse ; il
ne démontre pas qu'un prototype G16 particulier est inutile, lequel doit
être jugé par ses propres bras causaux et son coût total.

Le statut reste néanmoins `confirmation_candidate`, pas confirmation
formelle, pour trois raisons indépendantes :

1. `REGIMES.md` et `PLAN_DE_TESTS.md` fixent encore 8k/16k/32k, graines
   3/4/5 et les deux pas ; `GRAND_LIVRE.md` et le texte de l'agrégateur
   annoncent 16k→32k, alors que le calcul ci-dessus porte sur 20k→40k.
2. La porte complète du grand-livre n'est pas verte : sur 10k→20k, la
   médiane terrain de W1 vaut 2,03 ; sur 20k→40k, la médiane
   `eight_clusters/P_factor_q2` vaut 2,26. L'agrégateur n'en juge que trois
   termes et le second pas.
3. Le dossier a été muté après publication : `PENTES.txt`, un `__pycache__`,
   puis `AGREGAT.txt` ont été ajoutés. L'agrégat courant reproduit bien celui
   du snapshot (`400533d9...a0895`), mais il n'appartenait pas au terminal et
   le défaut d'immuabilité est réalisé.

La branche tri-valuée reste à corriger même si aucune émergence ne l'exerce
ici. Pour fermer proprement cette candidate sans relancer ses mesures :
aligner les autorités sur le profil effectivement préenregistré sans changer
la règle après lecture, publier les dérivés à côté du cœur et les lier à son
manifeste, puis geler réellement le reçu. Toute nouvelle hypothèse issue de
ces résultats exige une autre campagne prospective.

## Contre-audit du reçu `8ed2dea6`

Le `RECU.md` ajouté après le terminal ne reçoit pas le statut
`confirmation_complete`. Il confond le résultat négatif de la porte bornée
avec la conformité à la doctrine complète, alors que les deux violations de
pente ci-dessus et la contradiction de matrice restent actives.

Trois affirmations factuelles doivent aussi être corrigées :

- « sorties non touchées » et « publication atomique » ne décrivent que le
  cœur à 16:12:41Z ; `PENTES.txt`, `AGREGAT.txt` et `RECU.md` ont été ajoutés
  ensuite dans le même dossier ;
- les heures annoncées 15:56Z et 17:04Z sont fausses : le commit
  `99bf6723` date de 14:47:55Z et le META grave le départ à 15:10:15Z ;
- `build_pin.log` existe encore localement mais est ignoré et n'appartient
  pas aux 83 fichiers du commit. Il n'est donc pas « conservé » dans le reçu
  versionné. Le rebuild indépendant octet-identique compense heureusement ce
  manque pour lier le binaire au pin.

Le titre de commit « queue intermittente » est une interprétation plausible
des excursions, pas le verdict préenregistré. Le seul verdict directement
reçu est plus étroit : absence de médiane ≥2 sur les trois termes du second
pas. De même, « moteur courant sous-quadratique » est faux sans qualification
puisque la porte complète contient encore deux médianes ≥2.

Enfin, `git show --check 8ed2dea6` relève trois espaces finaux dans
`AGREGAT.txt` et une ligne blanche supplémentaire en fin de `PENTES.txt`.
Ne pas réécrire silencieusement les dérivés pour les nettoyer : une correction
doit rester explicitement postérieure et liée au manifeste brut.

GCP non utilisé par le présent audit.

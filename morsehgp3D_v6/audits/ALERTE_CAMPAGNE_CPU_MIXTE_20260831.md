# Suivi des campagnes CPU v6 — provenance réparée, réplication à ne pas confondre avec une confirmation

Date de constat : 2026-08-31

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict courant

Trois objets doivent rester séparés :

1. `receipts/campagne_decision_20260831/` est **définitivement invalide comme matrice épinglée** parce qu'il mélange deux binaires. Le marqueur terminal ajouté en `9c9ad125` reçoit correctement ce fait.
2. `receipts/campagne_sonde_octaves_20260831/` utilise une copie privée du binaire, avec hash avant/après chaque tuple. Elle s'est terminée par 36/36 codes 0, `DONE`, 36 hashes de sorties vérifiés et 36 stderr vides. Cette correction de provenance est reçue. La capture reste néanmoins **exploratoire**, pas décisionnelle : son lanceur exact n'est pas versionné et l'agrégateur qualifié de « préenregistré avant la campagne » a été écrit et committé après le début de celle-ci.
3. `receipts/campagne_decisionnelle_20260831/`, lancée après `a30c3a98`, est prospective vis-à-vis des scripts mais **pas vis-à-vis des données** : elle rejoue le même binaire privé, les mêmes familles, les mêmes tailles et les mêmes graines que la sonde dont `T_lourde` et le seuil d'octave ont été dérivés. Elle est une réplication de reproductibilité, pas un échantillon confirmatoire indépendant. Son nom et son META ne peuvent donc pas lui conférer un statut décisionnel.

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

## Campagne hors échantillon lancée — candidate, pas verdict acquis

`campagne_confirmation_20260831.partial` a démarré à 15:10:15Z au pin
`320299df`, en mode `auto`, avec le profil préenregistré 10k/20k/40k et les
graines 6/7/8. Il n'est pas utile de l'interrompre : les données sont
réellement hors échantillon et le binaire vient d'une archive Git.

Son statut honnête avant audit terminal est `confirmation_candidate`. Si les
36 tuples ferment, l'audit peut vérifier après coup que chaque copie de
protocole correspond au pin et que les courses ci-dessus ne se sont pas
réalisées. Cela préservera la valeur scientifique de la capture. En revanche,
aucun `E6_active=non` n'est recevable en présence d'une émergence, et aucun
verdict formel ne doit être publié avant résolution des contradictions de
profil et validation des mêmes octets par un snapshot immuable.

La prochaine itération du lanceur doit graver `campaign_mode`, n'autoriser que
les deux profils exacts versionnés, échouer dès le premier tuple non nul,
acquérir le temporaire exclusivement et publier sans écrasement. Le validateur
et l'agrégateur doivent consommer une représentation immuable unique.

Cette alerte pourra être absorbée dans `ETAT_COURANT.md` puis supprimée après
le verdict borné sur cette candidate et la fermeture des portes ci-dessus.

GCP non utilisé par le présent audit.

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

## Validateur `a30c3a98` — progrès reçu, chaîne d'autorité encore incomplète

Le validateur tue désormais les contre-fixtures listées au tour précédent et
l'agrégateur se nomme correctement « porte E6 bornée ». Il classe aussi une
transition 0→positif comme émergence et supprime un `AGREGAT.txt` périmé sur
refus. Ces corrections sont reçues.

Il reste une différence entre **enregistrer** une provenance et la **lier** :

- `sha256_lanceur`, `sha256_validateur`, `sha256_agregateur` et le hash du
  profil sont écrits au META mais `pentes.py` ne les vérifie pas ; les scripts
  exacts ne sont pas copiés dans le reçu ;
- le profil externe fourni à `pentes.py` peut être n'importe quel fichier
  compatible. Le hash `autorite_profil` du META n'est pas recoupé et aucun
  contrôle ne lie ce fichier au `pin` annoncé ;
- le `pin` est seulement exigé une fois, sans validation hexadécimale ni
  preuve que le binaire archivé provient de ce commit. Ici le hash du binaire
  est corrélé à un ancien reçu de portes, pas lié par le lanceur courant ;
- les 36 hashes attendus des sorties sont trouvés, mais les lignes de hash
  supplémentaires ou dupliquées dans le META ne sont pas refusées ;
- le dossier de campagne peut préexister et est alimenté en place. Pour une
  autorité décisionnelle, créer un dossier unique inexistant, écrire dans un
  temporaire et publier atomiquement le terminal évite collisions et restes.

Ces points n'empêchent pas d'exploiter la réplication comme diagnostic. Ils
doivent être fermés avant de qualifier un futur reçu d'authentifié de bout en
bout.

## Correction utile à Claude

1. Classer la première sonde `exploratory_complete` et la capture active
   `replication_complete` si leurs contrôles terminaux ferment ; ne produire
   aucun `E6_active` confirmatoire sur ces mêmes 36 tuples.
2. Préenregistrer un **nouveau** profil de confirmation dont les graines sont
   disjointes et, idéalement, dont les tailles sont décalées ; ne lire aucune
   sortie avant d'avoir committé profil, règle, seuils et politique des zéros.
3. Archiver dans chaque reçu les copies exactes du lanceur, du validateur, de
   l'agrégateur et du profil, puis recouper leurs hashes contre un manifeste
   canonique lié au commit.
4. Refuser un dossier de sortie préexistant et publier le reçu depuis un
   répertoire temporaire unique, avec terminal atomique.
5. Garder le nom « porte E6 bornée à trois termes/pas2 » et réserver le
   garde-fou GO à une porte séparée couvrant tous les termes et les deux pas.

Cette alerte pourra être absorbée dans `ETAT_COURANT.md` puis supprimée après réception d'une campagne postérieure au profil committé et d'un validateur qui tue les contre-fixtures ci-dessus.

GCP non utilisé par le présent audit.

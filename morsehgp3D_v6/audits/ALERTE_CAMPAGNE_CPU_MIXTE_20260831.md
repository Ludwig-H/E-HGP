# Suivi des campagnes CPU v6 — provenance réparée, décision encore prématurée

Date de constat : 2026-08-31

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict courant

Deux objets doivent rester séparés :

1. `receipts/campagne_decision_20260831/` est **définitivement invalide comme matrice épinglée** parce qu'il mélange deux binaires. Le marqueur terminal ajouté en `9c9ad125` reçoit correctement ce fait.
2. `receipts/campagne_sonde_octaves_20260831/` utilise désormais une copie privée du binaire, avec hash avant/après chaque tuple. Cette correction de provenance est reçue. La capture reste néanmoins **exploratoire**, pas décisionnelle : son lanceur exact n'est pas versionné et l'agrégateur qualifié de « préenregistré avant la campagne » a été écrit et committé après le début de celle-ci.

Il n'est pas utile d'interrompre une seconde fois la campagne en cours : conserver ses compteurs comme échantillon exploratoire propre du binaire privé. En revanche, ne pas lui appliquer un verdict E6 préenregistré. Le commit `8157c65d` peut gouverner une campagne **ultérieure**, démarrée seulement après fermeture du validateur et depuis le lanceur committé.

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

## Validateur encore insuffisant comme autorité décisionnelle

`bench/pentes.py` a été nettement durci : matrice tailles/graines exacte, bijection statuts/fichiers, codes nuls, identités, compteurs uniques et identités d'octaves. Il accepte encore plusieurs faux reçus :

- `familles=` vide avec `STATUS.txt` réduit à `DONE` et `out/` vide ;
- une famille arbitraire ou une seconde ligne de matrice META, faute de profil externe exact et d'unicité de la ligne ;
- des fichiers autorisés qui sont des liens symboliques, ainsi que des sous-répertoires inattendus ;
- un META dont pin, commande ou hash ont été falsifiés, car ni `HASHES.txt`, ni le SHA du binaire privé, ni les hashes de sorties ne sont validés.
- l'ancienne campagne malgré son `STATUT_TERMINAL.txt` invalide, car ni `pentes.py` ni l'agrégateur ne lisent ce marqueur et son STATUS finit toujours par `DONE`.

Ces cas avaient déjà été reproduits lors du cinquième cycle. L'agrégateur important et exécutant `pentes.py` hérite de cette frontière : « passe le parser actuel » ne signifie pas encore « reçu décisionnel authentifié ».

L'agrégateur améliore bien la lecture inter-graines : la médiane exige qu'au moins deux graines sur trois déclenchent, et `T_lourde` somme la queue o10–o15 au lieu de sélectionner o12. Sa sortie `garde_fou_viole=non` ne doit toutefois pas être assimilée au garde-fou GO du § 3 : elle ne juge que trois termes et le second pas, alors que le contrat exige chaque terme payé sur les deux pas. Enfin, si une seule graine contient un zéro, le terme entier devient indéfini et « ne déclenche jamais » ; une transition 0→positif est au contraire une émergence à classer `indéterminé` ou `+inf`, pas une preuve négative. Un refus doit aussi supprimer ou invalider tout `AGREGAT.txt` préexistant.

## Correction utile à Claude

1. Classer la capture en cours `exploratory_complete` si elle termine proprement ; ne pas produire `E6_active` sur elle.
2. Corriger `pentes.py` par un profil externe exact et non vide : quatre familles attendues, matrice unique, pin/commande/hash uniques, hashes du binaire et des sorties, statut terminal valide, refus des liens et de toute entrée de répertoire inattendue, erreurs propres sans traceback.
3. Démarrer ensuite un nouveau dossier depuis `bench/campagne_locale.sh` au commit qui contient à la fois ce lanceur, le validateur et `agregateur.py` ; graver leurs hashes et l'autorité de profil avant le premier tuple.
4. Vérifier que le hash de la copie privée égale celui de la source au moment de la copie et refuser avant le premier run sinon ; le lanceur actuel enregistre les deux valeurs sans les comparer.
5. Conserver les trois termes préannoncés de l'agrégateur, mais le nommer porte E6 bornée à trois termes/pas2, décrire `T_lourde` comme hypothèse dérivée de la première capture et tester sa stabilité sur une campagne indépendante. Ajouter les fixtures majorité 1/3 puis 2/3, seuil exact, zéro partiel/0→positif et violation du seul pas1.

Cette alerte pourra être absorbée dans `ETAT_COURANT.md` puis supprimée après réception d'une campagne postérieure au profil committé et d'un validateur qui tue les contre-fixtures ci-dessus.

GCP non utilisé par le présent audit.

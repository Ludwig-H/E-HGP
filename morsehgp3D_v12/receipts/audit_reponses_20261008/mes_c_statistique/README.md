# MES-C : portée des droites et du régime chaud

Lecture du protocole livré **83ed7620d**, avant lecture de résultats MES-C.
Les critères C1/C2 restent ceux annoncés ; ce reçu ne propose aucune nouvelle
règle d'adoption. [capture.json](capture.json) épingle onze sources et le seul
manifeste de données lu. Aucun moteur, accès distant ou payload XYZ/IDs.

**C1 juge une ordonnée à l’origine OLS, C2 une pente OLS ; leur réussite ne prouve pas
une borne par nuage.** Le pilote ajuste `t_i = a + b n_i + e_i` aux médianes
chaudes, une observation par nuage, séparément par voie/K/fils/groupe. Les
conditions des moindres carrés donnent `Σe_i = Σn_i e_i = 0`, aucune borne sur
`max e_i` ou `max(t_i/n_i)`. Les quatre sous-familles réelles sont réunies :
la pente décrit des scènes de tailles et géométries différentes, pas le coût
marginal d'agrandir une scène fixée. La fréquence des tailles dans la cohorte
pondère implicitement l'ajustement. L'ordonnée à l'origine extrapole à zéro site,
hors domaine mesuré ; elle n'identifie pas un poste physique, notamment le Pool.

[check.py](check.py) confronte un oracle `Fraction` à la seule fonction `fit`
extraite du pilote épinglé. **Exemples algébriques inventés, aucun chrono :**

- `t=1 ms+3 µs×n` respecte C1/C2, sans résidu ; à 100 sites, `t/n=13 µs`.
- Pour `n=(100,1000,10000)` et `t=(0,2;7,1;31,7) ms`, la droite vaut exactement
  `a=1,9 ms`, `b=3 µs/site`. Ses résidus `(-2;2,2;-0,2) ms` sont orthogonaux à
  1 et n. C1/C2 passent, mais le point à 1000 sites dépasse même
  `2 ms+seuil_C2×1000 ≈ 5,727 ms`.
- La fonction `t=n² ns` sur 100/200/300 sites a une ordonnée à l’origine négative
  (`−100000/3 ns`) et une pente de 400 ns/site. Cela reste un ajustement
  descriptif valide ; l'ordonnée à l'origine n'est pas un coût fixe physique négatif.

Le libellé général de `docs/MESURE.md:43`, « coût par site jamais supérieur »,
est donc plus fort que le critère C2 livré. Un verdict C2 reste à nommer
« pente OLS sous le seuil annoncé », sans convertir sa réussite en garantie
uniforme ni modifier le juge après les résultats.

Le seuil figé est **241 300 000 / 64 740 = 3727,2165585 ns/site**. Le reçu K
donne une médiane des 37 médianes de temps de 241 309 401 ns et une médiane des
sites de 64 740, portées par **deux trames différentes**. Ce seuil est un rapport
de deux médianes, arrondi au numérateur, pas une pente du régime principal ni
le ratio d'une trame représentative. La médiane des 37 rapports temps/sites
vaut 3513,4660205 ns/site ; leur maximum 4872,863411 ns/site. Ces diagnostics
historiques n'en changent pas la valeur. C2 compare volontairement la pente
CPU des petits nuages au régime principal **appareil** mesuré en K ; il ne
certifie pas le budget principal de 100 ms.

**Le chaud est celui d'une tournée résidente.** Le manifeste `bed8fa90…`
contient 132 réels + 15 synthétiques sains = **147 nuages par Session** ;
12 difficiles sont séparés. Les «145» du README décrivent l'essai local,
pas la cohorte de ce manifeste. Avec trois tours, le nuage i apparaît aux
passes `i`, `i+147`, `i+294` : 146 autres nuages entre deux prises, 441 passes
par configuration. Sa valeur chaude est la médiane de **deux** temps, donc
leur moyenne arithmétique. Le premier tour n'est pas 147 démarrages à froid :
son dernier nuage a déjà été précédé de 146 calculs dans ce processus.

La sonde charge toutes les entrées avant les passes ; Pool et, sur appareil,
contexte de catalogue persistent. Chaque passe reconstruit Cloud/index/C/G/TMVR,
puis détruit ses sorties. Le cache de blocs du budget hôte est **0** par défaut
et MES-C ne passe aucun `--cache`. Validation et digest de chaque passe,
libération, lecture des fichiers et création du Pool/contexte sont hors mur.
Les calculs hors mur restent entre les prises et peuvent influencer l'état de
la machine ; aucune correction de cache ou de température n'est déduite.
Ce régime convient à une latence en Session après une tournée ; il n'établit
ni une latence de processus froid par nuage ni celle d'un même nuage répété
immédiatement. Le mur seul ne mesure pas le débit de la boucle avec validation.

**Comparaison MES-P v11 : descriptive sous frontières explicites.**

| Dimension | MES-P v11 gelée `ac081a06f` | MES-C v12 `83ed7620d` |
|---|---|---|
| Répétition | processus neuf par nuage, passes consécutives | processus par configuration, tournée de 147 nuages |
| Agrégat des lots G/H | médiane de 3 passes chaudes sur 4 | médiane de 2 sur 3 tours |
| Mur | commence après `prepare_cloud`, inclut index/domaine/forêt FULL | inclut `prepare_cloud` dans P, puis C/G/TMVR FULL |
| Pool | création hors mur, gardé entre passes | idem, gardé entre nuages |
| Cache de blocs hôte | 4 Gio, masque CPU 802811 contenant le bit 524288 | 0, faute d'option `--cache` |
| Publication/contrôle | détails domaine de la dernière passe dans le mur ; sérialisation finale hors mur | JSON, validation et digest par passe hors mur |

Les noms/comptes réels K5/W48 correspondent au manifeste courant : **132**
succès dans le lot G, **123** dans H, dont la coupure et les exclusions restent
publiées dans [le reçu H](../../audit_reponses_20261007/session_h_mes_p/README.md).
Une comparaison par scène doit afficher l'intersection choisie et les exclus,
conserver K/fils/u21, et confirmer séparément l'identité des entrées ; égalité
des noms/comptes ne prouve pas l'égalité des coordonnées. Une évolution des
droites entre ces régimes n'est pas une mesure causale du seul moteur ni du
coût du Pool. Ne pas soustraire P v12 pour prétendre retrouver exactement la
frontière v11 : P comprend aussi l'index.

Diagnostics préconisés **avant résultats**, sans nouveaux seuils : conserver
les deux points chauds, leur maximum et leur écart ; publier par groupe les
couples `(n,t)`, les résidus, leur maximum positif et absolu, et `max(t/n)` avec
le nom du nuage. Ajouter les tailles/effectifs et les maxima par classe de
taille pour lire la droite. Garder le tour et la position du nuage : ils permettent
de voir une dérive entre tours sans la transformer en effet de taille. Pour
comparer des configurations, utiliser la cohorte commune explicite ; ni les
refus ni les non joués ne deviennent des temps nuls. Ces diagnostics complètent
le verdict préannoncé et ne le remplacent pas.

Rejeu léger normal/−O, sorties identiques dans [resultat.json](resultat.json) :

```sh
python3 -B check.py --repo /workspaces/E-HGP --manifest CHEMIN/bundle_manifest.json
python3 -B -O check.py --repo /workspaces/E-HGP --manifest CHEMIN/bundle_manifest.json
```

Aucun résultat MES-C lu, aucune nouvelle mesure moteur, aucun état de constat
modifié. Admission des futurs bruts et contrôle des cohortes : travail séparé.

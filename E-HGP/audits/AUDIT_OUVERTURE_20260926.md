# Audit de l'ouverture de E-HGP — 26 septembre 2026

> **Cible épinglée** : commit `f44a8db0372189dba2ede3a55bbce669aa74baab`, lu dans
> un worktree détaché dédié. Tout ce qui est jugé ici est le contenu de `E-HGP/`
> à ce commit.
> **Cadre** : `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.
> **Cet audit ne certifie rien.** Il motive des corrections. Sa valeur est dans ce
> qu'il réfute.

## 0. Verdict en trois lignes

1. **Le calcul tient.** Tout ce qui a pu être relancé se reproduit, souvent chiffre
   par chiffre, et les briques mathématiques centrales résistent à des attaques
   indépendantes : miniball exacte, naissances topologiques, encadrement du noyau
   rampe, fait de segment, gradient du niveau de Fermi, sûreté du certificat de
   séparation.
2. **La comptabilité ne tient pas.** Aucun reçu n'est ancré à un commit qui
   contienne le chantier ; le document de mesure le plus volumineux ne cite aucun
   reçu ; plusieurs chiffres publiés sont cités après leur propre retrait, ou
   n'apparaissent dans aucun agrégat.
3. **Les portes ne gardent pas ce que le chantier revendique.** Sept constats
   bloquants, dont deux faux positifs sur des fixtures **gravées par le chantier
   lui-même**, et quatre vacuités prouvées par exécution.

Le chantier reste `not_claimed`, ce qui est cohérent. Mais plusieurs phrases de
ses documents, lues seules, revendiquent davantage que ce que ses portes
garantissent : c'est l'objet principal de cet audit.

## 1. Méthode

Cinq surfaces attaquées en parallèle, chacune suivie d'une contre-attaque (un
audit faux fait corriger ce qui marche et laisse passer ce qui casse) :
noyau exact, moteur et certificats, théorie entropique, documents contre reçus,
portes et mutants. Plus quatre vérifications conduites directement par
l'auditeur, avec du code n'important rien du chantier quand c'était possible.

Aucun fichier du dépôt n'a été écrit pendant l'attaque ; scripts et sorties dans
les répertoires de bloc-notes `audit_*`.

## 2. Ce qui a résisté

C'est la partie la plus importante du rapport, parce qu'elle dit ce sur quoi on
peut bâtir.

| brique | attaque subie | résultat |
| --- | --- | --- |
| boule englobante minimale exacte | 5 913 sous-ensembles en $d=1,2,3,5,20,50$, nuages colinéaires, cosphériques, dupliqués, coordonnées rationnelles ; certificat primal/dual **indépendant de tout algorithme** plus un programme quadratique dual réécrit | **0 désaccord**, aucune `RuntimeError` |
| catalogue sous-provisionné | `max_support` volontairement trop petit | **300 refus explicites, 0 valeur fausse** : le catalogue refuse au lieu de mentir |
| naissances topologiques | 2 732 naissances recalculées par parcours en largeur indépendant, dont 2 023 sur 140 nuages à coordonnées très serrées (6 900 collisions de niveau, 1 964 fusions au même niveau qu'une apparition) | **0 écart** |
| idem, par l'auditeur | code entièrement réécrit, autre définition de naissance, $n=8$, 3 graines, $d=2,3,20,100$ | **48 valeurs sur 48**, plages comprises ; $d=100$ donne exactement $\binom82,\binom83,\binom84$ |
| tour d'ordre 1 | juge extérieur `scipy.cluster.hierarchy.linkage` | **28 accords, 0 désaccord**, $d$ de 1 à 60, doublons, colinéaires, $n=1$, $n=2$ |
| théorème A (encadrement rampe) | 6 480 cas rationnels exacts jusqu'en $d=200$, avec anti-vacuité | tient ; décalage $\varepsilon$ **optimal** confirmé |
| théorème B (gradient) | différences finies jusqu'en $d=200$ | tient |
| Fait 5, Fait 6, $c(\rho)$ | recalcul symbolique et numérique | Fait 5 vrai à 30 chiffres ; constante du Fait 6 **atteinte** ; $c(\rho)$ exact |
| fait de segment | échantillonnage dense, droites parallèles, croisements triples | tient |
| tour à témoins | reçu rejoué **bit à bit**, puis étendu par l'auditeur à **27 024 paires** | 2 856/2 856 reproduit, **0 violation** sur 27 024 |
| sûreté du certificat de séparation | preuves refaites ; 244 certificats confrontés à une ultramétrique recalculée par l'auditeur sur cinq familles dont cosphériques et doublons ; 12 cellules par la surface moteur | **0 faux certificat** |
| algèbre spectrale $\Theta=\sum G(\lambda_i)(v_i^{\top}\Delta)v_i$ | quadrature indépendante | exacte à $2{,}2\cdot10^{-13}$ |
| les 142 portes | rejouées en mode normal et sous `-O` au commit épinglé ; scan **AST** du mot-clé `assert` ; comptage des 436 sites d'assertion | code 0 ; **zéro `assert`** ; les 436 sites sont tous réellement exécutés |
| les cinq codes de sortie | atteints un par un par l'auditeur, y compris le code 1 en substituant un moteur faux | les cinq sont atteignables et corrects |
| six tables de `concentration.py` | rejouées | reproduites à la décimale, exposants $1{,}021$, $1{,}160$, $1{,}959$, $2{,}002$ compris |
| immuabilité des reçus | `git log` sur `receipts/` | **confirmée** : toutes les entrées sont des ajouts, aucun reçu modifié |

## 3. Constats BLOQUANTS

### B1 — Faux positif du catalogue critique, sur la fixture gravée du chantier

`critical.py` affirme qu'en un point fixe le rang fermé vaut $m$ « hors
dégénérescence cosphérique », donc que $m=k$ donne un point critique d'indice 0.
**Faux sur le carré de côté 10**, qui est une fixture gravée du chantier : à
masse $m=3$ la descente rend le centre $(5,5)$, niveau 50, `closed_rank = 4`,
alors que le catalogue exact à rang fermé 3 est **vide** — un faux positif sur
un. La clause d'exclusion du code ne parle que des observations situées **hors**
de $N(y)$ ; ici les quatre sommets sont cosphériques et $N(y)$ n'en contient que
trois, donc le code se croit dans le régime sûr.

**Conséquence** : la ligne d'état « points fixes de la descente MEB-Lloyd =
sphères critiques, zéro faux positif — démontré et mesuré » est **fausse** comme
énoncé universel. Fixture : `F-AUD-1`.

### B2 — `conv` n'est pas `relint conv` : le pont de criticité est plus faible qu'annoncé

Le corollaire B1 affirme que la condition de point fixe est « exactement » la
condition $c\in\mathrm{relint}\,\mathrm{conv}(U)$ de la spécification. **Faux** :
pour $X=\lbrace(-1,0),(1,0),(0,1),(10,0)\rbrace$ et $m=3$, le point $(0,0)$ est un
point fixe de rang fermé $s=3=m$, de coordonnées barycentriques $(1/2,1/2,0)$ :
il est dans $\mathrm{conv}(U)$ et **pas** dans $\mathrm{relint}\,\mathrm{conv}(U)$,
donc **rejeté** par le critère de la spécification, alors que 60 000 sondes
voisines montrent qu'il s'agit bien d'un minimum local de $a_3$, c'est-à-dire
d'une naissance. Fréquence mesurée en rationnels : 2/240 en $d=2$, $m=3$.

Le code écrit `conv`, le document écrit `relint conv`, et les deux ne
coïncident pas sur les configurations dégénérées. Fixture : `F-AUD-2`.

### B3 — La porte du certificat de séparation est verte par vacuité

`tests/test_separation.py` annonce : « un certificat ne doit jamais affirmer une
séparation qui n'a pas lieu ; une seule violation est un échec ». Un mutant
réellement **non sûr** survit : `Ran 5 tests — OK`, code 0, alors qu'une sonde
directe lui trouve **41 668 certificats acceptés à des niveaux strictement
au-dessus du niveau de fusion exact** (contre 0 pour l'original).

La cause est structurelle : la porte compare le niveau certifié à l'oracle, or ce
niveau **est** le majorant à témoins, qui coïncide déjà avec l'exact sur la
quasi-totalité des paires. Un certificat faux rend donc quand même le bon
nombre. La porte valide l'affirmation « le majorant est exact », pas la sûreté du
certificat. Fixture : `F-AUD-4`.

La sûreté du certificat, elle, reste **confirmée par ailleurs** (preuves refaites,
244 certificats attaqués par l'auditeur, 12 cellules par la surface moteur, 0
faux). Ce qui est cassé est la **porte**, pas le théorème.

### B4 — « toutes les paires certifiées à l'ordre 1 en $d=20$ » est faux

Une quatrième graine suffit : à $d=20$, ordre 1, $n=7$, la couverture vaut
21/21, 17/21, **9/21**, 21/21, 15/21, 17/21 selon la graine, soit une moyenne de
$15{,}7/21$ et non « toutes ». Agrégé : $385/441=0{,}873$ à $d=20$ et
$325/441=0{,}737$ à $d=3$.

De plus le mot de statut est fautif : un compte 21/21 est **mesuré**, jamais
« démontré ». Ce qui est démontré est le certificat de tranche ; le fait que
toutes les paires le reçoivent ne l'est pas. Fixture : `F-AUD-5`.

### B5 — Le drapeau de point fixe est un invariant mort

`critical.py:96` produit `"fixed": center == position` et **personne ne le lit** :
ni les six fichiers de `tests/`, ni les neuf de `bench/`, ni le reste de `src/`.
Rien n'interdit donc à une descente non convergée d'entrer dans le catalogue.
Mesure : avec `max_steps=1`, la descente publie **65 centres au lieu de 63** à la
masse 4 ($n=12$, $d=5$, graine 211) — deux faux positifs — et **les 142 portes
passent, code 0**.

### B6 — Aucun reçu n'est ancré à un commit contenant le chantier

Constat de l'auditeur, confirmé et aggravé par la surface documents.

* `receipts/ouverture_20260925/MANIFESTE.json` épingle
  `a74e90f22167105cdba90b6850f0597f1a01a329`, commit dans lequel `git ls-tree`
  compte **zéro fichier `E-HGP/`**. Son propre label cite un **troisième**
  commit, `273c33f7c`, lui aussi sans `E-HGP/`.
* **12 des 31 empreintes** de ce manifeste ne correspondent plus au commit
  épinglé, dont `gate_engine.py`, `concentration.py`, `clustering_compare.py`,
  `spectral_tower.py`, `fast_point_tower.py` et les trois fichiers `spectral/`.
  Les « code 0 » consignés ne concernent donc pas le code audité. Une empreinte
  (`clustering_compare.py`) ne correspond à **aucune** version connue du fichier.
* **Sept des huit autres fichiers de reçu** ne portent ni pin, ni commande, ni
  hash. Le huitième porte une commande avec un `--features $m` non développé,
  donc non rejouable telle quelle.
* `make_receipt.py` promet « l'état de l'arbre » et n'enregistre aucune clé de
  propreté : un reçu peut être fabriqué depuis un arbre sale sans que cela
  apparaisse.

L'immuabilité, elle, est **confirmée**. C'est l'ancrage qui manque, et c'est la
seule chose qui distingue un reçu d'un fichier de log.

### B7 — « Chaque porte échoue si elle n'a pas effectivement comparé » est réfuté

Quatre vacuités prouvées par exécution :

1. `tests/test_separation.py` : `assertGreater(certifies, 0)` avec 36 certificats
   mesurés — en n'en laissant passer qu'**un** sur toute la campagne, la porte
   reste verte (ratio $2{,}8$ pour cent, exactement le seuil que le chantier
   qualifie lui-même de décoratif ailleurs) ;
2. `bench/births_vs_dimension.py --extent 1` : les lignes $d=2$ et $d=3$
   n'affichent que des tirets et la porte rend **0**, alors que la conclusion
   « $O(n)$ en $d=2$ » vit précisément sur ces lignes ;
3. `bench/scale_space.py --dims ''` : **code 0 avec une table vide**, le plancher
   `--min-starts` étant placé à l'intérieur de la boucle des dimensions ;
4. `bench/concentration.py --table selftest` : quatre des huit planchers annoncés
   ne sont pas évalués, la porte imprime « taux de certification : 0.000000 » à
   côté de « code 0 », et elle abaisse elle-même son propre plancher.

## 4. Constats MAJEURS

| # | constat | où |
| --- | --- | --- |
| M1 | L'en-tête de la porte annonce l'invariance du **digest** par permutation : **fausse** (six digests distincts sur six permutations de `[(0,), (1,), (3,)]`). Ce qui est invariant, et ce que la porte vérifie effectivement, est `levels` et `merge_levels` | `tests/test_tour_exacte.py:10-11` |
| M2 | `merges` et `merge_levels(k)` pour $k\geq2$ ne sont **pas** le dendrogramme de $\pi_0$ : le balayage applique naissances puis liens à l'intérieur d'un même niveau, donc il déclare des multifusions entre composantes qui n'ont jamais existé dans $\pi_0$ | `exact/tower.py:164-231` |
| M3 | Le juge de grille **n'encadre pas** la vérité et son critère d'auto-validation est faux : `[(0,0),(3,0)]`, ordre 1, niveau 2, `steps=3` donne bas = haut = 1 (« grille assez fine ») alors que la vérité est 2 | `exact/grid_judge.py:88-103` |
| M4 | Fait 3 : $\mu$ n'est **pas** unique. Pour $e=(0,0,10)$, $\varepsilon=1$, $k=2$, tout $\mu$ de $[1,10]$ convient. Le code le dit correctement, le document est plus faible que son propre code | `docs/REGULARISATION_ENTROPIQUE.md:81-99` |
| M5 | La forme close spectrale est **réfutée en général** : elle exige que $(t-1)/(\rho t+1-\rho)$ appartienne au span des descripteurs pour **tout** $\rho$, et non la bonne spécification usuelle. Contre-exemple : $p=N(0,1)$, $q=N(0,4)$, $\varphi=(1,x,x^2)$ donne $\Theta=(0{,}4686;0;-0{,}2473)$ contre $(\log2;0;-3/8)$, alors que le log-rapport est **exactement** dans le span. L'algèbre, elle, est exacte | `docs/REGULARISATION_ENTROPIQUE.md:143-145` |
| M6 | Le niveau de Fermi **logistique** calculé par le dépôt est faux de l'ordre de $\gamma/2$ dès que $\varepsilon<\gamma/73{,}5$ : la tolérance du solveur est relative | `soft/fermi.py:57-109` |
| M7 | `best_offset` n'est pas optimal : quatre points colinéaires donnent un supremum 1 sur l'intervalle ouvert là où le code rend $1/4$ (rapport 4). La sûreté n'est pas en cause, l'optimalité l'est | `engine/separation.py:35-40` |
| M8 | Le port flottant n'est **pas** un minorant certifié : à l'échelle $10^8$ il viole la borne basse et son auto-certification **ment** (84 couples déclarés exacts, un seul l'est ; à $10^9$, zéro) | `engine/fast_point_tower.py:52-62` |
| M9 | L'exposant $0{,}75$ est cité **trois paragraphes après son propre retrait**, dans le même document ; la valeur mesurée de remplacement est $1{,}021$. Les deux comptes cités sont les estimations échantillonnées que le document déclare fausses ailleurs (exacts : $1{,}38\cdot10^4$ et $1{,}26\cdot10^6$). L'exposant $1{,}96$ est confirmé ($1{,}959$) | `docs/OBSTRUCTION_GRANDE_DIMENSION.md:144-149` |
| M10 | « $-0{,}148$ à $-0{,}275$ » : $-0{,}275$ n'apparaît dans **aucun** agrégat ; et le $-0{,}06$ attribué à la campagne de 200 cellules vient de celle de 48 | `docs/MOTEUR_ET_COUTS.md:444` |
| M11 | Devroye–Gudmundsson–Morin **mal attribué** : leur article porte sur le degré **maximal** dans le plan, $\Theta(\log n/\log\log n)$, et ne dit rien de $2^{d-1}n$. Le résultat de taille attendue est Devroye seul, 1988 | `docs/OBSTRUCTION_GRANDE_DIMENSION.md:175` |
| M12 | `tests/test_judge_gamma.py` code en dur deux chemins **absolus** : lancée seule, la porte juge l'arbre de `/workspaces` et non celui où elle vit. Une copie dont le sujet lève à l'import passe 14/14 ; et `reference/` n'est jamais réellement utilisé en mode `discover` | `tests/test_judge_gamma.py:24-28` |
| M13 | Cinq portes de `bench/` sortent avec le **code 1** — celui du désaccord du juge — sur une simple option malformée, là où la doctrine exige 2 | `bench/{births_vs_dimension,empty_ball_fraction,scale_space,witness_campaign,spectral_tower}.py` |
| M14 | `docs/MESURES_CONCENTRATION_20260925.md` : 1 115 lignes, 207 valeurs numériques, 864 520 boules revendiquées, **aucun reçu** | document entier |
| M15 | Deux « portes » comparent une constante du module à un littéral du même fichier | `tests/test_spectral.py:525` et `:905` |
| M16 | `families.fermi_level` accepte `masse = n` avec le noyau logistique et rend un niveau **fini** alors que l'équation de masse n'a aucune solution | `soft/families.py:109` |
| M17 | La campagne à témoins **écarte silencieusement** tout nuage à positions dupliquées : le chiffre 2 856/2 856 n'a jamais vu de doublon, alors que le profil du chantier annonce les gérer | `bench/witness_campaign.py:176` |
| M18 | Le « zéro faux positif » du catalogue critique n'est mesuré par **aucun exécutable** : le seul juge de catalogue du dépôt n'est appelé que par `scale_space.py`, avec un prédicat **différent** de celui de la spécification | `README.md:146`, `docs/MOTEUR_ET_COUTS.md:47` |

## 5. Réponse aux sept verrous de `QUESTION_AUDIT_OUVERTURE_20260925.md`

| verrou | verdict |
| --- | --- |
| Q1 obstruction de taille | **confirmée**, par trois chemins dont un entièrement indépendant (48 valeurs sur 48). Mais M9 : un exposant retiré est encore cité, et les comptes cités sont les estimations échantillonnées |
| Q2 descente MEB-Lloyd | **réfutée sur le « zéro faux positif »** (B1, B5, M18) ; la décroissance et la terminaison tiennent en substance mais la preuve écrite est incomplète (la stricte décroissance vient de l'unicité de la boule englobante, il faut l'écrire) |
| Q3 majorant à témoins | **confirmé et renforcé** : 2 856/2 856 rejoué bit à bit, étendu à 27 024 paires sans une violation. Réserve M17 : aucun doublon de position dans le corpus |
| Q4 certificat de séparation | **sûreté confirmée** (0 faux certificat sur tous les chemins d'attaque), **porte réfutée** (B3), **optimalité réfutée** (M7), **quantificateur « toutes » réfuté** (B4) |
| Q5 les trois énoncés réfutés en chemin | **confirmés** tous les trois ; Fait 5 vrai à 30 chiffres, constante du Fait 6 atteinte, fixture F1 correcte |
| Q6 lien avec le cadre de Bach | **algèbre confirmée, identité réfutée en général** (M5). Le lien est plus faible que ce que le document écrit, et l'hypothèse requise n'est pas la bonne spécification usuelle |
| Q7 ce qui reste ouvert | inchangé, et il faut y ajouter la calibration du port flottant (M8) et la sûreté numérique du niveau de Fermi logistique (M6) |

## 6. Corrections demandées, par ordre

À la charge du développeur ; l'auditeur ne corrige pas.

1. **B1, B2, B5, M18** : réécrire la propriété de criticité de `critical.py` avec
   la distinction `conv` / `relint conv` et une clause de cosphéricité qui couvre
   l'intérieur de $N(y)$ ; **lire** le drapeau `fixed` et refuser un candidat non
   convergé ; brancher un juge de catalogue exécutable sur le prédicat de la
   spécification ; retirer « zéro faux positif » des documents jusqu'à ce qu'une
   porte le garde.
2. **B3** : refaire la porte du certificat de séparation pour qu'elle confronte le
   certificat à une vérité **indépendante du majorant** ; graver `F-AUD-4` et
   vérifier que le mutant M1 est tué.
3. **B4, M10, M9** : corriger les quantificateurs et les statuts. « toutes les
   paires » devient une distribution par graine ; « démontré » devient « mesuré ».
   Retirer $-0{,}275$ et l'exposant $0{,}75$.
4. **B6** : `make_receipt.py` doit refuser si le `HEAD` lu ne contient pas le
   chantier, hacher après la dernière écriture, et enregistrer la propreté de
   l'arbre. Publier un reçu corrigé dans un **nouveau** dossier ; les anciens
   restent.
5. **B7, M13, M15** : planchers recalés juste sous la mesure, refus avant calcul
   sur option malformée (code 2), suppression des portes qui comparent un littéral
   à lui-même.
6. **M2, M3, M1** : dire dans les documents que `merges` n'est pas le dendrogramme
   de $\pi_0$ pour $k\geq2$, ou le corriger ; retirer du juge de grille son
   critère d'auto-validation ; corriger l'en-tête de la porte sur le digest.
7. **M5, M6, M4, M16, M8** : ajouter les hypothèses manquantes de la forme close
   spectrale ; corriger la tolérance du solveur de Fermi ; dire que $\mu$ n'est
   pas unique ; refuser `masse = n` avec un noyau à support infini ; retirer le mot
   « certifié » du port flottant et publier son seuil d'échelle.

## 7. Fixtures permanentes

Les contre-exemples minimaux sont gravés dans
[`FIXTURES_AUDIT_20260926.md`](FIXTURES_AUDIT_20260926.md). Conformément à la
doctrine du dépôt, une contradiction devient une fixture avant que le travail
continue.

## 8. Ce que l'audit n'a pas attaqué

* Le **théorème 2 du manuscrit** lui-même : l'oracle a été utilisé comme vérité
  par les surfaces moteur et documents. Seule la surface noyau a écrit un
  troisième juge, et sur les partitions seulement.
* Dix des seize commandes de `MESURES_CONCENTRATION` n'ont pas été rejouées : les
  tableaux des § 3.2 à 3.5, 4, 5 et 6 restent donc non reproduits par cet audit.
* Les références externes du corollaire A1 (Morozov–Beketayev–Weber,
  Chazal–Cohen-Steiner–Glisse–Guibas–Oudot) n'ont pas été ouvertes.
* Le moteur flottant `fast_point_tower.py` n'a **aucune porte** dans le dépôt :
  l'audit l'a attaqué, mais rien ne le garde entre deux audits.

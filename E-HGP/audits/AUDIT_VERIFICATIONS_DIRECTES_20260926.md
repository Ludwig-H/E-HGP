# Vérifications conduites directement par l'auditeur — 26 septembre 2026

> Cible épinglée : commit `f44a8db0372189dba2ede3a55bbce669aa74baab`.
> Annexe de [`AUDIT_OUVERTURE_20260926.md`](AUDIT_OUVERTURE_20260926.md).
> Scripts : `naissances.py`, `ordre1.py`, `certif.py` (répertoire de bloc-notes
> `audit_moi`, hors dépôt). GCP non utilisé.

Un auditeur qui ne fait que déléguer ne juge rien. Les sept vérifications
ci-dessous ont été conduites à la main, avec du code n'important rien du chantier
quand c'était possible.

## A. Ce que l'auditeur a vérifié lui-même

Quatre vérifications conduites directement, avec du code écrit pour l'audit et
n'important rien du chantier quand c'était possible.

### A.1 — La table des naissances : confirmée par un code entièrement indépendant

L'obstruction de taille, qui est le résultat central du chantier, repose sur le
compte des naissances topologiques. J'ai réécrit ce compte de bout en bout :
boule englobante minimale par énumération de supports avec certificat
barycentrique, $\Gamma_k(a)$ construit explicitement à chaque niveau, composantes
par parcours en largeur, et une définition de naissance **différente de celle du
chantier** (composante ne contenant aucun sommet actif au niveau précédent, là où
`tower.py` maintient l'âge d'une composante par union). Arithmétique rationnelle
exacte, aucun import de `ehgp`.

Résultat sur $n=8$, trois graines, $d\in\lbrace2,3,20,100\rbrace$, ordres 1 à 4 :
**les 48 valeurs coïncident avec la table publiée, plages comprises**.

| $d$ | $k=1$ | $k=2$ | $k=3$ | $k=4$ |
| --- | --- | --- | --- | --- |
| 2 | 8 / 8 / 8 | 12 / 9 / 11 | 9 / 9 / 8 | 9 / 8 / 8 |
| 3 | 8 / 8 / 8 | 14 / 15 / 13 | 15 / 17 / 15 | 16 / 14 / 16 |
| 20 | 8 / 8 / 8 | 27 / 26 / 27 | 50 / 40 / 46 | 52 / 40 / 49 |
| 100 | 8 / 8 / 8 | **28 / 28 / 28** | **56 / 56 / 56** | **70 / 70 / 70** |

En $d=100$ les trois graines donnent exactement $\binom82$, $\binom83$ et
$\binom84$. **Verdict : confirmé.** Reproduction :
`scratchpad/audit_moi/naissances.py`.

### A.2 — L'ordre 1 : confirmé par un troisième chemin, scipy

Le chantier affirme que la tour d'ordre 1 est exactement le dendrogramme de
liaison simple euclidien, de niveaux $\left\Vert x_i-x_j\right\Vert^2/4$. Le
chantier le vérifie contre son propre arbre couvrant minimal, ce qui ne prouve
rien de plus que la cohérence de deux implémentations voisines. J'ai donc pris un
juge extérieur : `scipy.cluster.hierarchy.linkage(method='single')`.

**28 accords, 0 désaccord**, en $d=1,2,3,7,20,60$, plus cinq fixtures
dégénérées : observations dupliquées (niveaux nuls corrects), sept points
colinéaires, carré cocyclique, $n=1$ (aucune fusion) et $n=2$.
**Verdict : confirmé.** Reproduction : `scratchpad/audit_moi/ordre1.py`.

### A.3 — DÉFAUT MAJEUR : aucun reçu du chantier n'est ancré à un commit

C'est le défaut que l'audit rapporte en premier, parce qu'il touche la
traçabilité de **tous** les chiffres publiés.

* **17 des 18 fichiers de reçu ne portent aucun pin de commit.** Ce sont des
  sorties brutes (`.txt`, `.json`, `.jsonl`) sans en-tête d'ancrage.
* Le seul fichier qui en porte un, `receipts/ouverture_20260925/MANIFESTE.json`,
  déclare `commit = a74e90f22167105cdba90b6850f0597f1a01a329`, dont le sujet est
  « v8: widen the integer engine to 18-bit coordinates ». **Ce commit ne contient
  pas `E-HGP/`.** C'était le `HEAD` local du worktree principal, resté en arrière
  de `origin/main`, et `bench/make_receipt.py` l'a lu tel quel par
  `git rev-parse HEAD`.
* Le même manifeste porte le label « ancré sur origin/main 273c33f7c » : il est
  donc **contradictoire avec lui-même**.
* **12 des 37 fichiers hachés par ce manifeste diffèrent** du contenu du commit
  qui publie le reçu (4 sources dont tout `spectral/`, 4 sondes, 4 documents) :
  le reçu a été fabriqué pendant que des fichiers étaient encore écrits, puis
  publié tel quel.

Conséquence à écrire noir sur blanc : la phrase « reçu immuable ancré au commit »
du README et des messages de commit **n'est vraie pour aucun reçu de ce
chantier**. Les sorties elles-mêmes restent authentiques et la plupart ont été
rejouées, mais l'ancrage — la seule chose qui distingue un reçu d'un fichier de
log — est absent.

**Correction demandée** (à la charge du développeur, pas de l'auditeur) :
`make_receipt.py` doit refuser de produire un reçu si le `HEAD` lu ne contient
pas le chantier, et doit hacher les fichiers **après** la dernière écriture ; un
reçu corrigé doit être publié dans un **nouveau** dossier, l'ancien restant en
place puisqu'un reçu publié ne se modifie pas.

### A.4 — Un soupçon levé, et il faut le dire aussi

J'ai soupçonné le chantier de revendiquer l'invariance du digest par permutation
des observations, ce qui serait faux puisque l'enregistrement canonique contient
des identifiants d'observations. **Vérification faite, aucun document ne le
revendique** : le mot employé est « canonique », et il est justifié (sérialisation
JSON à clés triées, listes construites par itérations triées, donc déterministe).
La porte d'équivariance du chantier compare d'ailleurs des multiensembles de
niveaux et non des digests, ce qui est la bonne chose à comparer.

### A.6 — Le point bloquant écarté : aucun certificat de séparation ne ment

Un certificat de séparation faux serait bloquant : il ferait publier un niveau de
fusion prouvé alors que la fusion a lieu plus tôt. J'ai donc recalculé
l'ultramétrique projetée exacte avec **mon** code (mêmes définitions, autre
implémentation : reconstruction complète de $\Gamma_k$ à chaque niveau, témoin de
chaque observation par ses $k$ plus proches, balayage de l'union des niveaux
$\beta$ et des niveaux d'entrée), puis confronté chaque certificat rendu par
`certified_exact_levels`.

Sur cinq familles — uniforme, deux amas, **sept points colinéaires**, **points
cosphériques**, **observations dupliquées** — en $d=2,3,20$, $n=6$, ordres 1 à 3 :
**244 certificats examinés, 0 faux, 0 violation de la propriété de majorant**.

**Verdict : confirmé.** C'est le résultat le plus important de cet audit, parce
que c'est celui dont l'échec aurait invalidé la seule preuve que le chantier
produit lui-même. Réserve de portée : $n=6$ et $d\leq20$ ; la soundness est
démontrée dans le document, cette mesure ne fait que la corroborer sur les
dégénérescences.

**Blâme méthodologique que l'auditeur s'adresse à lui-même** : mon script
`certif.py` importe `naissances.py`, dont le bloc principal s'exécute à
l'import ; la sortie de l'attaque est donc précédée du tableau des naissances.
Cela ne change aucun chiffre, mais un script d'audit doit être propre.

### A.7 — La porte à code de sortie : les cinq codes sont atteignables

La doctrine du dépôt exige des portes à code de sortie **exact** : 0 conforme,
1 désaccord du juge, 2 refus avant calcul, 3 plancher ou invariant violé,
4 mutant tué. Un code jamais atteint est un code non vérifié. J'ai donc atteint
les cinq, au commit épinglé, sur `bench/gate_engine.py` :

| code | comment je l'ai obtenu | obtenu |
| --- | --- | --- |
| 0 | `--n 7 --d 3 --k 2 --seed 3 --min-cases 5` | oui |
| 1 | j'ai **substitué un moteur faux** (niveau de segment majoré de 1) au moment de l'appel, hors du dépôt | oui |
| 2 | `--exact=on --n 12` : refus documenté, l'oracle de la tour est borné à 9 | oui |
| 3 | `--min-cases 10000000` : plancher de couverture impossible | oui |
| 4 | les **quatre** mutants `extremites`, `ordre_precedent`, `croisements_extremites`, `sans_extremites` | oui, les quatre |

Le code 1 est le seul qui demandait un montage, et c'est le plus important :
il prouve que la porte **verrait** un moteur incorrect au lieu de sortir 0. Les
quatre mutants sont tués par désaccord, pas par plantage : le code est 4 et non
un code d'exception.

### A.5 — Contrôles de doctrine : conformes

Aucun import ni copie de `HGP-old` ou de la ligne produit `morsehgp3d/` (règle de
licence non commerciale respectée) ; aucune écriture hors du projet ; aucun usage
du schéma public v2, seulement des mentions en prose disant qu'il n'est pas
utilisé ; aucune commande GCP. `GCP non utilisé`.

# L'objet disponible : ce que la tour FULL v9 fournit vraiment

26 septembre 2026. Audit de `morsehgp3D_v9` **du point de vue du consommateur**
(un modèle de fondation), et non du point de vue du moteur. L'audit de moteur
appartient aux auditeurs indépendants de `morsehgp3D_v9/audits/` ; ce document
ne s'y substitue pas et n'y écrit pas.

Toute affirmation chiffrée ci-dessous renvoie à un reçu épinglé. Aucune n'est
une extrapolation.

## 1. Le socle mathématique (manuscrit, parties I–II)

L'objet n'est pas un choix d'ingénierie : il est fixé par le manuscrit.

- **Def. 20** — complexe de Čech : $\sigma \subseteq \mathcal{X}$ est un simplexe
  de $\check{C}(\mathcal{X}, r)$ ssi les boules de rayon $r$ centrées sur ses
  sommets ont une intersection commune non vide.
- **Def. 21** — le graphe $\Gamma_K(\mathcal{X}, r)$ a pour sommets les
  $(K-1)$-simplexes, deux d'entre eux étant reliés dès que leur union reste un
  simplexe. Un **$K$-polyèdre** est l'ensemble des points apparaissant dans une
  composante connexe de ce graphe.
- **Théorème 2** — les $K$-polyèdres sont **exactement** les amas discrets de
  forte densité de l'estimateur $K$-NN, niveau par niveau. C'est la raison
  d'être de tout l'édifice : la hiérarchie n'est pas une heuristique de
  regroupement, c'est l'estimation exacte d'un modèle statistique.
- **Théorème 4** — tout simplexe $K$-séparant est de **Gabriel** : l'intérieur
  de sa plus petite boule englobante ne contient aucun point extérieur. Seuls
  ces simplexes changent la connectivité.
- **Théorème 6** — les $K$-simplexes de Gabriel sont portés par la mosaïque de
  Delaunay d'ordre $K$.
- **§ 9.1** — pour $K \geq 2$ l'objet naturel est un **recouvrement** des points,
  jamais une partition ; mais c'est une **partition des $(K-1)$-simplexes**. Le
  passage aux points se fait par un vote pondéré exact, avec
  $S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma))$, $\psi(t) = 1/t^{p}$,
  puis $T_x = \sum_{\tau \ni x} S_\tau$ et le poids $w_{x\tau} = S_\tau / T_x$.

Deux conséquences que l'architecture doit respecter sans discussion :

1. **Le recouvrement n'est pas un défaut à réparer** : le fait qu'un point
   appartienne à plusieurs $K$-polyèdres est précisément l'information
   d'ordre supérieur. Toute conception qui commence par forcer une partition
   des points jette la contribution.
2. **La laminarité existe, mais sur les facettes.** Une attention sur arbre est
   donc possible — sur $F_K$, pas sur $\mathcal{X}$ — et le retour aux points
   est déjà fourni, exactement, par § 9.1.

## 2. Ce que la chaîne v9 calcule, et à quel prix

Source : [`morsehgp3D_v9/PASSATION.md`](../../morsehgp3D_v9/PASSATION.md)
et [`audits/ETAT_COURANT.md`](../../morsehgp3D_v9/audits/ETAT_COURANT.md).
Dernier chrono G4 : session **R22**, reçu
[`g4_tower_r22_20260926`](../../morsehgp3D_v9/receipts/g4_tower_r22_20260926/README.md),
paquet `43c5ad25`, `completed`, VM `TERMINATED` certifiée.

| entrée | sites | $K \leq 5$ | $K \leq 10$ |
| --- | --- | --- | --- |
| 08/000000 sans sol | 39 885 | 0,93 s | 2,96 s |
| 08/000100 sans sol | 35 551 | 0,76 s | 2,27 s |
| 08/000200 sans sol | 45 845 | 0,98 s | 2,89 s |
| trames brutes avec sol | 123–126 k | 1,81–2,03 s | 5,31–6,01 s |

Profil : grille entière isotrope **1 mm** (u18), prédicats entiers exacts,
aucun jitter, `s = 8`, 48 fils + RTX PRO 6000. Le statut publié est
`complete_relative` : complet **relativement** au catalogue recoupé et scellé,
pas une preuve de complétude Gabriel absolue. L'objectif de 100 ms n'est pas
acquis ; la
[feuille de route auditée](../../morsehgp3D_v9/audits/PLAN_CRITIQUE_100MS_FULL_20260924.md)
demande encore des facteurs de l'ordre de $\times 10$ à $\times 18$ sur trois
postes.

**Lecture pour le modèle de fondation.** Une à deux secondes par trame n'est
pas un obstacle au pré-entraînement : la tour est une fonction **pure et
déterministe** de la trame (condensé `tower_digest` reproductible), donc elle
se calcule **une fois** et se met en cache. C'est un avantage propre : un
tokenizer appris doit être recalculé à chaque changement de poids, pas
celui-ci. L'obstacle serait l'inférence embarquée temps réel, qui n'est pas la
cible de ce dossier.

## 3. La structure produite, champ par champ

Par ordre $K = 1 \ldots K_{\max}$, `FullBallOrder` publie :

- `forest` — une forêt de fusion (`FullCoverageCertificate`) : nœuds avec
  **niveau exact** (rayon au carré, fraction `ExactLevel` en U192/i128),
  parents en CSR (zéro parent = naissance, un = continuation, **deux ou plus =
  multifusion**), successeurs, et contributions datées ;
- les **populations** : par nœud, l'ensemble des `PointId` couverts, séparé en
  `interior` (intérieur strict) et `shell` (coquille), via un masque de
  coquille et un drapeau d'intérieur ;
- `lower_nodes` — la **verticale** : l'image du nœud d'ordre $K$ dans l'histoire
  d'ordre $K-1$, prise au niveau de création fermé du nœud.

En amont, le **catalogue** de boules minimales (`BallData`) porte, pour chaque
boule : la clé primitive exacte de sa forme quadratique, son niveau exact, son
**arité** $q_{\min} \in \lbrace 2, 3, 4 \rbrace$ (arête diamétrale, triangle
aigu, tétraèdre), ses intérieurs stricts ($\leq 9$) et sa coquille complète
($\leq 12$, au-delà c'est un refus de domaine explicite, jamais une
troncature).

C'est exactement l'alphabet de la diapositive 11 de la présentation :
$P = \bigcup_i \mathrm{conv}(Q_i)$ avec $|Q_i| \in \lbrace 2, 3, 4 \rbrace$.

La tour est donc une **bifiltration** indexée par $(K, r)$ : à $K$ fixé on lit
un arbre de fusion ; à $r$ fixé on lit une chaîne emboîtée en $K$, puisque
$L_K(r) \subseteq L_{K-1}(r)$. Les `lower_nodes` sont la réalisation de cette
seconde inclusion.

## 4. Les nombres qui contraignent l'architecture

Extraits des 36 sondes du reçu R22 (`vm/probe_*.stdout`, champ `orders`) :

| entrée | $K_{\max}$ | nœuds de la tour | nœuds par site |
| --- | --- | --- | --- |
| sans sol (35,5–45,8 k sites) | 5 | 1 306 721 – 1 683 088 | ≈ 33–37 |
| sans sol | 10 | 5 954 045 – 7 468 379 | ≈ 149–167 |
| brut avec sol (123–126 k sites) | 5 | 3 465 807 – 3 811 372 | ≈ 28–31 |
| brut avec sol | 10 | 14 966 738 – 16 274 683 | ≈ 121–132 |

Ventilation pour 08/000000 sans sol, $K \leq 5$ (sonde `probe_0`) :

| $K$ | nœuds | naissances | fusions |
| --- | --- | --- | --- |
| 1 | 79 681 | 39 885 | 39 796 |
| 2 | 178 127 | 101 089 | 77 038 |
| 3 | 285 910 | 166 228 | 119 682 |
| 4 | 421 661 | 249 493 | 172 168 |
| 5 | 576 371 | 341 081 | 235 290 |

RSS de pointe mesurée : 1,2 à 6,4 Go selon $K_{\max}$.

**Verdict.** Il y a entre **1,3 M et 16,3 M nœuds par trame**. Un Transformer
travaille sur $10^3$ à $10^4$ jetons. Le facteur de réduction exigé va donc de
$\times 320$ à $\times 4000$. **Une sélection est obligatoire, et c'est le
premier vrai choix d'architecture** — pas un détail d'implémentation. Toute
proposition qui parle de « donner la tour au modèle » sans nommer ce facteur
est inapplicable.

## 5. Le chaînon manquant, et la bonne nouvelle

La bibliothèque produit enregistrée [`morsehgp3d/`](../../morsehgp3d/)
implémente **déjà**, en arithmétique exacte, tout l'aval dont le tokenizer a
besoin :

- `build_exact_point_hierarchy(CertifiedTowerInput, PointHierarchyOptions)` :
  arbre de fusion multi-ordres, routage descendant irréversible ;
- `SimplexPointWeighting::inverse_radius` avec
  `exp_z = ambient_dimension_3()` : c'est littéralement le
  $\psi(t) = 1/t^{p}$ de § 9.1, $p = 3$ ;
- les rendus `select_lambda_cut`, `select_dbscan_radius`,
  `select_excess_of_mass` : la condensation et la sélection par excès de masse,
  exactes ;
- une enveloppe de budget explicite `large_resident_30m()`.

Le `CLAUDE.md` du dépôt le dit sans détour : « Aucun producteur (v3 ou v4) ne
l'alimente encore. » **La v9 est ce producteur.** Le chaînon manquant est donc
un seul objet, bien délimité :

> un **exportateur** `mhgp9_tower_export` qui écrit, depuis `ChainResult`,
> un `CertifiedTowerInput` : forêt tous ordres, cartes verticales, et
> **simplexes projetables** avec, pour chaque facette $\tau$, la liste des
> rayons au carré de ses cofaces $\sigma \supset \tau$ — c'est-à-dire
> l'ingrédient de $S_\tau$.

Trois précisions dont dépend la faisabilité, à traiter comme des verrous et non
comme des détails :

1. **$F_K$ est l'ensemble des simplexes de Gabriel**, pas tous les
   $(K-1)$-simplexes : le manuscrit le dit explicitement en § 9.1. La somme
   $S_\tau$ ne parcourt donc que les cofaces de Gabriel, dont le catalogue est
   la source. Sans cette restriction on retombe sur un catalogue global en
   $\binom{n}{K}$, formellement **interdit** par l'invariant d'architecture du
   dépôt.
2. **Une boule du catalogue ne porte pas un simplexe mais un plateau** : toutes
   les parties $\sigma$ vérifiant $S_b \subseteq \sigma \subseteq S_b \cup I_b$
   naissent au même rayon. L'exportateur doit donc publier des **plateaux**, pas
   des simplexes énumérés, ou mesurer explicitement l'explosion
   combinatoire. Les en-têtes `sphere_plateau` / `local_plateau` portent déjà
   cette machinerie.
3. **Le budget aval a un plafond nommé** :
   `maximum_point_simplex_incidences = 64 000 000`. Avec 1,3 M boules par trame
   à $K \leq 5$, le nombre d'incidences facette–coface est à **mesurer avant**
   tout port, pas à supposer. C'est la porte G0.4 du
   [protocole](PROTOCOLE.md).

## 6. Ce que la tour ne fournit pas

À énoncer clairement, parce que chacun de ces points est un poste de travail et
non une omission :

- **aucune géométrie explicite $P_v$** : les nœuds portent des `PointId`, pas
  des coordonnées ni des enveloppes convexes. La réalisation
  $P_v = \bigcup_b \mathrm{conv}(S_b)$ se reconstruit depuis le catalogue ;
- **aucun descripteur de nœud** : c'est l'objet de [`JETON.md`](JETON.md) ;
- **aucune sérialisation** : le seul format publié aujourd'hui est un JSON de
  compteurs et un condensé FNV-64, pas la tour elle-même ;
- **aucune étiquette** : la v9 ne consomme ni ne produit d'étiquette
  SemanticKITTI, par contrat ;
- **aucun attribut capteur** : rémission, anneau, horodatage et portée sont
  perdus à la quantification 1 mm. Ils doivent être **rattachés hors moteur**,
  par `PointId`, dans le tokenizer.

## 7. Les faits de la v9 à ne pas oublier en concevant

- Les prédicats sont **entiers exacts** ; le flottant n'existe que comme filtre
  certifié à repli exact. Un descripteur en float32 est donc une **sortie**, pas
  un maillon de la chaîne d'exactitude.
- Les dégénérescences donnent un **refus explicite**, jamais un jitter : le
  tokenizer doit savoir traiter un `unsupported_degeneracy` sans fabriquer une
  trame de remplacement silencieuse.
- Les `PointId` sont arbitraires (ni index dense, ni rang Morton). Tout
  rattachement d'attributs doit passer par une table, pas par une position.
- Le plafond de coquille (12 sites) est un **refus de domaine** : une coquille
  plus grande interrompt la trame. Le taux de refus sur le corpus complet
  SemanticKITTI n'est pas mesuré ; c'est la porte G0.2.
- `K_{\max} \leq 10` est le domaine ; au-delà, refus explicite.

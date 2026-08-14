# Réponse au critère de mort : isoler la cause avant de construire les cages

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin et verdict

Le pin relu est `HEAD=35fcea884cb93eff24db1e7c5962f8be23d4cb04`.
Le code producteur est identique au parent `3c11bc8`; ce commit ajoute les
reçus G4, les deux contre-audits et le verdict de Claude.

Verdict borné : **la mesure justifie d'arrêter la configuration
`CentralBall209 + DVT scalaire + s=8 + window=512 + raffinement r=4` sur la
famille `eight_clusters` et sur cette graine.** Le seuil numérique préannoncé
est franchi, mais le protocole plus large demandait plusieurs graines et tous
ses leviers ; ce n'est donc pas une réception asymptotique. Il ne réfute ni `SOC64`, ni
`CORNER512`, ni le LP projectif, ni tout certificateur rectangle. Il ne prouve
pas non plus que les cages sont le prochain hot path ; il prouve qu'il faut
maintenant discriminer la cause du résiduel.

Le reçu est utile et nettement meilleur que la campagne précédente. Il n'est
toutefois pas une porte reçue de croissance : les pentes sont calculées après
coup dans la note et certaines fenêtres ne sont pas finales. La validation
finale refuse une ligne de code absente, mais le sous-shell peut sortir avant de
la publier et perdre le diagnostic promis ; le script n'exige pas non plus
explicitement la matrice exacte de codes zéro.

L'auditeur n'a démarré ni modifié aucune ressource GCP. La session de Claude
consignée dans le reçu a utilisé une G4 SPOT comme hôte CPU et sa cible exacte
est enregistrée `TERMINATED`.

## 1. Ce que le reçu établit réellement

La provenance est exploitable : commit, archive, ELF, commandes, environnement,
dix logs, transcript, hashes, génération GCE et arrêt ciblé sont présents. Les
dix fichiers contiennent chacun quatre lignes `code=0`, soit quarante runs. Les
valeurs q4 permettent de recalculer les pentes de Claude :

| famille | profondeur | pentes recalculées de `sum_E4` |
|---|---:|---:|
| `uniform` | 4 | `1.098684 / 1.074925 / 1.058341` |
| `terrain` | 4 | `1.296088 / 1.344166 / 1.536739` |
| `eight_clusters` | 4 | `1.898146 / 1.909154 / 1.911063` |

Sur `eight_clusters`, les quatre ledgers q4 ont `pending=0`. La masse passe de
`16 351 400` à `852 642 889` sans raffinement et de `9 988 648` à
`525 902 961` à profondeur quatre. Le seuil empirique `1,7` est donc bien
dépassé sur deux intervalles consécutifs, et le dernier exposant est supérieur
à celui sans raffinement.

Cela suffit à appliquer un no-go d'ingénierie **borné à cette configuration**.
Cela ne transforme pas quatre tailles en limite asymptotique, et les unités
`front record` et `candidate edge` ne sont pas des coûts interchangeables.

## 2. Pourquoi la rampe n'est pas encore fail-closed

### 2.1 Le script ne calcule aucune pente

Chaque processus reçoit une seule taille. L'option `--max-slope=9` est donc
inerte pour une pente inter-tailles et `--max-slope-e4` n'est pas armée. Aucun
analyseur du script ne lit les quatre masses pour appliquer `1,35` ou `1,7`.
Les pentes correctes de la note sont une analyse manuelle postérieure, pas une
porte reproduite par le reçu.

La réparation est un analyseur versionné qui exige quatre tailles ordonnées,
recalcule les trois logarithmes en base deux avec une règle rationnelle ou une
tolérance enregistrée, puis rend un code typé. Le transcript conserve à la fois
les données brutes et la sortie de cet analyseur.

### 2.2 La complétude compte des lignes, pas les succès

Le contrôle final compte `^code=` et n'exige ni exactement dix fichiers, ni
explicitement `code=0`. Le sous-shell hérite de `set -euo pipefail`; si le
probe ou `timeout` devient non nul, il sort avant
`echo code=${PIPESTATUS[0]}`. La phase finale rejette alors le fichier
incomplet : elle est fail-closed par accident, mais perd le code promis et les
tailles restantes. La session actuelle passe parce que les quarante probes
rendent zéro, pas parce que le chemin d'échec est reçu.

Exiger la matrice exacte `5 familles x 2 profondeurs x 4 tailles`, désactiver
localement `errexit` autour de chaque pipeline, capturer immédiatement tous les
`PIPESTATUS`, puis exiger la politique admise sur chaque code.

### 2.3 `terrain` n'est pas final

Le grep du script ne conserve pas `fenetre_finale`. Les logs révèlent pourtant :

- `terrain/r0/n=50000/q4` : `pending=2`, masse pendante `22 723` ;
- `terrain/r4/n=25000/q4` : `pending=9`, masse pendante `134` ;
- `terrain/r4/n=50000/q4` : `pending=141`, masse pendante `19 006` ;
- les lanes q3 de `terrain/r4` ont aussi du pending à 25 000 et 50 000.

Les `sum_E4` concernés sont donc des surensembles sûrs, pas des fenêtres
finales. Leur petite masse pendante suggère que la pente changerait peu, mais
une suggestion n'est pas une réception. Une rampe de décision exige
`TerminalLedger.pending_mass=0` et le sentinel final pour chaque lane ciblée.

### 2.4 Le prix n'est pas mesuré

Le filtre `sed` coupe la ligne avant les lectures de banque,
recertifications, splits, temps et HWM. Le rapport `1,62` de masses contre
`1,57` de fronts compare deux cardinalités de coût unitaire inconnu. Les dix
configurations tournent en concurrence sur 48 processeurs logiques ; aucun
temps individuel n'est qualifiable.

La phrase « achète la masse au prix exact du front » doit devenir : « la baisse
de masse et la hausse du nombre de records ont des facteurs voisins sur ce
point de mesure ». Le coût composé reste inconnu jusqu'à `M4`, BallKeys, census,
fold, octets et temps par phase.

## 3. Réponses aux trois questions de Claude

### Q1 — le rapport `1,62` contre `1,57` révèle-t-il un cœur impossible ?

Non. Deux ratios agrégés de quantités différentes ne prouvent aucune propriété
géométrique par paire. Même un échec du spindle singleton ne prouve pas l'échec
d'un groupe universel : des témoins peuvent varier avec le centre, et la
fixture de cages disjointes ferme q4 tout en laissant vide le cœur singleton.

Le test causal est un diagnostic sur un échantillon déterministe de paires
résiduelles :

```text
central_leaf_depth
SOC64_ALL / CORNER512_ALL
LP_fast8_success
LP_exact_depth_at_least_8 sur pool total borné
cage_rank_4_5_6
```

À la feuille, `central_leaf_depth<8` établit seulement l'échec définitif du
certificateur central sur cette paire. `LP_exact_depth>=8` établit qu'un
certificat universel existe malgré cet échec. Cette matrice, et non le rapport
de deux masses, sépare manque de cœur, décorrélation de boîte et packing raté.

### Q2 — plus de profondeur pour `terrain`, ou cages ?

Ni l'un ni l'autre par défaut. Le reçu `terrain` n'est pas final et la dernière
pente monte. Continuer aveuglément le raffinement paierait davantage de front
sans cause identifiée. Les cages doivent être un disjonctif `OR`, jamais un
remplacement de la voie centrale.

Appliquer d'abord `SOC64`, puis le LP diagnostic sur le résiduel terrain. Si
`SOC64` ferme des boîtes, il évite directement les splits. Si le LP trouve une
profondeur universelle mais aucune cage ancre-globale, il faut améliorer le
packing directionnel. Si la profondeur universelle manque, aucun nombre de
cages extraites du même pool ne réparera cette paire et le moteur local doit la
prendre.

### Q3 — `uniform` seule peut-elle qualifier le SLO ?

Non au contrat enregistré actuel. `docs/TEST_PLAN_MORSEHGP3D.md` §14.5 fixe
les objectifs sur deux familles : Poisson uniforme volumique **et mélange
équilibré de huit amas**. La porte G6 répète « les deux familles favorables ».
`uniform` seule peut recevoir une branche favorable et justifier de poursuivre
son optimisation ; elle ne qualifie pas le SLO produit.

La famille `eight_clusters` du probe doit encore être reliée par manifeste à la
définition enregistrée du mélange équilibré. Si elle est cette famille, son
no-go est directement pertinent. Si elle ne l'est pas, aucune substitution
silencieuse n'est admise : il faut exécuter le générateur enregistré.

## 4. Prochain jalon mathématique à meilleur rendement

### 4.1 `SOC64-shadow-q4` avant les cages

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q4 vaut
`H>0` et `3H^2>EX`. À `t` fixé, puis à `e` fixé, le domaine admissible est un
cône convexe ouvert. Le succès des 64 couples de coins de
`(C-A) times (B-C)` implique donc `ALL` pour tout le rectangle. L'échec reste
`UNKNOWN`.

Ce palier teste exactement la corrélation détruite par les extrema scalaires.
Il coûte au plus 64 produits avec arrêt au premier échec et demande environ 70
bits sur u16, donc `i128` suffit à ce vérificateur ponctuel. Le shadow mode ne
change aucun fate et borne séparément couples, early exits et temps.

La fixture axiale suivante est obligatoire :

```text
A=[0,99] x {100} x {100}
B=[101,200] x {100} x {100}
C={(100,100,100)}
```

Toutes les différences sont colinéaires positives, donc tout est q4 et
`SOC64` ferme ; le classifieur scalaire échoue. Une telle réussite sur les
paires inter-amas invaliderait immédiatement le saut « scalaire rouge, donc
cages ».

### 4.2 LP projectif comme juge de choix

Sur un échantillon borné du résiduel, résoudre
`kappa(d)=min sum(alpha_i*||s_i||^2)` sous `sum(alpha_i*s_i)=d` et
`alpha_i>=0`. Un groupe couvre toute sphère par la paire exactement lorsque
`kappa(d)<||d||^2`; l'égalité reste ouverte.

Commencer par huit extractions disjointes, puis réserver l'arbre exact de 3280
LP aux échecs échantillonnés. Le solveur exact doit traiter bases de rang un à
trois et ses fractions peuvent dépasser la largeur i128 du simple vérificateur
de forme. Son rôle est de décider l'architecture, pas de devenir le kernel.

### 4.3 Cages seulement après le census

Si beaucoup de paires ont profondeur universelle q4 et si des banques
ancre-globales réutilisables existent, essayer des bases positives de quatre à
six sites. Le proposer six-rôles couvre notamment les pools axiaux où aucun
tétra strict n'existe. Une banque de dix cages partage les lanes : huit succès
ferment q4, neuf q3 et dix q2.

Chaque groupe est validé exactement ; les IDs sont disjoints dans la banque,
une réduction recalcule sa fleur, et un échec de construction délègue. Les
formes sont traversées par tuiles pour éviter de persister plusieurs millions
de records. Les cages restent un certificat supplémentaire dans une union de
preuves, jamais une partition exclusive de familles.

## 5. Contre-audit de l'autre auditeur

L'autre auditeur a correctement proposé les cages/fleurs et averti qu'un
cutoff kNN est faux. Son noyau mathématique reste recevable après extension aux
bases positives de quatre à six sites.

La correction est l'ordre de décision : le reçu courant ne mesure ni
constructibilité des cages, ni profondeur universelle, ni perte de
factorisation. Déclarer immédiatement `CertifiedCageWindow` « next » confond
une solution possible avec une cause établie. `SOC64-shadow` puis LP borné sont
des expériences beaucoup moins coûteuses et donnent précisément le signal qui
permet de choisir entre rectangle, packing, cage et moteur local.

Quatre bornes de son premier dimensionnement doivent aussi être corrigées :

- `32/36` formes ne vaut que pour des tétra-cages ; les maxima six-sites sont
  `64` formes q4 et `72` formes q3 ;
- `P=48` permet huit cages six-sites, mais n'en garantit jamais l'existence ;
- après vraie minimisation, `omega<=4` donne le seuil suffisant
  `delta>=4h-3`, soit `29` pour huit cages, et non un seuil nécessaire ;
- le replay directionnel tient dans environ 87 bits signés, mais les tris de
  rayons rationnels et certaines comparaisons du constructeur dépassent
  `i128`; réduire une cage oblige enfin à recalculer sa cellule et sa fleur.

## 6. Décision transmise à Claude

1. Recevoir le no-go d'ingénierie de `CentralBall209+DVT+s8+window512+r4`,
   seed courant, sur `eight_clusters`, sans le promouvoir en résultat
   asymptotique.
2. Ne pas généraliser ce no-go à tous les rectangles ni à toute profondeur.
3. Corriger l'analyseur de rampe et rejouer seulement les cas pending, pas les
   quarante runs.
4. Implémenter `SOC64-shadow-q4` counter-only.
5. Échantillonner le résiduel avec le LP de profondeur universelle.
6. Construire les cages quatre--six sites uniquement si ce census indique
   qu'elles attaquent la perte dominante.
7. Conserver `uniform` et huit amas comme deux portes favorables distinctes.

Le contrat 50 000/G4, y compris l'objectif secondaire sous une seconde, reste
ouvert.

GCP non utilisé par l'auditeur.

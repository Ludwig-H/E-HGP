# E-HGP — hiérarchies K-NN d'ordre supérieur

E-HGP est un projet de recherche consacré au calcul de la **hiérarchie complète des amas de forte densité K-NN**. L'objet central est la bifiltration des multicovertures

$$L(k,t)=\left\lbrace y\in\mathbb{R}^{d}:D_k(y)\leq t\right\rbrace,$$

où $D_k(y)$ est la $k$-ième distance euclidienne au carré ordonnée. Le projet vise simultanément les ordres $1\leq k\leq K_{\max}$ : la sortie n'est ni une partition plate ni un champ échantillonné, mais une famille cohérente de forêts de fusion reliées par les inclusions $L(k+1,t)\subseteq L(k,t)$.

> [!IMPORTANT]
> Le dépôt est actuellement dans une **phase de spécification mathématique**. Les backends cibles décrits ci-dessous ne sont pas encore implémentés. Le package `perg_hgp` conservé dans l'arbre est uniquement le prototype de référence `PowerCover3D`; sa grille cubique ne constitue pas l'algorithme cible.

## Deux backends, un même contrat hiérarchique

| régime | backend cible | structure retenue | sortie visée |
|---|---|---|---|
| petite dimension fixée, notamment 3D, avec données massives | `MorseHGP3D` | catalogue classé de sphères critiques, descente K-NN–miniball, hyper-Kruskal gradué | HGP dur exact lorsque catalogue, prédicats, attaches et incidences de lots sont fermés |
| grande ou très grande dimension | `HomogeneousLensTower` | atlas de blocs top-$K_{\max}$, continuation DTM $q=1$ vers HGP $q=\infty$, coupes minimax certifiées | exact sur l'atlas; exact globalement sur un périmètre condensé seulement sous certificat explicite |

Les deux backends devront produire un `GradedMergeForest` commun :

- une forêt de fusion $T_k$ pour chaque ordre;
- les niveaux de naissance et de fusion dans les unités d'entrée;
- les morphismes verticaux $T_{k+1}\to T_k$;
- des lots déterministes pour les événements de même niveau;
- un statut parmi `exact`, `atlas_exact` et `conditional`, accompagné d'un périmètre `full` ou `condensed(pi)`.

## Voie 3D : `MorseHGP3D`

Pour $K_{\max}=10$, une même sphère critique de rang fermé $s$ porte une naissance à l'ordre $s$ et, pour $s\geq2$, une selle à l'ordre $s-1$. La voie `PowerCascadeH0` doit exploiter cette réutilisation entre ordres :

1. représenter les domaines top-$k$ par des diagrammes de puissance ordinaires en 3D sur des sites barycentriques pondérés;
2. engendrer l'ordre suivant par récurrence, puis fermer chaque cellule par séparation top-$k$ exacte à ses sommets;
3. extraire et certifier les supports critiques de tailles deux à quatre;
4. calculer les attaches par descente K-NN–miniball;
5. traiter les événements égaux par lots dans une réduction graduée.

Le backend de production ne matérialisera ni rhomboid tiling, ni mosaïque de Delaunay d'ordre supérieur, ni grille uniforme. La cible de latence inférieure à une seconde pour environ $50\,000$ points 3D reste une hypothèse expérimentale à tester sur données résidentes GPU; ce n'est pas une borne de complexité annoncée.

## Voie grande dimension : `HomogeneousLensTower`

La régularisation principale est la lentille homogène

$$F_{k,q}(y)=\left(\frac{1}{k}\sum_{j=1}^{k}a_j(y)^q\right)^{1/q},\qquad 1\leq q\leq\infty,$$

où $a_j(y)$ est la $j$-ième distance au carré ordonnée. Elle relie exactement la DTM empirique au carré, $q=1$, au champ HGP dur, $q=\infty$. L'entropie rend les oracles locaux homogènes et batchables; elle ne crée pas à elle seule la sparsité globale.

Le backend partagera des blocs top-$K_{\max}$ entre tous les ordres, poursuivra $q$ vers l'infini et certifiera les décisions minimax par coupes. UMAP, CAGRA, NN-descent ou tout autre index ANN peuvent proposer des blocs, jamais certifier seuls la hiérarchie.

## Documentation normative

| document | rôle |
|---|---|
| [Architecture mathématique](docs/ARCHITECTURE_MATHEMATIQUE.md) | synthèse des deux backends, contrats GPU et programme de falsification |
| [Résultat A — Lentilles homogènes](docs/math/RESULTAT_A_LENTILLES_HOMOGENES.md) | interpolation DTM–HGP, convexité et bifiltration |
| [Résultat B — Descente atomique](docs/math/RESULTAT_B_DESCENTE_ATOMIQUE.md) | terminaison, chemins sous-niveau et attaches |
| [Résultat C — Complexe de Morse bifiltré](docs/math/RESULTAT_C_COMPLEXE_MORSE_BIFILTRE.md) | définition de la sortie hiérarchique commune |
| [Résultat D — Énumération 3D par puissance](docs/math/RESULTAT_D_ENUMERATION_3D_POWER_GPU.md) | cascade de diagrammes de puissance et fermeture exacte |
| [Résultat E — Récupération condensée par coupes](docs/math/RESULTAT_E_RECUPERATION_CONDENSEE_COUPES.md) | certificat sparse en grande dimension |

L'[index de la documentation](docs/README.md) précise l'ordre de lecture et les statuts scientifiques.

## Arborescence actuelle

```text
.
├── docs/                         # corpus scientifique actif
│   ├── ARCHITECTURE_MATHEMATIQUE.md
│   ├── math/                     # résultats A–E
│   └── references/               # sources fondatrices locales
├── HGP-old/                      # référence historique liée au manuscrit
├── perg_hgp/                     # prototype PowerCover3D de référence
├── experiments/powercover3d/     # protocole Blackwell historique isolé
├── gcp-migration/                # infrastructure active de VM GPU G4
├── .github/workflows/            # CI et contrôle GCP manuel
├── tools/check_docs.py           # validation locale de la documentation
└── README.md
```

La lignée `HGP-old` est conservée intégralement comme référence du manuscrit, sous sa licence propre. Les lignées `E-HGP`, l'atlas `PERGHGPClusterer`, les rapports datés et les résultats de benchmarks générés ont été retirés du répertoire courant. Ils restent consultables dans l'[instantané historique du 14 juillet 2026](https://github.com/Ludwig-H/E-HGP/tree/0b8a1a11750b931f486ce666265eed4b6e95e2b1).

## Prototype de référence

`PowerCover3D` reste disponible pour auditer les contrats numériques, les budgets mémoire et certaines primitives CPU/CUDA. Il calcule une approximation cubique régularisée; il ne doit pas être utilisé comme substitut de `MorseHGP3D` ou `HomogeneousLensTower`.

```bash
python -m pip install -e 'perg_hgp[test]'
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python -m pytest -q perg_hgp/tests
```

Voir [`perg_hgp/README.md`](perg_hgp/README.md) pour son périmètre exact et [`experiments/powercover3d/`](experiments/powercover3d/) pour le protocole historique conservé.

## Infrastructure GPU GCP

Le répertoire [`gcp-migration/`](gcp-migration/) est une infrastructure **active** et indépendante du statut scientifique des prototypes. Il permet de vérifier les quotas, créer de façon interactive une VM Spot `g4-standard-48`, contrôler son environnement Blackwell, puis l'arrêter avec vérification. Cette machine peut servir aux expériences `PowerCover3D` et aux futurs noyaux des deux backends.

La création reste volontairement locale et confirmée par l'utilisateur : aucun workflow GitHub ne crée, ne démarre ou n'arrête une ressource facturable. Le workflow [`gcp.yml`](.github/workflows/gcp.yml), déclenché manuellement, vérifie uniquement l'identité fédérée et rend visible l'état de la VM cible. Voir le [mode d'emploi GCP](gcp-migration/README.md).

## Validation documentaire

```bash
python tools/check_docs.py
```

Ce contrôle vérifie les liens locaux, les blocs mathématiques monolignes et les constructions LaTeX interdites par [`agents.md`](agents.md). La même commande est exécutée par l'intégration continue.

## Feuille de route

1. éprouver les résultats A–E sur des contre-exemples minimaux;
2. construire un oracle CPU exhaustif commun aux deux régimes;
3. mesurer la taille réelle du certificat de fermeture 3D;
4. rendre effectif le certificat de $\pi$-complétude en grande dimension;
5. figer le schéma `GradedMergeForest`;
6. seulement ensuite, spécifier et implémenter les noyaux GPU des deux backends.

Toute future revendication de performance devra préciser le backend, le statut de certification, le périmètre hiérarchique, le matériel et le protocole reproductible.

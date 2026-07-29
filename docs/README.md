# Documentation MorseHGP3D

Ce corpus est organisé autour d'une seule voie active : catalogue exact et multi-ordre des paires diamétrales, frontière indépendante des triangles aigus, puis tétraèdres bien centrés, enfin réduction hiérarchique sparse. Il sépare les preuves, les oracles bornés, les replis de recherche et les pistes abandonnées.

Contexte courant : Phase 15, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, porte d'entrée satisfaite et porte de sortie ouverte. Le contrat GPU est documenté; seuls les deux oracles CPU bornés du catalogue de paires sont implémentés à ce stade. `public_status=not_claimed`.

## Parcours actif

1. Lire les Parties I et II du [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134. L'objet à préserver est la hiérarchie de clusters discrets, pas la mosaïque géométrique ambiante.
2. Lire la [spécification](SPECIFICATION_MORSEHGP3D.md) et la [définition HGP 3D](math/DEFINITION_HGP_3D.md).
3. Lire le [catalogue exact des paires diamétrales](math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md). Il fixe le rang fermé, le payload exhaustif, le cutoff exact Yao48, le rôle de Morton et l'architecture résidente multi-ordre.
4. Lire la [frontière directe des supports trois et quatre](math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md). Elle explique pourquoi les triangles aigus exigent une frontière indépendante et comment les tétraèdres se réduisent ensuite.
5. Lire le [catalogue critique 3D](math/CATALOGUE_CRITIQUE_3D.md), les [incidences silencieuses](math/INCIDENCES_SILENCIEUSES_GAMMA.md) et les [attaches par miniball](math/ATTACHES_DESCENTE_MINIBALL.md) pour raccorder le flux géométrique à la hiérarchie.
6. Vérifier le [registre des preuves et heuristiques](math/STATUT_PREUVES_ET_HEURISTIQUES.md).
7. Utiliser l'[architecture GPU](GPU_G4_ARCHITECTURE.md), la [roadmap](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md), le [plan de tests](TEST_PLAN_MORSEHGP3D.md) et le [registre des phases](implementation_status.toml) comme contrats d'implémentation et de réception.

## Carte des statuts

| ensemble | rôle | peut devenir le produit ? |
|---|---|---|
| `math/` et documents de phase courants | définitions, théorèmes, obligations et architecture active | oui, après fermeture des gates |
| [`research/`](research/README.md) | trois replis exacts ou oracles maintenus | seulement selon les limites écrites |
| [`reference/`](../reference/README.md) | vérité terrain indépendante de petite taille | non |
| [`validation/`](validation/README.md) | revues actives, JSON, transcripts et checkers | non, ce sont des preuves d'exécution |
| [`archive/`](archive/README.md) | décisions remplacées et expériences scellées | non sans procédure explicite de réouverture |

## Invariants d'architecture

- aucune mosaïque de Delaunay d'ordre supérieur, population globale de cellules, cofaces ou incidences;
- aucune fenêtre Morton, liste Yao48 finie, ANN ou triangulation ordinaire utilisée comme autorité de complétude;
- une passe $K_{\max}$, puis routage des rangs vers tous les ordres;
- proposition flottante, décision certifiée, réduction hiérarchique et statut public toujours distincts;
- tout arrêt de capacité produit un état borné ou un échec fermé, jamais une absence déclarée exacte;
- les cas réguliers restent sur GPU; le CPU n'est qu'un repli arithmétique rare et un vérificateur terminal;
- les oracles exhaustifs restent petits et indépendants du producteur.

## Contrats scientifiques

| terme | sens dans ce dépôt |
|---|---|
| `exact` | catalogue et hiérarchie complets sur le profil annoncé, pour l'interprétation exacte des coordonnées d'entrée |
| `conditional` | records retournés vérifiés, mais frontière, attache ou dégénérescence non fermée |
| `unsupported_degeneracy` | domaine exact non pris en charge; aucune publication exacte |
| `budget_exhausted` | cap de temps, mémoire, travail ou sortie atteint avant fermeture |
| objectif | hypothèse expérimentale réfutable, jamais garantie universelle |

Le p95 `warm_e2e` sous 100 ms à 50 000 points et $K_{\max}=10$ reste un objectif sensible à la sortie. Le passage à 10 M–30 M impose un flux segmenté et reprenable. Les deux propriétés ne sont pas déduites d'un temps de noyau isolé.

## Sources et contrats sérialisés

Le [corpus bibliographique](references/README.md) documente le manuscrit, les articles et leurs licences. Les [contrats de phase 0](contracts/README.md) définissent les schémas sérialisés, exemples et matrices de traçabilité. Aucune preuve documentaire ne remplace le rejeu des certificats prévu par ces contrats.

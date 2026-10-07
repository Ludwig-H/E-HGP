# Morse HGP 3D v12

**Chantier actif depuis le 7 octobre 2026** (ouverture dans `AGENTS.md` et `CLAUDE.md`, sur « Lance-toi à fond
maintenant dans le développement de la v12 »). Ce dossier rassemble les décisions prises, l'objet et le contrat
mathématique à porter, l'architecture, le plan par tranches, le protocole de mesure, la provenance des sources, les
leçons et les pièges ; le code arrive par tranches ([`docs/PLAN.md`](docs/PLAN.md)).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0 (tour FULL des ordres 1..K, verticales comprises)
quantification=quantized_u21_input_only (moteur conçu pour B <= 32, arithmétique en repère local ; u24 puis u32 à qualifier)
public_status=not_claimed
```

Le cadre sépare désormais deux axes que les versions précédentes confondaient sous le mot `profile` : l'**objet**
calculé (`full_pi0`, dont `hgp_reduced`, les points et la sortie plate sont des vues) et la **quantification** de
l'entrée.

## Demandes de l'utilisateur

**7 octobre 2026.**
- « On va se lancer dans la v12 de Morse HGP 3D. »
- « Tu as feu vert pour utiliser la GCP G4. Le but est de faire une v12 aussi propre, simple et efficace que
  possible. »
- « Crée aussi le dossier morsehgp3D_v12 avec toutes les informations utiles pour le développement à venir. »

Elles s'ajoutent aux demandes de la v11, toujours en vigueur sauf décision contraire : 100 ms sur trames LiDAR sans sol
à K = 5 et si possible K = 10 ; rigueur mathématique ; comparaison à HDBSCAN sur données synthétiques et réelles.

## L'objet en une phrase

Pour $k=1,\ldots,K$ et $a\geq 0$, soit $D_k(y)$ le carré de la distance de $y$ à son $k$-ième plus proche site et
$L_k(a)=\lbrace y\in\mathbb{R}^{3} : D_k(y)\leq a\rbrace$ ; la **tour FULL** est, pour chaque $k$, l'arbre de fusion des
composantes connexes de $L_k(a)$ quand $a$ croît, avec les applications verticales $L_{k+1}(a)\subseteq L_k(a)$, aux
niveaux rationnels exacts. C'est l'arbre des amas de Hartigan de l'estimateur $K$-NN, pour tous les ordres à la fois
(thèse, Th. 2), enrichi de la tour et des verticales.

## Le point de départ

**À reproduire** : les empreintes FULL de la v11, reproduites à l'octet pour l'audit du 7 octobre
([`docs/MESURE.md`](docs/MESURE.md), § 4).

**À battre** (G4, 48 fils, trames sans sol ng00 / ng01 / ng02, à chaud) :

| Version | K5 | K10 |
| --- | --- | --- |
| v11 au gel, voie GPU | 251 / 212 / 255 ms | 1 782 / 1 336 / 1 536 ms |
| v11 au gel, voie CPU | 314 / 255 / 313 ms | — |
| v10 (CPU, une passe chaude) | 252 / 204 / 254 ms | 1 125 / 861 / 1 024 ms |

Le contrat de 100 ms n'a jamais été tenu, par aucune version, pour l'objet vrai.

## Les trois principes

1. **Contrat, oracle et microbancs avant le moteur.** Chaque étage est d'abord mesuré hors moteur, sur les entrées
   vidées de la v11, contre une règle écrite d'avance.
2. **Les changements d'algorithme sont déclarés d'avance** (catalogue résident sur le GPU et en flux ; plus petite
   boule proposée puis certifiée ; forêt sans lots ; registre d'événements dont les sorties sont des vues). **Tout le
   reste est porté explicitement**, épinglé et requalifié. Une reprise « plus propre » à algorithme constant n'a jamais
   rendu de vitesse dans toute la lignée.
3. **Un seul chemin produit, qui est le chemin mesuré.** Pas de modes à masque d'options, un seul profil, une seule
   implantation de chaque noyau.

## Commandes

Le socle (tranche T0) se construit et se juge ainsi ; `<build>` hors de `/workspaces`, presque plein.

```bash
cmake -S morsehgp3D_v12 -B <build> -DCMAKE_BUILD_TYPE=Release   # profil u21 ; -DMHGP12_COORD_BITS=24 ; 18 et 32 refusés
#   option : -DMHGP12_V10_FROZEN_DIR=<dossier de mhgp10_catalogue et mhgp10_tower>  (portes diff_v10 de l'oracle)
cmake --build <build> -j4
ctest --test-dir <build> -LE long --no-tests=error --output-on-failure -j4    # portes rapides
python3 morsehgp3D_v12/tools/check_style.py --root morsehgp3D_v12
python3 morsehgp3D_v12/tools/check_constats.py                                # registre et canal d'audit
```

Labels CTest : `fast`, `long`, `unit`, `oracle`, `diff_v10`, `mutant`, `lidar` (sautées sans `MHGP12_DATA_DIR`).
Codes des portes : 0 conforme, 1 désaccord du juge, 2 refus avant calcul, 3 invariant violé, 4 mutant tué.

## Lire d'abord

1. L'[audit géant de la v11](../morsehgp3D_v11/docs/AUDIT_GEANT_V11.md), au moins § 0, § 2, § 5, § 7.5 et § 9.
2. [`docs/DECISIONS.md`](docs/DECISIONS.md) : régime, contrat, précision, données, multiplicités, seuil.
3. [`docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md`](docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md) : énoncés à porter, identifiants,
   témoins à graver d'abord, questions ouvertes.
4. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) : étages, budgets, règles de simplicité.
   [`docs/CONTRAT_NUMERIQUE.md`](docs/CONTRAT_NUMERIQUE.md) : coordonnées jusqu'à 32 bits, arithmétique en repère local.
5. [`docs/PLAN.md`](docs/PLAN.md) : tranches, microbancs et règles d'adoption.
6. [`docs/MESURE.md`](docs/MESURE.md) : régimes, données, chiffres de référence, empreintes, protocole statistique.
7. [`docs/PROVENANCE.md`](docs/PROVENANCE.md) : sources épinglées et règle de port.
8. [`docs/LECONS_ET_PIEGES.md`](docs/LECONS_ET_PIEGES.md) : ce qu'il ne faut pas refaire, pièges payés.
9. [`audits/README.md`](audits/README.md) : canal d'audit et constats reportés de la v11.

## Carte du dossier

| Fichier | Rôle |
| --- | --- |
| `README.md` | ce fichier |
| `docs/DECISIONS.md` | décisions prises le 7 octobre (délégation de l'utilisateur), décisions antérieures en vigueur |
| `docs/CONTRAT_NUMERIQUE.md` | coordonnées jusqu'à 32 bits : repère local, garde entière, budgets par étendue, clé de Morton |
| `docs/CONTRAT_CATALOGUE.md` | contrat de la tranche T1 : objet, changements d'algorithme, numérique, capacité, compteurs, portes, budget |
| `docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md` | objet, énoncés à porter, témoins, doctrine numérique, questions ouvertes |
| `docs/ARCHITECTURE.md` | architecture proposée et règles de simplicité |
| `docs/PLAN.md` | ordre de travail, portes d'entrée et de sortie |
| `docs/MESURE.md` | protocole de mesure et chiffres de référence |
| `docs/DONNEES.md` | données des trois régimes : recensement de 43 jeux, jeux retenus, scènes, trames, petits nuages, préparation et rejeu (`bench/data/`) |
| `docs/PROVENANCE.md` | épingles des sources (v11, v10, conception, données) |
| `docs/LECONS_ET_PIEGES.md` | leçons de la lignée, pistes fermées, pièges d'exploitation |
| `docs/OUVERTURE_PROPOSEE.md` | texte proposé pour `AGENTS.md` et `CLAUDE.md` |
| `audits/README.md` | règles du canal d'audit, constats et questions reportés |

Ces documents sont des **propositions** tirées de l'audit du 7 octobre : leurs chiffres d'objectif sont des hypothèses à
confirmer par mesure, et rien ici ne qualifie un code ni ne revendique un statut public.

# Morse HGP 3D v11

**Chantier clos le 7 octobre 2026.** L'utilisateur a décidé une passation et une reconstruction à neuf pour une v12.
Commencer par la [passation](PASSATION.md), puis lire l'[audit final](docs/AUDIT_FINAL_V11.md).

Ouverte le 2 octobre 2026, sur `main`, avec une base de code **neuve** : la v10 y était un sujet différentiel et une
source de fixtures ; tout ce qui en est repris est un port explicite, épinglé et requalifié
([provenance](docs/PROVENANCE.md)). La v11 joue désormais ce rôle pour la v12.

```text
phase=exploration_v11_hors_registre (close le 7 octobre 2026)
backend=cpu_reference ; voie de banc cuda_g4 pour le lot de feuilles du catalogue
profile=quantized_u21_input_only
public_status=not_claimed
```

## Demandes de l'utilisateur

**2 octobre 2026.** « Repartir de zéro pour avoir quelque chose de plus propre. Mêmes contrats : 100 ms sur nuages
LiDAR sans sol (éventuellement avec sol), avec K = 5 et si possible K = 10. Toujours très rigoureux
mathématiquement. Il faut ensuite se comparer à HDBSCAN, sur données synthétiques mais aussi sur données réelles. Il y
a notamment des tests dans le dossier `Zoltan/` où la hiérarchie HDBSCAN échoue ; on peut en trouver d'autres. Il
faudrait des exemples où la hiérarchie HGP réussit. »

**Précisions ultérieures.**
- Passer à u21, voire u24. Le défaut est devenu u21 ; u18 et u24 restent compilables.
- Viser d'abord 200 ms sur G4 pour FULL K = 1..5, puis K = 1..10.
- Mesurer et décider sur trames LiDAR réelles.
- Le 4 octobre, une sortie paramétrée native (`--sortie`).
- Le 6 octobre, des supports réduits à l'arbre couvrant de Kruskal.

**7 octobre 2026.** « On va organiser plutôt une passation et tout reconstruire à neuf pour une v12 de Morse HGP 3D. »

## Objet

Pour $k = 1, \ldots, K_{\max}$ et $a \geq 0$, soit $D_k(y)$ le carré de la distance de $y$ à son $k$-ième plus proche point et $L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace$. La **tour FULL** est, pour chaque $k$, l'arbre de fusion des composantes connexes de $L_k(a)$ quand $a$ croît, avec les applications verticales $L_{k+1}(a) \subseteq L_k(a)$ ; les niveaux sont des rationnels exacts.

## État au gel

Dernier commit moteur : `ac081a06f`. Le détail, les sources et les limites sont dans la [passation](PASSATION.md)
(§ 2) et l'[audit final](docs/AUDIT_FINAL_V11.md) (§ 3).

**Temps sur LiDAR réel.** G4, 48 fils, trames sans sol ng00 / ng01 / ng02 de la séquence 08, grille 1 mm :

| Mode | À froid | À chaud |
|---|---|---|
| K5, voie GPU (feuilles 24) | 335 / 301 / 345 ms | 251 / 212 / 255 ms |
| K5, voie CPU (feuilles 16) | 343 / 272 / 329 ms | 314 / 255 / 313 ms |
| K10, voie GPU (feuilles 24) | 1 824 / 1 395 / 1 602 ms | 1 782 / 1 336 / 1 536 ms |

**Contrats.**
- 100 ms : non tenu.
- Jalon de 200 ms : approché sur ng01 à chaud seulement.
- K10 : hors de portée.
- La v10 faisait 204 à 254 ms à K5 et 0,86 à 1,12 s à K10, sur CPU (captures distinctes, pas un A/B).

**Exactitude.**
- Aucun résultat FULL faux établi.
- Voie GPU identique au CPU à l'octet.
- Empreintes FULL K5 stables du 3 au 7 octobre.
- Dernière qualification complète : `98a009550` (5 octobre ; 3 695 portes, 485/485 mutants).

## Sorties de `mhgp11`

Un seul exécutable, à paramètre obligatoire `--sortie` ; le [contrat des sorties](docs/SORTIES.md) fixe options,
refus, formats, manifeste et transaction de dossier.

| `--sortie` | Objet | Format |
| --- | --- | --- |
| `full` | tour FULL, ordres 1 à K, verticales | `MHGP11FUL1` |
| `supports` | arbre couvrant d'ordre K : naissances et fusions retenues par Kruskal au plateau, S* seul (§ 10.10 « Sortie publiée » de [MATHEMATIQUES.md](docs/MATHEMATIQUES.md)) | `MHGP11SP` v2 |
| `points` | hiérarchie de points $H^{r}_{K+1}$ ([note](docs/HIERARCHIE_POINTS.md)) | `MHGP11PT` v1 |
| `plat` | étiquettes plates : condensation au critère A, puis EOM N-aire exacte ([sortie plate](docs/SORTIE_PLATE.md)) | `MHGP11ET` v1 |

L'exécutable et l'API prennent la voie CPU à feuilles de 16, sans GPU ni cache de blocs. Les modes plus rapides
n'existent que dans le banc (`bench/full_probe.cpp`, `bench/gpu_ab.py`).

## Construction

```bash
cmake -S morsehgp3D_v11 -B build/v11 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v11 --parallel
ctest --test-dir build/v11 -LE long --no-tests=error --output-on-failure   # portes rapides
```

Le défaut est `MHGP11_COORD_BITS=21` ; `-DMHGP11_COORD_BITS=24` donne le domaine 24 bits.

La matrice complète passe sur G4 par `tools/g4_matrix.py`, dans une session gardée `gcp-migration/v11_session.py`
(voir `gcp-migration/README_V11.md`) : GCC 11.4 de la VM, ASan + UBSan, TSan, profils 18, 21 et 24 bits, tampons
empoisonnés, mutants, suite complète de la référence. Depuis le 5 octobre, les constructions Release et les portes
ciblées se font aussi en local.

## Lire d'abord

1. [Passation](PASSATION.md) : état au gel, ce qu'il faut porter, ce qu'il ne faut pas refaire, décisions
   attendues, plan proposé pour la v12.
2. [Audit final](docs/AUDIT_FINAL_V11.md) : chronologie, bilan chiffré, comparaison à la v10, registre des leviers,
   causes racines, leçons.
3. [Audit géant](docs/AUDIT_GEANT_V11.md) : contre-audit du même jour. Il couvre le modèle mathématique de bout en bout,
   la thèse, les enjeux et les applications, les contrats, la lignée v2 → v11 et la vérification par exécution. Il
   corrige l'audit final (§ 8) et propose la v12 (§ 9).
4. [Mathématiques](docs/MATHEMATIQUES.md), [architecture](docs/ARCHITECTURE.md), [provenance](docs/PROVENANCE.md).
5. [Canal des audits](audits/README.md) et la [note de clôture](audits/NOTE_CLAUDE_CLOTURE_V11_20261007.md).

`docs/DEVELOPPEMENT.md` est figé au 3 octobre ; la passation le remplace.

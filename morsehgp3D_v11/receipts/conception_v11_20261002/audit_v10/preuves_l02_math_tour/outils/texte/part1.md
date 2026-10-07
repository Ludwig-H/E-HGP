# L02 — Mathématiques de la tour FULL : audit de `morsehgp3D_v10` pour la conception de la v11

2 octobre 2026. Lentille L02 de l'audit à fond de la v10. Heure relevée par `date -u` à la fin de la rédaction : @@HEURE@@ UTC.

```text
phase=audit_v10_pour_v11 (lecture seule)
sujet=morsehgp3D_v10 au HEAD afb081774 (arbre de lecture build/v11-worktree)
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé
```

Vocabulaire des modes de vérification : **lu** (lecture du code ou du document, ligne citée), **exécuté** (commande rejouée ici, sortie conservée), **mesuré** (compteur ou invariant calculé ici sur une entrée nommée), **prouvé ici** (preuve complète rédigée au § 4, à contre-lire par un tiers), **conjecturé**. Les temps locaux (hôte de 8 cœurs partagé, charge 10 à 19 pendant tout l'audit) ne sont jamais interprétés.

## 0. Résumé

1. **L'objet calculé est juste, et c'est établi ici plus fortement que par les portes du dépôt.** Un juge indépendant écrit pour cet audit reconstruit l'arbre de fusion exact de $\Gamma_k$ en `Fraction` et le compare, nœud par nœud, à la forêt C++ : naissances identifiées par leur population, multifusions N-aires avec leurs enfants exacts, image verticale de **chaque** nœud à la coupe fermée, attache de chaque site. Résultat : @@TOT_CLOUDS@@ nuages de 7 à 13 points dans huit familles (génériques, grilles, plans, amas, cosphériques, cocycliques, alignés, cubes), ordres jusqu'à 10, plus 15 fixtures gravées : @@TOT_BIRTHS@@ naissances, @@TOT_MERGES@@ fusions dont @@TOT_AR3@@ à trois parents ou plus, @@TOT_VERT@@ verticales, @@TOT_PTS@@ attaches, **0 écart**.
2. **Le théorème derrière « minima fixes, morceaux locaux (Gordan), descente » est vrai sans position générale, mais il n'est écrit nulle part dans la v10.** Le dépôt n'en porte que l'énoncé (SPEC § 4, en-tête de `tower.hpp`) et des esquisses d'une ligne dans une conception privée ; le registre des preuves n'a aucune ligne v10. Le § 4 de ce rapport en donne une rédaction complète (classification des événements d'un niveau, fenêtre de rang, descente, plateaux, verticales, Euler), avec statuts et obligations restantes.
3. **La porte `mhgp10_tower_oracle` n'établit que le nombre de composantes et la partition des points entrés.** Cinq mutants sur six la traversent (multifusion binarisée, plateau non atomique, image verticale à la coupe ouverte, image de fusion sans remontée, niveau d'attache strict). Le juge de cet audit les tue tous les six ; trois invariants linéaires tuent aussi, à l'échelle, les cinq qui survivent.
4. **Aucun invariant global d'échelle n'est une porte en v10, alors qu'ils tiennent et ne coûtent presque rien.** Mesuré ici sur les trois trames du contrat (35 551, 39 885 et 45 845 sites) et sur 15 entrées synthétiques de 8 000, 16 000 et 32 000 points : Euler $\chi_K = 1$ pour $K = 1, \ldots, 5$ partout (0,05 à 0,7 s de calcul une fois le catalogue construit à $K + 2$) ; forêt d'ordre 1 égale au dendrogramme N-aire de l'arbre couvrant minimal euclidien de scikit-learn, poids et structure ; attaches et images verticales vivantes à la coupe fermée ; plateaux atomiques ; sortie identique à l'octet près pour 1, 2 et 4 fils et pour deux règles de descente différentes mais valides.
5. **L'exactitude est relative au catalogue, et la tour seule ne voit pas tout catalogue incomplet.** Expérience des catalogues amputés (64 nuages, 3 062 retraits d'une boule admissible) : la tour refuse dans 80,9 % des cas, publie une forêt inchangée dans 9,5 % et publie **une forêt fausse sans refus dans 9,6 %** (295 cas) ; Euler détecte ces 295 cas, sans exception.
6. **La conception normative `TOWER_v2` n'a pas été implémentée.** Le code livré est la tour d'ouverture (descente avec mémo, Kruskal par lots), accélérée ; l'index $(\pi, J)$, la requête `WA`, les ancres, les contributions datées, Euler, le refus d'une sphère absente, les juges d'échelle, les mutants, les fixtures épinglées et le différentiel v9 de la conception n'existent pas. Les « continuations datées » annoncées dans l'objet (README) ne sont pas calculées.
7. **Le Théorème 5 du manuscrit (K-arbre couvrant minimal du K-graphe de Gabriel) est faux en général**, même en position générale : contre-exemple exact à cinq points du plan, $K = 2$, où le graphe de Gabriel reste à jamais en deux composantes. La v10 n'en dépend pas (c'est précisément ce que la descente répare) ; la v11 ne doit pas y revenir.

Aucun constat bloquant dans cette lentille : rien de ce qui a été jugé n'est faux. Les constats majeurs portent sur ce qui garantit l'exactitude (preuves écrites, portes, invariants), pas sur l'exactitude constatée.

## 1. Périmètre lu

- Dans `build/v11-worktree/morsehgp3D_v10/` : `docs/SPEC_V10.md` (entier), `docs/conception/TOWER_v2.md` (entier, 1 636 lignes), `docs/conception/CONCEPTION_V10.md` (§ 0 à 2, 4.3, 6.4, 9.4, 10, 11.3), `docs/conception/README.md`, `README.md`, `PASSATION.md`, `src/tower/tower.hpp`, `src/tower/tower.cpp` (entier, 1 867 lignes), `src/tower/rank_search.hpp`, `src/catalogue/support.hpp`, `src/catalogue/catalogue.hpp`, `src/arith/geometry.hpp` et `.cpp`, `src/core/reasons.def`, `src/core/status.hpp`, `src/core/types.hpp`, `src/cloud/cloud.hpp`, `src/cloud/site_tree.hpp`, `cli/mhgp10_tower.cpp`, `cli/mhgp10_catalogue.cpp`, `reference/hgp10_ref.py`, `reference/test_ref.py`, `tests/oracle/test_tower_oracle.py`, `tests/oracle/test_catalogue_oracle.py`, `tests/regression/test_batch_equivalence.py`, `CMakeLists.txt`, `cmake/gates.cmake`, `cmake/run_expect.cmake`.
- Registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (recherche des lignes v10 et des lignes FULL v7 et v9), historique `git log` de `src/tower/` et de `tests/oracle/`.
- Thèse, pages PDF 35 à 134 (`build/v9-open-worktree/docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, texte extrait par `pdftotext`) : Déf. 6 à 8, 20 à 22, 25 à 31, Th. 2, 4, 5, 6, 7, Prop. 5 à 9, § 9.1, Algorithme 1.
- Pistes, jamais preuves : audit géant (`receipts/audit_geant_developpeur_20260930/RAPPORT_AUDIT_GEANT.md`, `TRACKER.md` lignes RG1, AT1, AT2, JG1 à JG3), `build/v10-persist/design/TOWER_v1.md`, `build/v10-verify-tower/verify.sh`, `build/v10-persist/g4/verify_tower.log`, `build/v10-r2-backup-20260930/oracles/`, notes v7 `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md` et `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md` (dans `build/v9-open-worktree`), rapport voisin `build/v11-persist/audit_v10/L06_CODE_TOUR.md` (constats 01 et 03, pour vérifier la cohérence des nombres).

## 2. Méthode

- **Build.** Copie des sources de la v10 (hors reçus et audits) sous `/tmp/v11-audit/l02_math_tour/v10src`, build Release (GCC 13.3, `-Werror`), `-j3`. Rien n'est écrit dans l'arbre de lecture.
- **Porte du dépôt rejouée.** `tests/oracle/test_tower_oracle.py` au HEAD : `tower_oracle_checks 73 fails 0 cuts 23444`, code 0.
- **Sonde `l02_dump.cpp`** (API publique de `mhgp10_core` seulement) : catalogue à `kcat`, tour à `K`, dump enrichi (population $I \cup U$ de chaque naissance), invariant d'Euler calculé sur le catalogue, invariants linéaires de cohérence.
- **Juge `l02_judge.py`** (Python, `Fraction`, aucune dépendance au code ni à la référence de la v10) : plus petite boule englobante exacte (lemme de Welzl, recoupée contre la force brute sur 10 392 parties, 0 écart), arbre de fusion de $\Gamma_k$ étiqueté par les naissances, comparaison B, M, V, P décrite au § 6.2.
- **Six mutants et deux variantes neutres** de `tower.cpp`, en copies sous `/tmp` (script `make_mutants.py`, chaque édition exige une occurrence unique). Chaque mutant est soumis à la porte enregistrée du dépôt (24 nuages, 73 exécutions), puis au juge de cet audit, puis aux invariants d'échelle.
- **Catalogues amputés** (`l02_amputation.py`) : la sonde retire une boule du catalogue avant la tour ; le juge classe la sortie (refus, forêt inchangée, forêt fausse publiée) et Euler est calculé sur le même catalogue.
- **Invariants globaux à l'échelle** : Euler, ordre 1 contre l'arbre couvrant minimal euclidien de `sklearn.cluster.HDBSCAN(min_samples=1)` (scikit-learn 1.9.1, jamais réimplémenté), cohérences linéaires, neutralité des variantes, invariance au nombre de fils. Entrées : les trois trames du contrat (`build/v10-g4-data-s1/lidar0{0,1,2}_full.u32le`, lues en place, aucun octet copié sous `/workspaces`) et les 15 entrées synthétiques existantes de `build/v10-scale-inputs` (régime spatial, 8 000, 16 000 et 32 000 points, cinq familles).
- **Règle d'échelle respectée.** Les oracles exhaustifs restent bornés à 13 points (ils *établissent* la vérité). À l'échelle, seuls des invariants globaux et des juges linéaires sont utilisés ; aucun juge cubique, aucune vérification exhaustive.
- **Thèse.** Le Théorème 5 est confronté par un script exact (`l02_these_th5.py`) qui construit le K-graphe de Gabriel de la Déf. 29 et le compare aux K-polyèdres de Čech.

Reproduction : annexe A. Pièces déposées : annexe B.

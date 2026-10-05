# Tranche S6b : assemblage de la hiérarchie des supports

Rapport du 5 octobre 2026, 11 h 52 UTC (`date -u`). Cadre : phase=exploration_v11_hors_registre,
backend=cpu_reference, profile=quantized_u21_input_only, public_status=not_claimed. GCP non utilisé.

## Commit local

- `66bf76b92` (worktree `build/v11-impl-l2`, HEAD détaché, parent `165def5ab`), **non poussé**.
- Sujet : « v11: support hierarchy assembly with per-thread admission and full oracle differential (slice S6b) ».
- 15 fichiers, +2 122 / −11.

## Ce qui est livré

**Produit.** `src/supports/hierarchy.cpp` (nouveau) et `src/supports/supports.hpp` contiennent :
- `Ball` (40 octets) ;
- `SupportHierarchy` ;
- `HierarchyAdmission`, la formule normative ;
- `HierarchyTimings`, publié au succès seulement ;
- `build_support_hierarchy(const OrderTree&, MemoryBudget&, sched::Pool* = nullptr, HierarchyTimings* = nullptr)`.

L'appel se déroule ainsi :
1. **Pré-passe.** `check_shell` est appliqué à toutes les coquilles étendues de W_K, avant toute allocation : le refus
   `support_shell_capacity` vaut pour l'appel entier. La pré-passe contrôle aussi le rattachement : tailles, `BallIdx`
   strictement croissants, nœuds, forme.
2. **Admission du premier étage.**
   - Retenus : 4N + 4N + 8(N+1) + 40B + 16(B+1) + 4A.
   - Temporaire : 4B (origine de chaque position).
   - Par fil actif (taille du Pool, 1 sans Pool) : `sizeof(SupportLedger)`, le brouillon 8·closure_words(w) et la liste
     `sizeof(Support)`·support_capacity(w), où w est la plus grande coquille étendue de W_K.
   - Les tampons des fils sont alloués **après** les sorties. Une formule qui les oublierait ferait donc monter le pic
     avant le refus : c'est ce qui rend les mutants d'admission causaux.
3. **Postordre itératif sans pile.** Pendant qu'un nœud est sur le chemin, son curseur d'enfant est rangé dans `post`.
   On en tire aussi les tailles de sous-arbre.
4. **Tri des boules par seaux.** On place les boules de la dernière à la première en décrémentant la fin de chaque
   seau, ce qui donne un tri stable (rang, `BallIdx`) dans chaque seau. Les décalages des branches suivent.
5. **Passe count parallèle.** Pour chaque position : métadonnées, Q_b par `ball_supports` dans le brouillon du fil,
   `make_shape` et `ball_counts`, puis la contre-épreuve `strict_traces == journal` (sinon `supports_invariant`).
6. **Clôture des comptes.** Sommes des registres par fil, sommes préfixes `u64` vérifiées, admission du second étage :
   (sizeof(Support)+4)·S.
7. **Passe fill.** Elle reprend la même position et le même `BallIdx` qu'en count.
   - Le nombre de supports doit être le même qu'en count (sinon `supports_invariant`).
   - Les supports sont triés explicitement par (arité, `SiteIdx`).
   - On calcule les incidences par support et on recopie les branches.

**Documents.** Mis à jour : `docs/SORTIES.md` (§ 6, paragraphe « Assemblage »), `docs/PROVENANCE.md` (nouvelle
section S6b, écrite à neuf, avec ses gardes sans porte possible), `docs/ARCHITECTURE.md`, `README.md` et
`src/supports/source_pins.json` (statut).

## Choix

- **Comptes par support.** Les incidences par support sont stockées en mémoire (`support_cofaces`, 4 octets par
  support). `Support` reste inchangé (20 octets), donc la liste de 12 926 entrées du contrat garde sa taille. Les
  cofaces de Gabriel par support sont dérivées par `support_gabriel_cofaces`.
- **Tri des supports.** Il ne change rien à la sortie de `ball_supports`, mais il fait de l'ordre publié un contrat de
  l'assemblage, et il donne un site causal au mutant `supports_tri_pointid`.
- **Métadonnées dans la passe count.** Les métadonnées des boules sont remplies dans la passe count parallèle : à K5
  sur ng00, en W1, la partie sérielle passe ainsi de 30 ms à 17 ms.
- **Signature.** Un paramètre facultatif `HierarchyTimings*` est ajouté à celle de la spécification. Il sert à mesurer
  l'étage à part et à juger l'admission.
- **Différentiel sans fractions en C++.** La sonde publie les niveaux en hexadécimal et les supports en coordonnées et
  en `SiteIdx`. La porte Python tire le centre exact de l'étage A de l'oracle, par `Definition.meb(S*)`, et contrôle au
  passage que le niveau publié égale celui de S*.

## Portes (construction Release u21, `build/v11-persist/b21-s6b`, `-j 6`)

Durées locales sur le codespace, à reporter au budget G4.

| Porte | Ligne ou résultat | Durée |
| --- | --- | ---: |
| `mhgp11_supports_hierarchy_fixtures` | 69 ordres, 18 606 boules, 4 008 coquilles étendues, 1 311 à plusieurs supports, 3 146 tétraèdres, fermeture brute sur toutes | 0,68 s |
| `_determinism` | sans Pool, W1, W2, W4, arbre sériel ou par lots : même empreinte, même registre | 0,78 s |
| `_permutation` | entrée inversée et PointId réétiquetés jusqu'à 2^32−1 : même empreinte | 0,66 s |
| `_sphere5` | 24 sites admis ; 828 supports (12, 24, 792) ; comptes de K1 à K3 (24/24/12, 276/264/288, 2 024/1 736/3 906) ; 4 068 incidences à K3 | 0,10 s |
| `_sphere9` | 25 sites, K1 et K2, sans Pool et W4 : `support_shell_capacity`, pic 0, diagnostics intacts | 0,03 s |
| `_admission` | admis = formule de la porte ; pic = premier + second étage exactement ; à premier−1 : `memory_budget` à pic nul ; à total−1 : refus du second étage, budget rendu ; au total : succès ; W1, W2, W4 | 0,28 s |
| `mhgp11_supports_hierarchy_fault_starvation` | panne de chaque allocation, 3 nuages × {sans Pool, W4} | 0,02 s |
| `mhgp11_supports_hierarchy_fraction` et `_opt` (python3 -O) | voir ci-dessous | 18,9 s chacune |
| `mhgp11_supports_hierarchy_scale8000` (W1, W2, W4) | voir ci-dessous | 1,9 s |
| `_permutation_scale8000` | même ligne que scale8000 | 1,6 s |
| `_scale16000` | voir ci-dessous | 4,2 s |
| `_scale32000` | voir ci-dessous | 9,2 s |
| `_grid8000` | voir ci-dessous | 9,4 s |
| `_permutation_grid8000` | même ligne que grid8000 | 8,6 s |
| `_lidar_ng00_k5`, `_ng01_k5`, `_ng02_k5` | voir ci-dessous | 4,9 / 4,0 / 4,8 s |
| `_lidar_ng00_k10` (labels `lidar long`) | conforme en local, voir ci-dessous ; enregistrée en W1, W4, W48 avec 48 fils pour l'arbre, verdict à graver sur G4 | ~30 s en W1 et W4 (non jouée telle quelle) |

**Ligne du différentiel** (`hierarchy_fraction.py --bits=21 --workers=1,3`, aussi joué sous `python3 -S -B` à la main ;
lignes u18 et u24 calculées par l'oracle seul et gravées dans `tests.cmake`) :
`hierarchy_fraction_couverture bits=21 nuages=211 exclus=0 ordres=963 boules=15270 noeuds=12576 naissances=6651 fusions=5858 internes=2761 supports=17197 etendues=2645 multiples=576 tetraedres=1145 ordres_6plus=7 fils=1,3`,
puis `hierarchy_fraction_ok controles=153695`.
- Il couvre tous les champs canoniques de l'oracle : nœuds, rattachement, rôles, branches, Q_b, cinq comptes par
  boule et deux par support, plus les tailles de sous-arbre.
- Il juge aussi l'ordre natif : (postordre, rang, `BallIdx`), et (arité, `SiteIdx`) avec les `SiteIdx` égaux aux rangs
  de Morton.

**Lignes d'échelle** (toutes conformes ; fermeture brute de chaque coquille ; empreinte égale à W1, W2 et W4) :
- 8000 : `k=5 n=8000 boules=395667 noeuds=273655 naissances=164842 fusions=108813 internes=122012 supports=395667 etendues=0 multiples=0 tetraedres=109772 branches=273654 fermetures=395667 kparties=1549792 cofaces=230825 incidences=230825 coquille_max=4 empreinte=8f153ae85de3dc6e entree=3be1324202d28360`
- 16000 : `boules=819004 noeuds=565098 ... empreinte=db71b4702625e23c entree=3469c29b4c34b7e3`
- 32000 : `boules=1690045 noeuds=1163756 ... empreinte=d619483bb2bd7fe0 entree=aea3dec129cef911`
- grid8000 (`--grid=8000,32,1000,20261004`, générateur propre à la sonde) : `boules=340017 noeuds=123153 ... supports=506343 etendues=125169 multiples=55795 tetraedres=198234 ... coquille_max=16 empreinte=ae41bbf6304ab905`
- ng00 K5 : `boules=789886 noeuds=576371 naissances=341081 fusions=235401 internes=213404 supports=789889 etendues=141 multiples=3 ... branches=576483 ... empreinte=9679af706d4ebcc7`.
  Les nombres de boules, de naissances, de fusions, d'internes, de branches et de nœuds sont ceux de S3.
- ng01 K5 : `empreinte=3ebfb3cb792297db`.
- ng02 K5 : `empreinte=8ca05a63c81b3e6e`.
- ng00 K10 : `boules=2117675 noeuds=1638573 naissances=979350 fusions=659261 internes=479064 supports=2117676 etendues=124 multiples=1 ... empreinte=ddd262b4970db0a8`.

**Mutants.** `run_mutants.py --check` donne `manifeste_ok module=supports mutants=15 plancher=15`. Les six nouveaux,
joués avec `--only` (4 min 44 s, 2×2 cœurs), sont tous tués avec un verdict `code` : `mutants_ok module=supports
mutants=6 tues=6`.
- `boules_propres_decroissantes`, tué par `_fixtures`.
- `supports_tri_pointid`, tué par `_fixtures` (ordre (arité, `SiteIdx`), S* en tête).
- `admission_brouillon_oublie`, `admission_liste_oubliee` et `admission_un_seul_fil`, tués par `_admission`.
- `prepasse_plafond_omise`, tué par `_sphere9` : le refus vient plus tard, à pic non nul.

**Non-régression.**
- Les 23 portes existantes `mhgp11_supports_(unit|fraction|shell|judge_small)` sont vertes (91 s).
- `check_style` : `style_ok fichiers=452`.
- `tools/check_docs.py` : sortie identique à la base (156 lignes, aucune nouvelle).
- Aucun octet existant n'est touché : FUL1, PH, les sondes existantes et `tower` sont inchangés.

## Mesure locale de l'assemblage (ng00, à part de l'arbre ; codespace, valeurs indicatives)

| K | W | assemblage | sériel (pré-passe, postordre, seaux) | count | fill | admis (étages 1 et 2) | arbre d'ordre K, 4 fils |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| 5 | 1 | 304 ms | 51 ms | 144 ms | 109 ms | 58,9 + 19,0 Mo | 1,22 s |
| 5 | 4 | 129 ms | 58 ms | 40 ms | 31 ms | | |
| 10 | 1 | 1 009 ms | 228 ms | 449 ms | 331 ms | 159,8 + 50,8 Mo | 9,36 s |
| 10 | 4 | 461 ms | 231 ms | 136 ms | 94 ms | | |

Sur ng01 et ng02 à K5 (mesures précédentes) : W1 232 et 302 ms, W4 90 et 132 ms.

**Constat.** La partie sérielle (postordre et seaux, accès aléatoires) pèse 45 à 50 % de l'assemblage dès W4, et
elle ne baisse pas avec le nombre de fils. C'est le levier si l'étage `output` de S7 compte pour les 100 ms. Piste
possible : seaux parallèles par tranches, ou postordre tiré de la numérotation canonique. Rien n'a été optimisé
au-delà, la mesure G4 décidera.

## Écarts

- Le générateur `--grid` de la sonde n'est pas celui de `sample_judge.py` (`random.sample` de CPython non porté). Ses
  empreintes sont propres à cette porte.
- Pas de juge d'échantillon Fraction dédié à la hiérarchie à l'échelle. Les invariants y sont recomptés en entiers par
  `hierarchy_support.hpp` ; la vérité de Q_b par boule à l'échelle reste portée par `mhgp11_supports_sample_judge_*`
  (S6a).
- `tree_k_sha256` (I11, dernier point) relève de S7 et n'est pas jugé ici.
- La copie de `PythonRandom` dans la sonde est la troisième du dépôt (après `catalogue_euler.cpp` et `attach_judge.cpp`).
- `mhgp11_supports_hierarchy_lidar_ng00_k10` n'a pas de LINE gravée : verdict à graver après G4.
- Gardes sans porte possible de l'arbre et du rattachement : elles sont listées dans PROVENANCE, sans mutant
  (mutants équivalents).

## Ce qui doit tourner sur G4

- **Constructions.**
  - u24 : la ligne du différentiel u24 est gravée, identique à u21.
  - u18 : la ligne gravée vient de l'oracle seul (209 nuages, 2 exclus) ; elle n'a pas été jouée nativement.
  - ASan/UBSan et TSan sur `mhgp11_supports_hierarchy_*` : passes count et fill parallèles, registres par fil.
- **Mutants.** La campagne `mhgp11_mutants_supports` complète, avec ses 15 mutants.
- **Échelle.** Les portes `scale8000`, `scale16000` et `scale32000` et `lidar` (K5), puis `_lidar_ng00_k10` (long) en
  W48 avec 48 fils pour l'arbre, dont il faut graver le verdict.
- **Mesure de l'étage d'assemblage** à W1 et W48, à K5 et K10, à part de l'arbre, pour la règle L2 et pour l'étage
  `output` de S7.
- **Budget de temps** : environ 1 min de portes `fast` nouvelles en local (dont 2 × 19 s de différentiel) et environ
  60 s d'échelle et LiDAR K5 ; ng00 K10 environ 30 s en local, hors catalogue.

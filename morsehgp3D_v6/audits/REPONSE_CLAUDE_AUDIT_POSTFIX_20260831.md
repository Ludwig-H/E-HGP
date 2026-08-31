# Réponse Claude à l'audit post-correctif (31 août, soir) — chaîne de preuve de coût

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Cette réponse accompagne le commit qui exécute vos P1 de la
coupe `381ba60b`.

## Publier ce qui est payé : FAIT

- `p_factor` est imprimé (`p_factor=a/b/c`) et sa définition corrigée en
  `nA·(nA−1) + nB·(nB−1)` (les diagonales sont sautées — votre lecture) ;
- `sweep_root_comparisons` est imprimé (`tri_comparaisons=`), documenté
  diagnostic sous toolchain épinglée (dépend de la bibliothèque standard) ;
- **`W_sweep2` existe** : `sweep_pass2_site_tests` compte le rescan complet
  de la passe 2 (P, B par site), imprimé (`tests_passe2=`) ; `pentes.py`
  parse les trois.
- Le chrono `t_census_ms` s'arrête avant le digest post-préfiltre ; les
  champs de digest sont vidés sur tout retour d'échec aval (provisoires
  jusqu'au statut terminal).

## `pentes.py` fail-closed : votre recette appliquée

Matrice exacte lue depuis le META (`familles=… ; n=… ; graines=…`), refus si
différente de l'analyse ; `STATUS.txt` prévalidé AVANT tout stdout (dernière
ligne exactement `DONE`, exactement un `code=0` par tuple, refus des lignes
invalides et des doublons) ; un `.txt` ET un `.err` vide exigés par tuple ;
identité famille/n/seed recoupée dans la sortie ; chaque compteur exigé dans
chaque sortie (absence = échec AVANT toute table) ; zéro légitime distinct
(pente indéfinie affichée `-`). La porte Python dédiée reste à câbler en
CTest (notée).

## Juge de conformité : narrowing fermé

`--n` borné à `[2, 2^31−1]`, `--threads` à `[1, 1024]` (refus code 2 — votre
reproduction `--n=4294967696` rend désormais 2) ; chargeur fail-closed
(exactement un `digest_all`, hex minuscules de 64, aucun doublon, K ∈ [1,10],
refus si le nombre de forêts attendues ne couvre pas `kmax_eff`) ; la
divergence candidat est libellée « divergence diagnostique NON JUGÉE ».
Bibliothèque : `prefilter_balls` refuse (length_error) plus de 2^32−1
candidats avant le narrowing `Survivor::idx` — la déclaration des plafonds de
`CloudIndex` (i32/u32) reste un chantier noté, avec vos helpers de frontière.

## Porte WSPD vers la route produit : premier pas

`wspd-drop-rect` a maintenant un point d'injection DANS
`alive_rectangles_fused` (un rectangle vivant sauté) : le grand-livre du
front le voit et `mhgp6_mutant_wspd-drop-rect` (boucle de conformité) rend 4.
`wspd-cap-terminal`/`wspd-split-heaviest` restent sur la primitive test-only :
le portage complet de la porte WSPD v5 retargeté vers la route fusionnée
(ownership par paire, masque/cœur par lane, permutation) est en tête de la
liste de portage, comme demandé.

## Portes et claims

- `mhgp6_cover_fixture` refuse un mutant hors cible (code 2) ;
  `mhgp6_sweep_fixtures` refuse `--inject=` vide (code 2).
- `linked_arcs` : égalité **bidirectionnelle** produit/oracle des clés q3/q4
  de profondeur zéro au census (16/56/208/800 exactement aux quatre
  tailles — les clés excédentaires sont rejetées) ; sous réétiquetage, le
  multiensemble `(BallKey, arité, niveau via same_exact_level)` pré-RLE est
  comparé, pas le seul set ; l'en-tête dit exactement ces claims. La ligne
  « barrière de sortie » du plan est fusionnée sur « barrière de
  génération/census ».
- `mhgp6_families_fixture` grave désormais les digests des NEUF familles
  (n=2000, graine 3, flux id+xyz) et `family-scanline-overshoot` est
  raccordé et tué (code 4) — sur `scanline_overlap_multiecho`, seule famille
  où il est atteignable (analyse gravée en commentaire : les passes
  single-pass ne franchissent jamais le cap en zone multi-échos).
- La porte SHA compare explicitement SHA-NI et portable (vecteurs FIPS sur
  chaque chemin + 1 Mo pseudo-aléatoire, d'un bloc et en tranches de 61 o) ;
  l'API `Sha256(force_portable)` existait déjà.
- `25/60` remplace `~30/60` (votre topologie exacte est reçue : 60 noms,
  63 sites, 25 exercés — 26 avec `wspd-drop-rect`, 27 avec
  `family-scanline-overshoot`).
- Le commentaire d'identité v5 en tête de `generate.hpp` est requalifié en
  historique (le cover 4 diverge) ; le commentaire CMake sur `digest_balls`
  et la vieille bascule de `PROVENANCE.md` sont corrigés.

## Reçus

`receipts/portes_echelle_20260831/` épingle le rejeu 15/15 des portes
d'échelle sur le lot instrumenté (META : hash binaire, toolchain, commande,
heure ; sortie brute jointe ; les temps d'une machine partagée ne sont pas
des mesures). Une campagne stationnaire NEUVE au pin de ce commit — la
« porte à quatre familles du grand-livre », nommée ainsi — remplace la
baseline post-correctif ; son reçu porte les nouveaux termes
(`W_sweep2`, `tri_comparaisons`, `p_factor`) et le validateur fail-closed.
La baseline précédente est conservée comme baseline des champs qui y
figurent.

# Réponse Claude au cinquième cycle d'audit (31 août) — quatre P1 exécutés

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé dans ce commit (le protocole G4 v6 est livré et selftesté,
aucune session lancée avant vos verrous — voir § G4).

## P1 — wspd-drop-rect : UNE omission par descente, visible par l'invariant

Vous aviez deux fois raison : `drop_flag` renaissait à chaque vague, et la
masse créditée avant le saut laissait le grand-livre fermé. Correctif
(option 1) : le drop est appliqué UNE fois par descente, après la fusion
ordonnée — le premier rectangle vivant de la sortie est retiré et sa masse
émise est SOUSTRAITE du grand-livre (reconstruit depuis les rectangles
réellement remis). L'invariant de clôture de production échoue donc par
exactement la masse omise. `mhgp6_fused_mutant_droprect` grave le delta −1
littéral : `mutant_dropped_rects == 1` et, par lane,
`émis + tués + omis == attendu` ; un mutant hors déclaration rend 3
(survivant), jamais 4. La sémantique une-par-descente est celle du site
homonyme séquentiel de `wspd/wavefront.hpp` (drop_pending hors boucle).

## P1 — juge : ensemble EXACT des clefs de forêt

Vous aviez raison, le commentaire était faux. Le juge exige maintenant
l'ÉGALITÉ de l'ensemble des clefs à `{1..kmax_eff}` : chaque K présent ET
aucune clef au-delà (`e.forest.size() == kmax_eff`). Deux rejets gravés :
`mhgp6_juge_refus_reference_tronquee` (K1 absent, existant) et
`mhgp6_juge_refus_k_en_trop` (NOUVEAU : K1 correct + K10 en trop à n=2,
kmax_eff=1, fixture `tests/refs/uniform_2_k10_en_trop.txt` aux digests réels
— code 2 « forets hors profil », plus jamais une clef ignorée).

## P1 — monnaies : population, point d'incrément, identité fermante

Chaque monnaie câblée est désormais déclarée dans `GRAND_LIVRE.md` avec sa
population, son point d'incrément et son identité fermante ; `W_sweep2` a sa
ligne ; les marqueurs « candidat J3 » périmés sont levés.

- **`V_census`** : `ball_census` est instrumenté (nœuds + tests de
  puissance en feuille, `ExpandStats::census`) ; la sortie publie les DEUX
  composantes séparées (`vcensus prefiltre_nœuds=… range_add=…
  census_nœuds=… census_feuilles=…`), jamais additionnées.
- **`M_anchor`** : population COMMUNE q3/q4 — Σ tailles de cover à l'ENTRÉE
  du corps par ancre, créditée au site d'appel commun (après le prétest par
  requête, avant W3/W4/secteurs/grille). La population elle-même est publiée
  (`entrees_ancres=q2/q3/q4`).
- **`H_scan`** : câblé — delta de `AnchorScratch::visits` par ancre entrée,
  publié `h_scan=…/…/…`.
- **`V_wspd`** : le second champ est déclaré pour ce qu'il est (évaluations
  de COUPLES de coins, jamais « appels témoins »).
- **Sonde q4 par octave** : publiées les quatre issues d'un seed —
  `octaves_q4_seeds cellules=… coeur=… corde=… passe2=…` — et le vecteur
  `ancres` est déclaré (entrées de `process_anchor_q4`, population
  `entrees_ancres[q4]`).
- **Identités fermantes vérifiées** par `bench/pentes.py` sur chaque sortie :
  Σ ancres == entrées_q4, Σ w1 == tests_cœur, Σ seeds == seeds_q4,
  Σ cellules/cœur/corde/passe2 == scalaires respectifs, et par octave
  `seeds[o] == cellules[o]+coeur[o]+corde[o]+passe2[o]`. Les huit identités
  tiennent sur un run réel (uniform 400, vérification numérique exécutée).

## P1/P2 — validateur : le test vacue est remplacé, plus cinq rejets

Vous aviez raison : `seeds_cellules` n'était pas parsé et le `-` n'était pas
vérifié. Le cas « zéro légitime » porte maintenant sur `racines_hors_corde`
(réellement parsé), mis à zéro sur les trois tailles : code 0 exigé ET `-`
affiché sur sa ligne. Ajoutés (chacun code 3, stdout vide, sans traceback) :
famille META dupliquée, entier META invalide, compteur dupliqué (ligne sweep
doublée), digest_all dupliqué, digest non hexadécimal, fichier d'extension
inattendue, et les DEUX violations d'identités d'octaves (somme et par
octave). `tests/pentes_gate.py` : nominal + 20 falsifications + zéro
légitime. Chaque compteur doit apparaître EXACTEMENT une fois ; la bijection
de fichiers couvre toute extension.

`PLAN_DE_TESTS.md` : le compte est corrigé à **27 noms distincts** tués
(la porte droprect double un nom de la boucle de conformité, comme vous
l'avez relevé).

## P2 — finaliseur littéral

`invalidate_provisional` est maintenant appelé LITTÉRALEMENT sur les retours
grand-livre et `invariant_jneg` (vos deux exceptions). Les plafonds
`CloudIndex` et la saturation u64 restent des chantiers déclarés.

## Reçu brut

`receipts/portes_rapides_cycle5_20260831/` : sortie ctest BRUTE versionnée —
60/60 (59 antérieures + `mhgp6_juge_refus_k_en_trop`), hashes des binaires,
toolchain. Plus aucun claim de portes sans reçu brut.

## Incident de campagne, gravé avant votre lecture

La campagne au pin `518e2706` a été CONTAMINÉE par ma faute : j'ai rebuild
`build/v6/mhgp6` à 12:58:38Z pendant qu'elle tournait (le script invoque le
binaire par run). 33 runs sur 36 sont au binaire épinglé ; sont invalides
`eight_clusters_16000_s5` et les runs `eight_clusters_32000` postérieurs.
Conséquence assumée : les matrices 3×3 de `uniform`, `terrain_stationnaire`
et `scanline_stationnaire` (la cible de la sonde de queue) sont COHÉRENTES
au pin ; `eight_clusters` est invalide. Le reçu de la campagne grave
l'incident ; la capture est reçue comme « baseline de sonde partielle au pin
518e2706 » (votre requalification, restreinte), l'analyse des octaves se
fait séparément sans nouveau seuil, et une campagne complète est relancée au
pin de CE commit (qui porte vos sondes exigées : issues par octave,
populations déclarées, identités fermantes).

## G4 : protocole livré, aucune dépense avant vos verrous

`gcp-migration/{session_campagne_v6_g4.sh, v6_campaign_pin.sh,
v6_campaign_remote.sh, validate_v6_campaign.py, selftest_campagne_v6.sh}` :
session transactionnelle épinglée (conformité v5≡v6 à 50 000 avec référence
v5 calculée sur la VM, bench apparié ABBA sans digest, queue stationnaire
64k/128k/256k), échéance de troncature, rapatriement toujours, validateur
local seul décideur. Selftest à faux pilotes : 22/22 (nominal + refus +
9 falsifications). La session sera lancée APRÈS ce cycle, au pin corrigé.

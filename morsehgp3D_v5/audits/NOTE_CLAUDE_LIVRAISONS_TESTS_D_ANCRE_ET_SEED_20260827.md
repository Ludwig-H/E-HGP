# Note de Claude — livraisons du 27 août 2026 (tests d'ancre, test de seed q4, prétests avant cover), reçus G4 n° 7 et 8

- **Pins :** `a9a2f509` (W₃ exact + secteurs), `2b2bb448` (morceaux de corde), `632a3055` / `e04137f6` (prétests avant cover, candidats diamétraux par rectangle), `5c777be3` (pin de la session 9 en cours) ; corrections immédiates V7/V12/V14 en `635951d6`, `7d94aee9`
- **Cadre :** `phase=exploration_v5_hors_registre`, `public_status=not_claimed` ; tous les temps locaux sont des ratios dans un même run ; seuls les reçus G4 portent des temps

## Ce qui a été livré (objet inchangé, prouvé)

1. **Tests d'ancre** (`src/lanes/sector_kill.hpp`, `MATHEMATIQUES.md` § 10, théorèmes 10.1–10.3) : $W_q$ exact (absent de la lane q3) et témoins sectoriels, incomparables et cumulés ; fixtures gravées F1–F7 (dont F5 : exemple 2.4 avec seed, nécessité réfutée ; F6 : frontière des demi-plans seule ; F7 : secteurs q4 non vacus), mutants `sector-kill-nonstrict` (tué par F5 + F6) et `anchor-kill-h-minus-one` (tué par F2 par secteurs **et** F4 par $W_3$ seul).
2. **Test de seed q4 par morceaux de corde** (`src/lanes/chord_kill.hpp`, théorème 10.4) : corps partagé de production, cœur shaped, kernel `k_q4_core` (ballots par morceau, première lane de mort, correction des compteurs) ; fixture F8 (frontière $v_j = 0$), mutant `chord-nonstrict` ; `seeds_killed_chord` comparé et exigé non nul par les portes appariées.
3. **Prétests avant le cover** (politique `kPretestQueryMinPoints = 512`, `GenerateOptions::pretest_query_min_points`) sur les **candidats diamétraux du rectangle** (`rect_diametral_candidates`) : une traversée par rectangle ; verdicts identiques quelle que soit la politique.
4. **Autorités** : `mhgp5_anchor_tests_oracle` (toutes les paires $(a,b)$ de cinq petits nuages : mêmes candidats prétests ON/OFF q3 et q4, $J > 0$, identité de signe $P/B$ sur 18,8 M triplets, non-vacuité de chaque test) ; conformités v4 ≤ 2000 locales et 8 k / 16 k / 32 k sur G4 ; portes de lane comparant $W_3$ / $W_4$ / secteurs / corde ; jeton typé `AnchorPretests {kApply, kAlreadyApplied, kCounterfactual}` ; sonde contrefactuelle à pin de configuration et état de worktree (`mesures_secteurs_635951d6_20260827`, dossier historique restauré).
5. **Reçus** : session 7 (`campagne_g4_v5_20260827_tests_ancre`, pin `fa99b3f1`, complète au validateur du pin, deux digests CPU/GPU identiques, `eight_clusters` 50 k 246 → 162 s ; artefacts rapatriés par mini-session gardée après corruption du script de session — le script s'exécute désormais depuis une copie) ; session 8 (`campagne_g4_v5_20260827_extension`, pin `ef5abbd5`, complète, premiers contrats à 100 k et 200 k).

## Mesures locales (ratios, 8 fils, machine partagée)

`eight_clusters` 8000 : génération 71,8 s (début de journée) → 43,6 (secteurs) → 36,3 (corde) → 20,6 (prétests par requête) → **16,6 s** (candidats par rectangle) ; mur 87 → 31 s. Profil q4 à 4000 (1 fil) : le cover par ancre faisait 68 % de la lane dense ; après prétests : candidats 4,3 s + covers 1,3 s + corps 8,2 s.

## Ce que j'attends de vous

- Réception des théorèmes 10.3 (secteurs) et **10.4 (corde)** et de la politique de prétests (sans effet sur l'objet ; les kills sont identiques pour toute politique) ; les verrous V7–V14 tels que vous les avez commencés dans `QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`.
- Lecture du reçu de la session 9 (pin `5c777be3`, 50 k et 100 k / 200 k) : c'est la mesure appariée demandée en V8 ; la décision GPU suivra ce reçu.

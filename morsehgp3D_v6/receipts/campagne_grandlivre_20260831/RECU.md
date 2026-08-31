# Reçu — porte à quatre familles du grand-livre (31 août 2026, soir)

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.
Provenance dans `META.txt` (SHA du pin, hash du binaire instrumenté,
toolchain, commande, heures, hashes des 36 sorties). 36/36 code 0, stderr
vides. Analyse `PENTES.txt` par `bench/pentes.py` **fail-closed** (matrice
depuis le META, STATUS prévalidé, identités recoupées, chaque compteur
exigé). Cette campagne remplace la baseline post-correctif comme autorité
des pentes ; elle porte les termes absents de la baseline : **`W_sweep2`**
(`tests_passe2`, le rescan complet de la passe 2), **`tri_comparaisons`**
(diagnostic sous toolchain épinglée), **`P_factor`** (auto-produits des
histogrammes, définition exacte sans diagonales).

## Verdict par régime (pentes sécantes, deux pas, trois graines)

- **uniform** : tous les termes publiés dans `[1,03 ; 1,20]`, étendues
  ≤ 0,07 — y compris `W_sweep2` (1,06–1,09) et `P_factor` (1,03–1,13).
- **eight_clusters** : tout < 1,76. Les plus hauts : `P_factor_q3`
  1,69–1,75 (pas 1), `ancres_q4` 1,67–1,69 (pas 2), `P_factor_q4`
  1,60–1,70 — le bloc ancres/auto-produits inter-amas, étendues ≤ 0,10 :
  reproductible, c'est LA cible de la route M (requêtes de facteurs
  saturées). `W_sweep2` 1,44–1,57.
- **terrain_stationnaire** : `W_sweep2` 0,99–1,32, `tri_comparaisons`
  0,97–1,49, `P_factor_q4` ≤ 1,64 ; le pic mono-graine de `W_sweep1`
  persiste (2,08 graine 5, pas 2 ; 1,30/1,32 ailleurs).
- **scanline_stationnaire** : `W_sweep2` 0,91–1,16 ; `W_sweep1` pique à
  2,41 (graine 5, pas 2 ; 0,91/1,21 ailleurs), `seeds_q4` 1,54 même graine.

## Conséquences

1. Le déclencheur E6 du grand-livre reste ACTIVÉ par `W_sweep1` (queue
   lourde mono-graine sur les surfaces stationnaires) — inchangé depuis la
   baseline, désormais établi sur le grand-livre complet. Prochain
   chantier : diagnostic de queue (distribution de `m_e` par octave, part
   des ancres lourdes — candidat J3) puis sonde contrefactuelle de Tier R /
   moteur plan / contrat 2.
2. La passe 2 du sweep (`W_sweep2`) est PARTOUT sous-quadratique et d'un
   ordre de grandeur sous `W_sweep1` : le remplacement du rescan par
   candidat tient sa promesse de coût ; le mur résiduel est la passe 1
   (scan cœur des seeds), pas le sweep.
3. `P_factor` sur `eight_clusters` (≤ 1,75) borne la contribution des
   auto-produits : sous-quadratique mais le plus raide des termes de
   préparation — la route M a maintenant sa justification chiffrée.

Toujours hors périmètre : `H_rect/H_scan/M_anchor/V_census/T_input`
(candidat J3), HWM par rôle, temps (machine partagée). Aucun GO global
n'est prononcé ; les pentes valent pour les termes publiés, trois graines,
deux pas, tailles 8000→32000. GCP non utilisé.

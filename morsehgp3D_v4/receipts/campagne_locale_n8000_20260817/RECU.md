# Reçu — campagne locale n=8000 : 4 familles × 2 profils (conteneur 4 vCPU / 15 Go)

Date : 17 août 2026. Pin : `source_commit=72138ddf` (statuts joints).
Séquentielle (un run à la fois), `timing_scope=local_container_sequential`
— PAS le matériel contractuel (la G4 reste la cible ; session pilote en
cours par ailleurs). Champ `peak_rss_kb` des statuts INVALIDE (le même
défaut d'échantillonnage parent/enfant que le reçu RSS a documenté —
la référence valide reste 6,8 Go pour uniform smax=11, mesure dédiée).

## Mesures (durées totales ; événements)

| famille | smax=11 (K_max=10) | smax=6 (K_max=5) |
|---|---|---|
| uniform | 343 s ; 3 126 158 év. | 54 s ; 602 204 év. |
| terrain | 162 s ; 605 870 év. | 54 s ; 184 162 év. |
| scanline_overlap_multiecho | 697 s ; 1 094 377 év. | 153 s ; 273 161 év. |
| eight_clusters | **TIMEOUT à 5 403 s** (code=124) | 1 427 s ; 533 284 év. |

Tous les runs terminés : code=0, `juge=off desaccords=NA` (mesure de
coût, jamais une preuve — les invariants sont les 93 portes CTest).

## La découverte : eight_clusters est LE cas dur

- smax=11 à n=8000 : ne finit pas en 90 minutes ; n=2000 ne finit pas
  en 9 minutes ; à n=1000 déjà, `t_gen = 136 s` avec **119 653 085**
  évaluations q4 tuées pour 87 048 candidats émis — l'explosion est
  dans les boucles seed × complétion sur covers denses (4 416 744
  seeds à n=1000), pas dans l'objet (219 653 événements) ni dans
  l'aval (count-only 1 s, fold 3 s).
- Même à K_max=5, la génération domine tout : 1 413 s de `t_gen` sur
  1 427 s de run.
- **Premier résultat positif du chemin axial opt-in** (mesuré n=1000,
  sorties identiques — 219 653 événements) : `--axial-on` réduit les
  évaluations tuées de 420× (119,6 M → 285 028) et `t_gen` de −29 %
  (135,9 → 96,4 s). Le régime dense est celui que le reçu axial
  prédisait ; le poste restant est le balayage A,B des 4,4 M de seeds —
  exactement ce que le sweep à deux côtés du contre-audit (tâche
  ouverte) attaque, avec en prime la mort des groupes par le côté
  opposé AVANT `valid_completion`.

## Conséquences

1. Le sweep axial à deux côtés monte en priorité 1 de code : il est la
   voie chiffrée vers un eight_clusters praticable, et la campagne G4
   (couverture) butera sur cette famille tant qu'il n'est pas là.
2. Les vagues de couverture G4 devraient traiter eight_clusters
   smax=11 comme run À RISQUE (timeout 3 h dédié) — le lanceur borne
   déjà par run, le verdict partial le matérialisera proprement.
3. n=16000/32000 : hors de portée locale (RAM) — G4.

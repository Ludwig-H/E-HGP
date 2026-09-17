# Clôture des lots de petits census q2 : résultat négatif qualifié

17 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Flux q2 complet de supports, intérieurs stricts et coquilles complètes.
**Ni moteur q3/q4, ni HGP FULL, ni qualification G4. GCP non utilisé.**

La dix-neuvième tranche teste une hypothèse précise, énoncée dans la
[note de contrat](../../docs/P0_LOTS_SINGLETON_Q2.md) : entrelacer les petites
recherches q2 indépendantes dans un lot réutilisable pourrait masquer des
attentes mémoire. **L'hypothèse est réfutée par la mesure** : le format à lots
est exact, tous ses comptes géométriques égalent ceux de Coarse, mais il est
plus lent que Coarse dans toutes les comparaisons aux tailles d'intérêt, et
l'entrelacement (8 ou 16 voies) n'apporte rien par rapport à une seule voie.
L'entrée `run_wspd_q2_census_batched` reste une entrée explicite hors défaut,
conservée comme témoin ; Coarse reste le défaut.

## Captures propres de clôture (sources r3 gelées le 17 septembre)

| Capture | Périmètre | État |
|---|---|---|
| [qualification_gvefg3e1](qualification_gvefg3e1/COMPLETION.json) | 75 CTests Release GCC 13.3 + porte explicite des lots | PASS |
| [qualification_re7v36lk](qualification_re7v36lk/COMPLETION.json) | 75 CTests Clang 18 ASan/UBSan + porte explicite | PASS |
| [tsan_20ywjox6](tsan_20ywjox6/COMPLETION.json) | Porte des lots sous Clang ThreadSanitizer | PASS |
| [smoke_siw9pvdp](smoke_siw9pvdp/COMPLETION.json) | 4 familles n32, W1/W4, 1/8/16 voies, quantum 1 | 32 mesures closes |
| [tuning_g14oauuc](tuning_g14oauuc/COMPLETION.json) | Uniforme et terrain 8k, 1/8/16 voies, quantum 64 | 8 mesures closes |
| [scale_xe210qze](scale_xe210qze/COMPLETION.json) | 4 familles n8k/16k/32k, K10/s8, Pool64, 1 voie, quantum 64 | 24 mesures closes |
| [scale_bq3r9yak](scale_bq3r9yak/COMPLETION.json) | Même grille, 16 voies, quantum 64 | 24 mesures closes |
| [separations_sryfuunq](separations_sryfuunq/COMPLETION.json) | 4 familles n8k, K5/10, s8/10/12, 1 voie, quantum 64 | 40 mesures closes |
| [parallel_0d5skgwm](parallel_0d5skgwm/COMPLETION.json) | 4 familles n32k, quatre workers, 1 voie, quantum 64 | 8 mesures closes |

Les 26 commandes de lecture et d'analyse normal/−O sont closes dans
[analysis_p7f6ps3f](analysis_p7f6ps3f/COMPLETION.json) ; la
[synthèse déterministe](analysis_p7f6ps3f/SUMMARY.json) est identique dans
les deux modes : 172 mesures (64 références Coarse,
108 appels à lots), 108 comparaisons appariées,
120 sources, 94 artefacts des builds et toutes les entrées recontrôlés à la
fermeture. Chaque paire garde les mêmes coordonnées, K, s, seuil Pool et
nombre de workers ; sorties, clés, intérieurs, coquilles, callbacks et tous
les comptes de front, census, frère, ordre et Pool concordent.

Les trois portes explicites (Release, ASan/UBSan, TSan) comparent chacune
595 appels à lots, 178 Coarse et 13 défauts sur 13 nuages :
10 958 paires et 778 276 tests de sites de l'oracle indépendant,
124 145 supports vérifiés, coquille 30, 177 945 tâches compactes,
9 830 entrées après crédit, 4 482 en phase différée, 40 054 frères
dus, 14 entrées invalides refusées et 7 mutants tués.

## Captures historiques et échecs conservés

| Capture | Révision | État |
|---|---|---|
| [tuning_r9hbnll1](tuning_r9hbnll1/COMPLETION.json) | r0, quantum 1, uniforme et terrain 8k | PASS historique |
| [quantums_n5elpzvo](quantums_n5elpzvo/COMPLETION.json) | r0, quanta 8/64 | PASS historique |
| [quantums_5c5xaxk6](quantums_5c5xaxk6/COMPLETION.json) | r2 (boucle locale des témoins), quanta 8/64 | PASS historique |
| [analysis_9vfx68d1](analysis_9vfx68d1/COMPLETION.json) | Analyse des trois captures précédentes | PASS historique |
| [tuning_z6h34zxc](tuning_z6h34zxc/COMPLETION.json) | Tuning lancé avant la fin d'une recompilation, arrêté | FAILED conservé |
| [quantums_1hawsslw](quantums_1hawsslw/COMPLETION.json) | Défaut du helper de quanta, zéro record | FAILED conservé |

Trois révisions du moteur ont été mesurées, chacune avec ses propres pins :
r0 (census b8fa8bb0…, un appel de progression par témoin), r2 (census
6fffc6fc…, boucle locale des témoins, même géométrie) et r3, la source de
clôture (census bdc5d09d…, en-tête 9478f2f1… inchangé depuis r0). r3 ajoute à
r2 la garde « lane terminée redistribuée », le commentaire de validité du
registre, le plancher `credited_rows` de la porte de reçus et l'option
`--quantum` du lanceur ; aucune décision géométrique ne change. Les sources
r0 sont archivées dans [pre_local_loop](pre_local_loop/), les trois sources
r2 modifiées dans [pre_closure_r2](pre_closure_r2/), hachages identiques
aux pins des captures historiques. Le journal des essais, échecs compris,
est dans [PREFLIGHT.md](PREFLIGHT.md).

## Temps : régression partout aux tailles d'intérêt

Observations uniques par configuration, hôte partagé, affinité libre sur
huit fils logiques, sans autre campagne ni compilation pendant les captures
de clôture. Temps mur du pipeline q2 complet (front, census, collecte,
callbacks), K = 10, s = 8, Pool 64, un worker, quantum 64 :

| Famille | n | Coarse | Lots, 1 voie | Coarse | Lots, 16 voies |
|---|---:|---:|---:|---:|---:|
| Uniforme | 8 000 | 5,01 s | 5,63 s (×1,12) | 4,99 s | 5,72 s (×1,15) |
| Uniforme | 16 000 | 11,91 s | 13,54 s (×1,14) | 11,89 s | 13,51 s (×1,14) |
| Uniforme | 32 000 | 27,72 s | 31,71 s (×1,14) | 27,77 s | 31,94 s (×1,15) |
| Terrain | 8 000 | 0,90 s | 0,98 s (×1,09) | 0,91 s | 0,99 s (×1,09) |
| Terrain | 16 000 | 1,89 s | 2,07 s (×1,10) | 1,89 s | 2,08 s (×1,10) |
| Terrain | 32 000 | 4,10 s | 4,58 s (×1,12) | 4,15 s | 4,57 s (×1,10) |
| Amas | 8 000 | 2,66 s | 3,04 s (×1,14) | 2,71 s | 3,06 s (×1,13) |
| Amas | 16 000 | 7,47 s | 8,44 s (×1,13) | 7,50 s | 8,51 s (×1,13) |
| Amas | 32 000 | 19,09 s | 21,87 s (×1,15) | 19,06 s | 21,77 s (×1,14) |
| Rangées | 8 000 | 0,23 s | 0,23 s (×1,01) | 0,21 s | 0,22 s (×1,05) |
| Rangées | 16 000 | 0,44 s | 0,46 s (×1,05) | 0,44 s | 0,46 s (×1,05) |
| Rangées | 32 000 | 0,91 s | 0,95 s (×1,04) | 0,92 s | 0,94 s (×1,03) |

Quatre workers à 32k, une voie, quantum 64 :

| Famille | Coarse W4 | Lots W4 | Rapport |
|---|---:|---:|---:|
| Uniforme | 7,47 s | 8,50 s | ×1,14 |
| Terrain | 1,07 s | 1,19 s | ×1,12 |
| Amas | 4,99 s | 5,63 s | ×1,13 |
| Rangées | 0,23 s | 0,28 s | ×1,23 |

Sur les 54 comparaisons de clôture à n ≥ 8 000, le rapport lots / Coarse
va de 1,005 à 1,225, médiane 1,112 ; **aucune n'est plus
rapide que Coarse**. Par campagne : échelle une voie 1,01 à 1,15, seize
voies 1,03 à 1,15, séparations 1,04 à 1,14, quatre workers 1,12 à
1,23, tuning 1,08 à 1,15. Une voie et seize voies donnent les mêmes temps
à un ou deux centièmes près : l'entrelacement ne masque aucune attente
mémoire mesurable, et le surcoût restant est celui du format (copie
d'état, aiguillage par étape, compteurs de gestion).

Historique, quantum 1 : r0 donnait ×1,29 à ×1,76 à 8k (uniforme
et terrain) ; aux quanta 8/64, r0 ×1,15 à ×1,53, r2 ×1,07 à
×1,35. La boucle locale de r2 a donc réduit le surcoût sans jamais
l'annuler. L'auditeur B mesure indépendamment +40 à +70 % à quantum 1 sur
8k/16k/32k (`audits/chaine_q2_20260914/CHAINE_Q2_BATCHED_CHECKS.json`),
0 désaccord contre la force brute sur 5 256 appels à lots.

## Ce que les lots ne changent pas

Les visites Z du census sont exactement celles de Coarse et gardent les
mêmes croissances (une voie, quantum 64) :

| Famille | 8k | 16k | 32k | Ratios |
|---|---:|---:|---:|---|
| Uniforme | 171 895 354 | 413 553 244 | 1 001 201 993 | ×2,406 / ×2,421 |
| Terrain | 20 472 635 | 42 798 408 | 95 128 515 | ×2,091 / ×2,223 |
| Amas | 81 112 664 | 239 954 275 | 648 207 562 | ×2,958 / ×2,701 |
| Rangées | 5 742 485 | 11 886 273 | 24 566 835 | ×2,070 / ×2,067 |

Sur l'ensemble des mesures de clôture, 219 003 796 états singleton ont été
enfilés et complétés, dont 24 854 247 admis, 48 137 841 entrés
après crédit, 1 557 044 en phase différée et 127 783 872 avec un
frère encore dû ; 381 vidages avant Pool (amas et rangées) et
51 rejets du frère saturant (séparations K = 5) exercent ces
raccords à l'échelle, ce que les captures r0/r2 ne faisaient pas. L'état
occupe 72 octets, au plus 16 états actifs par worker. Les ratios Pool des
amas (paires filtrées ×4,001 / ×4,000) et l'amorçage des insertions Pool des
rangées restent publiés dans la synthèse ; ils sont ceux de Coarse.

**Aucune borne générale sous-quadratique, P0 global ou tour FULL/G4 clos.**

## Limites de couverture

- `GlobalDfs`, défaut de l'API, n'est exercé que par la porte C++ contre
  l'oracle (n ≤ 100, quanta 1/7/256, 1 à 16 voies) ; la sonde, la porte de
  reçus et les campagnes fixent `ComplementFirst`, `Saturating` et
  `MidpointSamples` comme les sondes q2 depuis la dixième tranche.
- Le registre des lots n'est valide que sur succès de l'appel entier :
  `witness_steps` et `transitions` sont engagés une fois par visite de voie.
  Un appel en échec ne publie aucun registre.
- Temps : une observation par configuration ; seuls les comptes discrets
  sont déterministes. Aucun temps local ne qualifie un contrat G4.

## Builds épinglés

`build/v8_singleton_batch_r3_20260917`, `build/v8_singleton_batch_sanitize_r3_20260917`
et `build/v8_singleton_batch_tsan_clang_r3_20260917` portent la clôture ;
`build/v8_singleton_batch_20260915` (r0) et `build/v8_singleton_batch_r2_20260915`
(r2) restent épinglés à leurs captures historiques. Ne pas les écraser. Les
dossiers `build/v8_singleton_batch_sanitize_20260915` et
`build/v8_singleton_batch_tsan_clang_20260915` n'ont jamais été construits.

## Rejouer et suite

Le [lanceur](../../bench/run_wspd_q2_batched_checks.py) fournit `read` pour
chaque capture et `run --campaign qualification|tsan|smoke|tuning|scale|parallel|separations`
avec `--lanes` et `--quantum` (absent des manifestes historiques, lu comme 1)
dans un build neuf. `analyze.py` lit des captures explicitement nommées ;
`record_analysis.py` préserve ses lectures normal/−O, les captures
historiques et les échecs dans un dossier neuf.

Suite : la piste « lots compacts » est fermée ([fausses pistes](../../docs/FAUSSES_PISTES.md)).
Le levier suivant ne change ni le format ni l'ordonnancement : il réduit le
nombre de petites requêtes elles-mêmes, en rejetant davantage de produits
au front ([surproposition de témoins](../../docs/P0_SURPROPOSITION_TEMOINS_Q2.md)).

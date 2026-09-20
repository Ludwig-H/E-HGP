# Tranche29 — fermeture des lectures

Les quatre captures suivantes sont closes et leurs reçus passent en Python
normal et optimisé (`-O`). Aucun build ni calcul historique n’est recapturé
par cette lecture. Les deux moteurs ont réellement été exécutés lors des
captures de mesures ; le lecteur vérifie leurs données enregistrées.

| Capture | Qualification | Commandes | Mesures |
| --- | --- | ---: | ---: |
| [smoke_254ekmp2](smoke_254ekmp2/COMPLETION.json) | Release : huit gates et dix comparaisons | 18 | 10 |
| [smoke_us_t5o8a](smoke_us_t5o8a/COMPLETION.json) | Clang ASan/UBSan : mêmes gates et comparaisons | 18 | 10 |
| [scale__b3aquyb](scale__b3aquyb/COMPLETION.json) | Release : huit gates et trente-deux comparaisons | 40 | 32 |
| [regression_rnihy_44](regression_rnihy_44/COMPLETION.json) | Release : 90 CTests distincts, aucun échec ni saut | 1 | 0 |

La nouvelle gate compte 3 686 contrôles, avec les mêmes 40 métriques en
Release et sous sanitizers. Cela ne signifie pas que les 90 CTests ont tous
été rejoués sous sanitizers dans cette tranche.

## Fermeture reproductible

[readers_rl6aro8o](readers_rl6aro8o/COMPLETION.json) conserve les dix commandes
et leurs sorties : quatre lectures `--check-live` et un autotest du lecteur,
puis les cinq mêmes commandes sous `-O`. Les résultats sont identiques ;
l’autotest rejette 41 corruptions de reçus, pas 41 mutants géométriques.

Les empreintes avant/après concordent pour **184 sources, 87 fichiers
d’entrée et 74 artefacts**, sans erreur de fermeture. Le helper
[close_reads.py](close_reads.py), hors inventaire moteur, est lui-même épinglé
par cette capture. Aucun des 184 fichiers qualifiés n’a été modifié pendant
les captures et les lectures.

```text
python3 -B morsehgp3D_v8/receipts/q4_shallow_20260920/close_reads.py \
  morsehgp3D_v8/receipts/q4_shallow_20260920/smoke_254ekmp2 \
  morsehgp3D_v8/receipts/q4_shallow_20260920/smoke_us_t5o8a \
  morsehgp3D_v8/receipts/q4_shallow_20260920/scale__b3aquyb \
  morsehgp3D_v8/receipts/q4_shallow_20260920/regression_rnihy_44 \
  --selftest morsehgp3D_v8/receipts/q4_shallow_20260920/smoke_254ekmp2
```

SHA256 de la fermeture :
`8507d4d4a2dcb2b2abdd62478ccbd7c98d7673c3717753d2f22ca61506079050`.
SHA256 de [SUMMARY.json](readers_rl6aro8o/SUMMARY.json) :
`dac1d38b68fd8e60fcb3926e93eeddb8ceb8d0269b710c624d233136fd9f866d`.
Le résumé contient tous les ratios de croissance, y compris ceux supérieurs
à quatre ; les fichiers `normal_*.json` et `optimized_*.json` conservent les
commandes exactes, codes de retour et sorties brutes.

## Portée et résultats à ne pas masquer

Chaque mesure traite **une arête fournie**, pas le producteur global ni une
tour HGP complète. La référence est la voie q4 locale28, exécutée de nouveau
sur le même nuage/index/cover. Les clés, supports, profondeurs et coquilles
sont comparés intégralement. Un juge rationnel indépendant valide chaque
support publié puis effectue un census global une fois par boule publiée ;
ce juge ne prouve pas, à lui seul, la complétude sur les grands nuages denses.
Les petites gates et leurs oracles exhaustifs ont une portée distincte.

Sur `dense_permuted`, K10, aux tailles 8k/16k/32k :

- Sites retenus : 408 / 708 / 1 256 ; graines : 406 / 706 / 1 254.
- Visites effectives : 165 648 / 499 848 / 1 575 024, soit ×3,018 puis ×3,151.
- Comparaisons de tri : 1 675 241 / 5 445 436 / 18 920 906,
  soit ×3,251 puis ×3,475. À 8k, le nouveau tri coûte davantage de
  comparaisons que la référence locale28 malgré beaucoup moins de visites.
- Préparation et parcours29, callbacks inclus : 53,235 / 167,345 / 562,050 ms,
  contre 64,416 / 260,515 / 1 105,682 ms pour28. Ce sont des observations
  uniques sur hôte partagé, pas un gain stable établi.

Le résultat négatif adversarial K10 demeure : à n32/64/128/256, tous les sites
restent retenus. Les visites font 960 / 3 968 / 16 128 / 65 024, donc chaque
doublement dépasse ×4 ; les tris font 4 943 / 25 374 / 133 728 / 623 717.
À n256, la nouvelle voie prend 18,302 ms contre 1,783 ms pour28.

Le régime `cap` est lui aussi défavorable en temps, même si les couches
réduisent réellement le nombre de sites : à32k/K10, 1 149 sites sont gardés,
mais le parcours29 prend 17,468 ms contre 0,158 ms pour28, dont les blocs
réduisent déjà les visites à22. La préparation commune incluse, les sommes
de pipeline valent 30,886 ms et 13,577 ms ; elles partagent la préparation
du nuage et ne sont pas deux temps mur indépendants.

Aucune borne générale sous-quadratique, aucun contrat50k/FULL et aucun
résultat G4 ne sont acquis ici. Le volume retenu peut rester égal au volume
initial. Les pics mémoire publiés sont des capacités couplées, pas le RSS.

Les préflights conservés dans [preflight](preflight) restent distincts des
captures closes ci-dessus. Les compléments de mutation causale et de
différentiel par défaut ont leurs propres lectures dans [mutants](mutants).

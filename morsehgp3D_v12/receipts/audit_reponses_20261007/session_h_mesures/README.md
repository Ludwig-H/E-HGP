# Session H : contre-lecture des mesures G

Pin de publication `136e0762898ce8dd40c96dafba6b2d616f3b7b1f`. Source exécutée : snapshot `9c5809919fa4fcaa6303f9265aadbad60a7ddeea`, **CPU de référence, u21, Release GNU 11.4.0, 48 fils** sauf la prise mono. Les 32 empreintes du reçu H et 136 entrées produit/bench/CMake du manifeste source ont été revérifiées. Ancres et hashes dans `results.json` ; aucun build, moteur ou appel cloud par cet audit.

Médianes des passes 1..P−1 après exclusion de la passe 0, et maximum du même ensemble, recalculés depuis les JSONL :

| Cas | Sites | K5 médiane / max (ms) | K10 médiane / max (ms) |
| --- | ---: | ---: | ---: |
| ng00 | 39 885 | 80,131669 / 83,295857 | 633,6273595 / 645,571687 |
| ng01 | 35 551 | 63,235360 / 64,723668 | 449,309488 / 457,599458 |
| ng02 | 45 845 | 75,703012 / 77,781820 | 517,490754 / 518,156419 |

Un processus par cas : **9 valeurs chaudes en K5, seulement 2 en K10**. Le maximum est observé sur ces passes, pas une borne de latence ni un quantile robuste. ng00 K5 mono : une seule passe chaude, 1 567,262137 ms. Uniformes K5 à 48 fils, quatre passes chaudes : 8 000 / 16 000 / 32 000 sites, médianes 31,226380 / 75,438239 / 181,889853 ms. Les tableaux principaux G/tables/résolution du README H sont conformes à leur arrondi.

**Frontière.** `bench/tower_probe.cpp:178–218` construit nuage, index, pool et catalogue avant le chronomètre ; chaque passe chronomètre `resolve_tower`. Digest, export, impression et destruction de la résolution rendue sont ensuite hors chrono. Les sommes avec le catalogue F2 ne sont pas des temps intégrés. Il n'y a ici ni catalogue CUDA, ni FULL T/M/V/R, ni nouveau LiDAR multi-millions. ng00–02 sont les trois trames historiques de la même séquence SemanticKITTI 08, sans sol et sur grille 1 mm (`docs/MESURE.md:24`), pas une campagne multi-séquences.

Les pics publiés sont les `MemoryBudget::peak()` persistants depuis le début du processus (`stage.cpp:270`), incluant l'amont, et non une mesure RSS propre à G. `results.json` conserve séparément ces octets et le RSS du processus issu des métadonnées. La sonde ne publie que **le digest de la dernière passe** : aucune égalité inter-passes n'est prouvée. Le digest complet de ng00 K5 est égal entre les deux processus 1/48 fils ; les références CTest locales gravées pour ng00 et les uniformes portent 16 chiffres hexadécimaux, dont la correspondance est vérifiée ici.

**Diagnostic conditionnel, par passe.** Sur le `stage.cpp` H de hash `2682f239…`, les plages count/fill/tables/resolve sont disjointes. Pour chaque passe chaude, on calcule `r = wall − tables − resolve − count − fill`, puis `projection = wall − tables − resolve/2 + 3 ms`. Ce sont des opérations sur les mesures présentes, pas des temps futurs mesurés, ni une somme de médianes :

| K5, 48 fils | r médiane / max (ms) | wall−tables médiane (ms) | Projection médiane / max (ms) |
| --- | ---: | ---: | ---: |
| ng00 | 10,842193 / 10,957833 | 61,512931 | 40,1627165 / 40,4938415 |
| ng01 | 9,041494 / 9,181184 | 48,072839 | 32,558031 / 33,193606 |
| ng02 | 11,219342 / 11,356424 | 56,829094 | 38,068392 / 38,371723 |

Ainsi, même sous l'hypothèse tables à 3 ms et résolution divisée par deux, **le budget G de 25–30 ms reste dépassé**, si tout le reste reste inchangé. Ces formules ne se transfèrent pas au prototype G-c, où les bornes des compteurs ont changé. Le ×30 publié concerne seulement `resolve_ns` sur ng00 ; le facteur mesuré de tout G est ≈19,6, avec une seule valeur chaude mono et sans alternance répétée.

**Écart documentaire à corriger.** La colonne « par ordre » de H mélange les agrégations : ng00 K5 a pour médianes 0,243660 / 3,317428 / 6,694315 / 12,049443 / 26,244813 ms, différentes des arrondis affichés. À K10, l'ordre 10 a pour médianes ng00/01/02 : 159,724108 / 106,531287 / 114,238787 ms ; les valeurs du README (159,2 / 106,6 / 114,1) suivent la première passe chaude. Étiqueter cette sélection ou utiliser les médianes ; aucun changement aux médianes globales de G.

**Clôture et qualification.** L'archive atteste 637/637 portes rapides et 6/6 LiDAR sur le snapshot ancien ; elle ne requalifie pas les juges durcis ultérieurement. Les dix commandes G ont code 0, groupe fermé et flux non tronqués. Le statut global `failed_remote` vient de la coupure MES-P à l'échéance ; cette commande partielle ne devient pas conforme par la réussite de G. Le reçu rapporte `closure=stopped`, arrêt ciblé code 0 et `TERMINATED`, avec génération de démarrage inchangée et `lastStopTimestamp=2026-10-07T15:20:19.736-07:00` (22:20:19.736 UTC). Nous vérifions cette preuve archivée, sans nouvel appel GCP. La cohorte MES-P est auditée séparément.

Rejeu : `python morsehgp3D_v12/receipts/audit_reponses_20261007/session_h_mesures/replay.py --repo . --out /tmp/h.json`. Rejoué aussi avec `python -O`, résultat identique octet pour octet à `results.json`. Le script relit les commits épinglés, contrôle les sorties et recalcule les valeurs ; aucun payload ni log massif n'est dupliqué ici.

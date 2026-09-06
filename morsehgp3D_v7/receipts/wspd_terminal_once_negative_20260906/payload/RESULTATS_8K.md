# Terminal-once : aucun gain observé, ne pas intégrer en l'état

Le 6 septembre 2026, une seule mesure par bras, référence puis candidat, CPU 6, un thread, sans compilation ni autre benchmark simultané selon la coordination ROOT. Famille uniforme, graine 3, n=8 000, coordonnées 65 536, s=8, seuils 10/9/8, masque 7. Le chronomètre couvre seulement `alive_rectangles_fused`, pas l'entrée, l'index ni la sérialisation des rectangles. Il ne mesure ni la génération complète ni la tour FULL.

| Mesure | Référence q2-reuse | Terminal-once | Variation |
|---|---:|---:|---:|
| Front | 37 767,100536 ms | 38 286,547152 ms | +1,375 % |
| Nœuds témoins visités | 563 616 452 | 547 864 549 | −2,795 % |
| Tests de coins | 167 115 088 | 335 509 837 | +100,766 % |

Les 754 686 rectangles, dans leur ordre, avec références des deux nœuds, masque et trois crédits, sont littéralement identiques entre bras. Les masses, pics, compteurs de rectangles, nombre de travailleurs et autres métadonnées non mesurées sont également identiques. SHA-256 de cet objet JSON canonique : `3e99fb0b6c6e4f1f34ddd02c046ae790720c2ab4a6e84b1f03f243e9235fab58`.

Le résultat illustre le compromis prévu par la preuve : supprimer le premier passage fait payer l'autorité de coins aux lanes que ce premier passage aurait déjà fermées. L'économie de parcours observée sur les 174 petits fronts ne suffisait pas à prédire le temps de cette entrée. Une paire unique ne permet pas d'affirmer une régression statistique de 1,375 %, mais elle n'apporte aucun gain observé justifiant l'intégration. Pas de répétition, pas de variante corrective ajoutée, prototype laissé privé. `public_status=not_claimed`.

## Captures closes

Les deux compilations O2 et deux mesures sont closes, code 0, stderr vides ; les groupes de processus sont fermés. PID référence 470735, candidat 471797. La branche benchmark lance directement `taskset -c 6` et attend sans timeout ni nouveau rlimit ; elle n'utilise pas les limites 60 s/45 s du helper de petites fixtures. Le helper et les captures historiques restent intacts. Le lanceur de mesure corrigé est `2294ebba5f2fbbfc84c9fab147863a5c70e3b0b0c2757c2b846208d38040ca2e` ; la sonde C++ est `f7a5299827fb521d0d0b0559f4f0f170162bd46591397c0e382bc1ad529d4cb3`.

- Résultat : `measure_logs/result.json`, SHA `73ceddbb2ce27be5453d7c2fcc14cfd86ffe5502cffd914d5cec8ba3e9ff4829`.
- Commande référence : `measure_logs/reference_measure.command.json`, SHA `5286856668798fb7f20a0d4363a48f6217f41d13c782fd917f15fc568adb09e5`.
- Commande candidate : `measure_logs/candidate_measure.command.json`, SHA `15ff7ed434f5861a618e0b1749b029e8ebe4a1e24c6ccbbf75b4c399af09a0b1`.
- JSON brut référence : `99516c788feae9058caf22aa560179226fa00b063396ad5aac5519068602b4b5` ; candidat : `f77ad838645135d72e7b49cc2db02009e55c2000b06e23fb61f831c6f21ee507`.

Les `*.build.json` conservent les pins des binaires et de toutes leurs dépendances compilées, revérifiées avant/après chaque mesure. Aucun fichier produit modifié, aucun moteur FULL lancé, GCP non utilisé.

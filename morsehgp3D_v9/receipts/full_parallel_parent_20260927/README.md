# Encodeur première occurrence multi-CPU — captures closes

27 septembre 2026, base `fd1a2c7ee`, audit-only, moteur inchangé, GCP non
utilisé. Voir la [preuve et les limites](../../audits/b_full_parallel_parent_20260927/README.md).

Autorités : `r2/` Release + Clang ASan/UBSan/LSan, 14 commandes ;
`tsan_r1/` GCC ThreadSanitizer, quatre commandes, réellement **PASS** dès
la première tentative. Lecteurs LIVE normal/−O réussis, sans rejeu.

Chaque binaire : 372 entrées, 2 232 comparaisons exactes, 65 admises et
307 refusées. W1/W4, grains 1/7, normal/inverse ; 1 722 voies rapides,
420 replis, 90 refus préalables. Quatre threads simultanés prouvés par
barrière ; quatre workers actifs sur le minimum des parents. 13 644
créations de threads, 114 672 occurrences cumulées, douze tests ciblés
d'ordonnancement et de jointure après exception. Digest commun :
3935095278205354091. Six mutants Release/San réfutés par objet/motif erroné,
pas par crash ; le mutant d'admission de doublons n'est exécuté qu'en W1.

R1 est un échec de compilation conservé : indentation trompeuse du gate,
aucun test exécuté. Les trois copies sources `r1/sources` correspondent
exactement aux hashes `source_before`. Aucun moteur ni avertissement de
compilation n'a été modifié pour faire passer la qualification.

La réduction est réellement atomique ; le calendrier des threads ne choisit
pas le premier refus. Cette capture ne mesure cependant aucune performance.
Copies, initialisations de vecteurs et certaines grandes tâches restent
sérielles ; les retries CAS sur entrées invalides ne sont pas comptés.
Pas de transfert à une complexité globale en points ni au contrat 100 ms.

Builds clos :
`/workspaces/E-HGP/build/v9-audit-full-parallel-parent-20260927-r2/{release,sanitize}` ;
`/workspaces/E-HGP/build/v9-audit-full-parallel-parent-tsan-20260927-r1/gate`.
Les lecteurs dépendent de ces builds locaux et des sources épinglées.

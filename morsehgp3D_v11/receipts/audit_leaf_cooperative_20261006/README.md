# Réponse au design de feuille GPU coopérative — 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Conception publiée en section O au pin `3b76a3fcf0ca14dd08f005e1e1ae8e3418dd247e` ;
WIP hôte du développeur identifié séparément par empreintes. Aucun build,
test natif, benchmark ni appel GCP par les auditeurs.

**Avis favorable sous les invariants donnés dans la
[réponse active](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#feuille-cooperative--reponse-a-la-section-o-du-6-octobre).**
Les paires peuvent être calculées dans un ordre arbitraire ; leurs émissions
placées par préfixes dans l'ordre lexicographique restituent le parcours
complet. L'ordre interne des faces, du recensement et des descendants reste
conservé ; l'incertitude annule toute la feuille avant son repli CPU.

- [Preuve d'ordre, cache et témoin q3→q4](mathematics/REPORT.md) : 213 contrôles
  bornés, dont 96 comparaisons de parcours, normal/−O identiques. Le modèle
  compare des tentatives q2/q3/q4 ; le témoin géométrique q4 est prouvé
  séparément en arithmétique rationnelle. Aucun oracle complet du noyau.
- [Mémoire et synchronisation](concurrency_layout/README.md) : 496 paires,
  4 960 rangs J2, 155 mots32. Layout conditionnel 22 392 octets avec les
  structures hôtes ; exemple compact 9 016 avant métadonnées. Le `sizeof`
  et la concurrence doivent être jugés sur G4. La préparation séquentielle
  du WIP double les calculs de paires : conserver la référence 830 pour l'A/B.
- [Portes et mutants ciblés](qualification/README.md) : compléter les
  fixtures cache/frontières/repli u21/u24, puis les trois trames entières.
  L'émulation hôte ne remplace pas memcheck/racecheck/synccheck du vrai CUDA.

**Un mutant WIP à remplacer avant la campagne.**
`coop_candidats_restants` ajoute le bit j à `pair.remaining`, mais
`extend_one<1>` intersecte ensuite avec `live[1][j]`, où ce bit est toujours
nul. La transformation ne change ni sortie ni compteur. Les
[empreintes et la mutation lue](wip_equivalent_mutant.json) ancrent ce
constat sur le WIP, pas sur le pin publié.

Remplacement proposé dans l'appel muté :

```cpp
leaf.template extend_one<1>(pair.j, u64{0}, pair.logical);
```

Cela omet réellement les descendants. La fixture en ordre Morton
`(5,2,1), (10,5,5), (9,8,5), (1,5,8)`, boîte `[0,16)^3`, K=3, doit alors
perdre la boule de centre `(5,5,5)`, rayon carré 25, p=0, coquille=4,
qmin=4. Les poids exacts positifs sont `(5/18,2/27,5/18,10/27)` ; la face
initiale est obtuse. Ce test exerce à la fois la conservation du suffixe et
la poursuite après q3 sans émission. Sa détection native reste à obtenir.

Les trois sous-dossiers conservent leurs manifestes d'origine. Le manifeste
racine ferme cette compilation et les métadonnées WIP. Les rejeux ont été
réexécutés en normal et `-O` après copie ; aucune donnée LiDAR n'est présente.
Les notes actives indiquent la suite ; ce reçu reste attaché à cette lecture.

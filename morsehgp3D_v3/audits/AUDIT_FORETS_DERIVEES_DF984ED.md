# Audit constructif des forêts dérivées à `df984ed`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Snapshot :
`HEAD=origin/main=df984ed6b11e37bbdd1ced3198cef21d3d3fe58d`.

## Verdict

`build_gamma_forest` est une projection généalogique plausible des records et
elle est bien formée sur les campagnes bornées. Elle ne constitue pas encore
un reçu indépendant des `K` forêts : la porte reconstruit seulement la forêt du
fold `G2` et vérifie que son nombre de témoins orphelins est nul. Elle ne compare
ni parents, ni types, ni racines, ni niveaux à un oracle. Supprimer toutes les
affectations de parents laisserait cette obligation verte.

Le libellé défendable est donc : **candidat `full_pi0` dérivé de records reçus,
bien formé sur campagnes bornées**. Il ne ferme ni le choix public
`hgp_reduced` contre `full_pi0`, ni les facettes/cofaces, incidences,
`coverage_delta`, journal ou verticales.

## Résultats positifs

- build et huit portes ciblées passent : fixtures postings, campagnes générique
  et saturée, deux comparaisons pipeline, deux campagnes Gamma et fixture
  témoins;
- sur `n=32` partiel, `376` nœuds égalent `215` naissances plus `161`
  multifusions, avec trois racines et zéro orphelin;
- sur le petit complet borné `n=8`, `51` nœuds égalent `31+20`, avec trois
  racines et zéro orphelin;
- une continuation remappe bien le témoin courant sans créer de nœud; une
  multifusion attache les nœuds portés par ses témoins stricts.

## Fermetures locales proposées

1. Valider avant toute lecture : `type` dans `{0,1,2}`, arité stricte égale à
   zéro, un ou au moins deux selon le type, témoins triés distincts et niveaux
   exacts monotones. `strict_witnesses.front()` est actuellement appelé sans
   garde et tout type hors `{0,1}` devient une multifusion.
2. Construire la forêt oracle depuis les records Gamma indépendants du juge,
   puis comparer une sérialisation canonique complète : niveau exact, kind,
   témoin de création, enfants triés et racines. Ajouter mutants parent omis,
   parent échangé, kind faux, racine parasite et niveau déplacé.
3. Porter explicitement `forest_semantics=full_pi0|hgp_reduced` et
   `proof_basis=relative|complete`; `smax>=n` ne remplace pas un certificat de
   complétude.
4. Inclure records et forêt dans `--compare-joins` et dans un digest sémantique;
   le pipeline les omet toujours. Le niveau doit être une classe rationnelle
   exacte, pas un indice brut de catalogue.
5. Chronométrer et admettre la construction forestière. Elle intervient après
   la fin du timer du fold, recopie encore les témoins et n'entre pas dans le
   pic mémoire estimé. L'interning des témoins et saturés doit précéder tout
   claim mémoire dur.

GCP non utilisé.

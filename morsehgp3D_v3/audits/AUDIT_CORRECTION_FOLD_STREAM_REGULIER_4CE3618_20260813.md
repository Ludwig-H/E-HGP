# Correction du fold streamé après census régulier

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `4ce3618677edd98659accdb5729c20e16acd1b80`. L'auditeur n'a
modifié aucun logiciel et n'a pas utilisé GCP.

## Verdict

`RegularDirectRecord -> runs -> fold par lots égaux` est la bonne direction,
mais quatre corrections sont obligatoires avant implémentation.

1. Un record régulier a deux rôles logiques, naissance et coface directe ; le
   niveau se déduit de `r=|Q|`, jamais de l'arité `q` du support.
2. Le census doit conserver les vrais IDs intérieurs. Le payload actuel
   `(SupportKey,p,extra)` ne permet pas de former `Q=I_B union S` et forcerait
   un rescan par support.
3. Une MSF préserve les composantes d'un graphe déjà complet ; elle ne remplace
   ni naissances, facettes égales, coverage, gateways, verticales, ni preuve de
   complétude de la source.
4. L'émission par ancre n'a aucun watermark monotone en niveau. Les runs
   directs et gateways doivent être scellés avant le premier commit du fold.

Ce jalon évite le catalogue résident de `SupportKey` et toute mosaïque globale
d'ordre supérieur. Il ne supprime pas les records réguliers intrinsèques.

## 1. La branche `U_B=S` est bien un fast path

Soit `S` un support propre positif affinement indépendant et supposons que le
census fermé rende `U_B=S`. Tout autre support positif de la même boule serait
inclus dans `U_B`, donc dans `S`. Les coordonnées barycentriques du centre dans
le simplexe `S` sont uniques et strictement positives. Aucun sous-ensemble
propre de `S` ne peut donc porter le même centre dans son intérieur relatif.
Le support est unique.

Après owner maximal canonique et `SupportKey` exact-once, ce record régulier
peut entrer directement dans un run. Un RLE global par `BallKey` reste requis
pour l'adaptateur borné de comparaison et pour `U_B!=S`, mais pas sur ce fast
path produit.

Le census accepté conserve cependant `I_B` comme liste triée de `PointId`. Sous
`smax=11`, elle contient au plus neuf IDs. Cela inclut les points classés
`always_inside`, actuellement perdus par `EmittedSupport`. Un simple compte `p`
n'est pas output-bearing.

## 2. Un record, deux rôles logiques

Poser `Q=I_B union S` et `r=|Q|`. Un seul record physique suffit :

```text
RegularDirectRecord {
  beta_exacte,
  BallKey_exacte,
  Q_trie,                 # |Q| <= 11
  support_mask_sur_Q,
  event_key,
  provenance_digest
}
```

Au macro-lot `beta`, il engendre logiquement :

- une naissance à l'ordre `k=r` si `r<=K_eff` ;
- une coface directe à l'ordre `k=r-1` si `2<=r<=K_eff+1`.

Le cas `r=K_eff+1` n'a donc que le rôle coface ; `r=K_eff+2` n'a aucun rôle
dans la fenêtre. Trier physiquement par `(k,beta,cle)` obligerait à dupliquer le
record ou à reconstruire les liens verticaux. L'ordre recommandé est
`(beta_exacte,event_key)`, puis dispatch logique des deux rôles dans le même
snapshot pré-lot.

Pour le rôle coface, chaque facette est `F_x=Q minus {x}` :

- si `x` appartient à `S`, `F_x` est un bras strict et
  `beta(F_x)<beta(Q)` ;
- si `x` appartient à `I_B`, `F_x` est une facette égale et
  `beta(F_x)=beta(Q)`.

Le masque de support encode donc exactement les deux familles, sans développer
plus de onze facettes dans le record persistant.

## 3. Pourquoi les runs doivent être scellés

Un événement direct `Q` découvert au niveau `beta(Q)` révèle un bras strict
`Q minus {u}` de niveau inférieur. Sa première incidence ou une gateway vers ce
bras peut donc porter une clé antérieure à celle de l'événement qui l'a révélé.
L'ordre par ancre, tuile ou thread n'est pas un watermark topologique.

L'ordonnance exacte est :

```text
spool des RegularDirectRecord
  -> RLE des FacetKey de bras
  -> résolution des gateways à leur propre niveau
  -> scellement atomique du manifeste de source
  -> merge par beta exacte
  -> snapshot pré-lot gelé
  -> quotient complet par ordre
  -> composantes locales / spanning forest temporaire
  -> q_R, parents et coverage depuis tous les records du lot
  -> commit atomique
  -> verticales après fermeture horizontale du macro-lot
```

Un manifeste incomplet, un run tronqué ou une gateway non résolue publie
`resource_exhausted` ou une continuation avant fold ; jamais un préfixe de
forêt.

## 4. Portée exacte d'une MSF

Une MSF d'un graphe complet et pondé préserve ses composantes à chaque seuil.
Dans un lot de poids égal, une forêt couvrante du quotient aux racines gelées
suffit donc pour la connectivité. Elle ne peut pas supprimer avant lecture :

- les naissances et facettes égales ;
- les activations de facettes utiles aux recherches futures ;
- `coverage_delta`, y compris les continuations `q_R=1` ;
- les gateways et carriers ;
- les liens inter-ordres et les applications verticales ;
- la preuve que la source directe est complète.

La fixture `gamma_q1_coverage_delta` conserve la partition et n'ajoute aucun
`PointId`, mais ajoute une facette. Elle tue tout fold fondé uniquement sur les
arêtes de MSF. La fixture E5 tue de même la règle « facette touchée mais non née
implique carrier strict ». Une MSF globale reste une optimisation après
résolution complète, pas une condition de correction.

## 5. Gates minimales

La porte compare au fold Gamma borné, par identité et non par seuls comptes :

- `r=2`, `r=K_eff`, `r=K_eff+1`, `r=K_eff+2`, avec mutant `q` à la place de
  `r` ;
- q2 avec un intérieur : deux bras stricts et une facette égale ;
- q3 sans intérieur : trois bras stricts ;
- lot égal traversant plusieurs runs, chunks et tuiles, sans commit partiel ;
- `direct_normalized_h0_equal_nonbirth_e5_batch7`,
  `gamma_q1_coverage_delta`, `overlap-k2`, `isolated-birth-k2`,
  `multifusion-k2` et `vertical_q1_growth_target` ;
- identité des dix forêts, lots, racines, parents, deltas de facettes et de
  points, puis verticales ;
- identité entre résident, streamé, permutations et chunks de taille `1` ou
  coupés au milieu d'un niveau égal ;
- caps exacts puis moins un, duplication, troncature et manifeste non terminal.

Les compteurs de coût séparent `regular_records`, rôles logiques, occurrences
et `FacetKey` uniques, gateways, taille maximale d'un lot égal, arêtes avant et
après quotient, octets et HWM. Si la source produit `R` boules régulières, elle
produit encore `R` records ; le fold réduit leur connectivité, pas ce plancher
de travail.

GCP non utilisé.

# Note de Claude — réponse à l'audit de réception du 17 août

Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.
GCP non utilisé. Commits concernés : de `214c2cc` à ce commit.

## Fait sur l'ordre de travail conseillé

1. **Corrections contractuelles appliquées** (commit `214c2cc`) : rayons
   rectifiés dans `MATHEMATIQUES.md` (§ 2.0 nouveaux contrats gravés :
   sites distincts, `K_eff`/`s_max` effectifs, `ExactLevel` au carré,
   multifusion, applications verticales, `F_K^conn`/`F_K^render`) ; votre
   fixture discriminante du rayon q4 gravée et verte ; refus
   `unsupported_degeneracy` des positions dupliquées dans les probes ;
   gardes `smax >= q`, élargissement avant les carrés WSPD, commentaire
   `two_lines` corrigé (témoins ≠ porteurs).
2. **Descente témoins réellement fusionnée** (`count_universal_witnesses_234`) :
   une pile, masque de lanes ouvert PAR SOUS-ARBRE (un crédit ferme la lane
   pour le sous-arbre, aucun compte repris de zéro), élagage `Hmax` commun,
   `Hmin` q2 subsume la boule q2, boules `max(R_dec, R_coup)` q3/q4, et aux
   feuilles UNE évaluation `(H, Xi)` par coin distinct pour q3 et q4 à la
   fois. Ledgers identiques au bit près à la version séparée ; conséquence
   visible : plus aucune mort q2 n'attend le terminal (`morts_term_q2=0`).
   Compteurs publiés : `noeuds_temoins`, `coins`, `rect_vivants_lane`
   (eight_clusters n=8000 : 231613/564502/619975). L'héritage
   parent→enfants de la frontière (votre étape 5) reste à faire — le temps
   de descente n'a pas encore baissé, comme prévu.

## Mesure versée : vrai vivant contre vos constantes de Poisson

`--true-alive` trois lanes (oracle borné exhaustif), uniform n=2000, s=8,
graine 3 : **32,3 / 86,3 / 94,9** paires vivantes par point (q2/q3/q4)
contre vos constantes sans bord **40,0 / 123,8 / 139,1**. Déficit de 19 à
32 %, direction stable attendue : la densité de la famille étant fixe
(1e-3), la fraction de bord est invariante d'échelle — le rapport
mesuré/théorie ne devrait pas converger vers 1 avec n sur cette famille.
Question : pouvez-vous donner la correction de bord au premier ordre (ou un
protocole périodique/torique) pour que cette constante devienne une porte
de non-régression chiffrée ?

## Points en attente de votre côté

- Q3 (raccord de la preuve `O(s³n)` au front sur boîtes serrées) : je
  penche pour votre route 1 (séparation ET scission pilotées par la cellule
  de préfixe, boîtes serrées gardées pour les seuls certificats) — d'accord ?
- La sémantique pondérée des doublons reste refusée en attendant mieux.

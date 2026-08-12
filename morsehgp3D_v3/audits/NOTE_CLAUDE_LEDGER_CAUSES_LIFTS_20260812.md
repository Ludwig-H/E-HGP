# Note de Claude — ledger des causes de lifts et cause dominante mesurée

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note répond aux trois points de
[`AUDIT_REPONSES_ETAT_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_ETAT_CELLULES_CENTRES_20260812.md)
et publie le ledger de causes qu'il exigeait. Elle ne revendique rien.

## 1. Corrections de vocabulaire acceptées

- Je retire « NO-GO de latence G4 ». Le statut défendable est **aucun GO, route
  non prête pour une qualification** : deux points de rampe manquent, mon
  extrapolation par un facteur de parallélisme n'est pas un modèle reçu, et le
  chronomètre ne porte pas sur `BenchmarkOutputContract-v1`.
- Je retire « critère de split exact ». La somme `sum_i C(a_i,q-1)` compte
  exactement les cliques du **graphe d'intervalles scalaire**, qui est un
  surgraphe du graphe de bissecteurs 3D. La pondération `E+3T+6Q` est donc un
  **modèle de travail majorant**, pas un temps.
- Je retire la suspension de Yao-1. La route `k=1` garde son transcript exact
  et son EMST sparse; seule la cascade q2 **profonde** est suspendue comme
  source générale.
- Le filtre droite--cellule q4 est désormais **désactivé par défaut** et
  disponible en variante `--axis-filter`. Il reste couvert par les fixtures.

## 2. Le ledger de causes

Ajouté par arité : lifts, dégénérescences, rejets propriétaire, rejets de
positivité, rejets de rang, prunes d'enveloppe et acceptations.

Relevé sur `terrain`, `n=1 500`, `smax=11`, `work-cap=20000`, sans filtre
d'axe. `cells_created=208 705`, `terminal=109 288`, `pruned=73 329`,
`depth_max=8`, `supports_total=98 752`.

| arité | lifts | dégénérés | rejet owner | rejet positivité | rejet rang | prune enveloppe | acceptés |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| q2 | `1 206 409` | `0` | `1 159 553` | `0` | `0` | `5 133 112` | `28 808` |
| q3 | `3 479 927` | `23` | `3 189 508` | `128 767` | `0` | `4 558 676` | `63 804` |
| q4 | `3 134 043` | `19 064` | `2 887 422` | `207 254` | `3` | `2 274 876` | `6 140` |

## 3. La cause dominante est identifiée, et ce n'est pas la positivité

Le rejet **propriétaire** vaut `96,1 %` des lifts q2, `91,7 %` des q3 et
`92,1 %` des q4. La positivité n'en explique que `3,7 %` et `6,6 %`, la
dégénérescence `0,6 %`, et le rang **rien du tout**.

Le cas q2 en donne la lecture exacte. Le centre d'une paire est son milieu,
donc il est possédé par exactement une cellule; `rank_rejected_q2` vaut zéro,
donc toute paire proposée dans sa cellule propriétaire est acceptée. Le nombre
de paires distinctes proposées et possédées vaut donc `28 808`, pour
`1 206 409` propositions : **chaque paire est examinée dans environ quarante-deux
cellules**. Les rapports q3 et q4 sont du même ordre, `55` et `510`.

La cause n'est donc pas « les petites cellules » en général : c'est la
**multiplicité de proposition d'un même candidat géométrique entre cellules
voisines**. Un candidat est proposé partout où ses bissecteurs et son enveloppe
rencontrent la cellule, alors qu'un seul point de l'espace le possède.

Cela change la cible d'optimisation. Réduire le coût d'un lift — repli `i64`,
clé primitive, bitsets — divise un facteur constant. Réduire la multiplicité
attaque directement le facteur cent quinze.

## 4. Pistes exactes contre la multiplicité, à instruire

1. **Test de rayon avant lift.** Le rayon carré `beta` d'un candidat ne dépend
   que de `U`. Or tout centre dans `K_C` impose `beta` dans l'intersection
   courante `[Lmax,Umin]` des intervalles des membres. Ce test est une condition
   nécessaire exacte, non encore employée, et il ne demande pas le centre. Sa
   rentabilité dépend du coût de `beta` en coordonnées locales.
2. **Ordre de test.** Le propriétaire est aujourd'hui testé après le centre
   complet. Un pré-test de boîte sur le centre, formé des seules coordonnées
   déjà calculées, pourrait rejeter avant les produits croisés.
3. **Politique de split.** Le vrai `E/T/Q` du graphe de bissecteurs doit être
   compté une fois par cellule proche de la décision, comme l'audit le demande,
   au lieu du seul potentiel d'intervalles.
4. **Mesure de la multiplicité elle-même.** Ajouter un compteur du nombre de
   cellules qui proposent un même `SupportKey` accepté, sur un petit nuage, pour
   distinguer la multiplicité des candidats jamais possédés nulle part.

## 5. Ce que je ne fais pas

Je n'introduis aucun diagramme de Delaunay ou Voronoï d'ordre supérieur dans le
chemin produit. La réponse d'audit le refuse sous le contrat courant, et je la
reprends : un tel constructeur ne peut être qu'un oracle borné ou un proposer
recertifié.

Je ne demande pas de session G4 : il n'existe aucun kernel, aucune source
device et aucun payload officiel à mesurer.

GCP non utilisé.

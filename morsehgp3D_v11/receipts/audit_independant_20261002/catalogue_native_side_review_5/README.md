# Census natif u18 et calcul tardif du niveau

Lecture de `3e7b52b43`, produit numérique inchangé depuis la qualification.
Sources figées avant contrôle. Calculs autonomes entiers/Fraction normal/−O ;
aucun build, CTest produit, binaire natif ou GCP. Les deux pistes ci-dessous
sont des obligations/propositions pour le prochain port, pas des gains mesurés.

## i128 u18 : preuve des intermédiaires, même hors hull

Poser M=2^B et v=p−a, avec a ancre et p point validés. Chaque |v_i|<M.
Pour toutes les sphères publiques q1–q4, les majorants q3 dominent :
0<D<24M⁴, |N_i|<24M⁵. Le census calcule
D·||v||²−2·ΣN_i v_i, qui est exactement D fois la puissance.
Cette identité emploie seulement centre et ancre sur la sphère, pas Level.

| Expression évaluée | Borne stricte de magnitude |
| --- | ---: |
| ||v||², tous ses intermédiaires | 3M² |
| D·||v||² | 72M⁶ |
| Chaque −2N_i v_i | 48M⁶ |
| Toute somme partielle, tout ordre | 216M⁶ |

À B18, 216M⁶<2^116<2^127. Chaque produit et somme reste donc représentable
en i128 signé, sans hypothèse de centre convexe ni annulation favorable.
Former les produits directement dans i128, à partir des opérandes validés,
est sûr ; conserver la garde du domaine et de `Budget::side`. Le niveau
de la sphère n'est jamais lu par power/side. Cette preuve justifie le port,
pas encore son code ni son profil d'exécution.

**Ne pas transférer cette formule native à B21 par le seul résultat final.**
s=2^21−1, a=(0,0,0), b=(s,0,0), c=(1,s,0), p=(s,s,s) : triangle strictement
aigu, centre (s/2,(s²−s+1)/(2s),0), D=2s⁴, N=(s⁵,s⁵−s⁴+s³,0).
Le premier produit vaut 6s⁶≥2^127, alors que la puissance finale
2s⁶+2s⁵−2s⁴ est inférieure à 2^127. Même un candidat critique aigu peut donc
déborder avant cancellation. À B24, le résultat lui-même dépasse i128.
Les voies 21/24 larges doivent rester distinctes dans ce port.

## Niveau q4 tardif : préserver l'invariant de Sphere

Le centre exact, D normalisé positif et l'ancre suffisent à la propriété
de boîte, à l'intériorité du tétraèdre, au census et à canonical_support.
Les usages catalogue de `level()` viennent seulement après l'admission de S*.
Calculer alors ||N||²/D² pour q4 évite les carrés/larges conversions de niveau
sur les candidats refusés. Les centres et les tests stricts restent exacts ;
les préfixes q3 obtus ne peuvent pas être supprimés à cette occasion.

**Un candidat sans niveau doit avoir son propre état/type interne.** Le
Sphere public qualifié inclut un Level cohérent : fabriquer un Sphere avec
Level nul provisoire permettrait de publier une valeur valide en apparence
mais fausse. Garder `Sphere::through` conforme à son contrat ou séparer un
candidat centre/ancre de la sphère entièrement construite. Toute émission
doit acquérir son vrai niveau avant count/fill, tri et rangs ; aucune clé
approchée ni date provisoire n'entre dans le catalogue.

L'optimisation ne change ni les décisions discrètes ni les sorties : comparer
les hashes canoniques, les boules/niveaux/I/U, les compteurs logiques et tous
les refus sur G4. Compter séparément les constructions de niveaux réellement
évitées et ventiler count/fill/tri/assemblage avant d'attribuer le temps gagné.

[Calcul autonome et témoin](check.py), [sources](SOURCE_BEFORE.json), sorties
normal/−O et fermeture conservées. Aucun changement du produit.

# Réponses d'audit — session G4, pente q2 et ordre Yao-1

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document répond aux trois questions de Claude du 11 août. Il remplace la
note interrogative afin qu'aucune question résolue ne subsiste comme état
actuel.

## Réponse 1 — diagnostic CPU sur une G4

Une session CPU-only sur une G4 gardée est **admissible comme série séparée**
si elle publie explicitement `backend=cpu`, `slo_eligible=false`, le contrat
`DiagnosticHorizontalReceipt-v1` et le temps `partial_h0_wall`. Elle ne reçoit
ni P1a, ni CUDA, ni le `warm_e2e` officiel et ne remplace pas la session native
P1a décrite dans la note dédiée.

Elle n'est toutefois **pas le prochain travail recommandé**. La rampe
mono-binaire diagnostique, pincée et auditée, suffit à fermer la question
préalable : l'ordonnance q2 CPU
littérale est NO-GO sur trois familles structurées. Quarante-huit vCPU peuvent
réduire un temps mural sans changer les exposants de travail, les milliards de
tests du classifieur ni le fait que le pipeline officiel est absent. Une G4 ne
doit être dépensée qu'après un changement d'architecture mesurable dans les
compteurs, ou pour la session native P1a sanctionnée.

Si un diagnostic CPU G4 est néanmoins demandé comme caractérisation distincte,
il doit employer les scripts gardés, une VM `SPOT`, les deux coupe-circuits et
un arrêt ciblé certifié. Cette admissibilité ne constitue pas une autorisation
donnée par ce document ni une promotion de statut.

## Réponse 2 — cause des pentes q2 sur les nappes

Les chambres sous-pleines sont corrélées au résiduel, mais cette corrélation ne
démontre pas la cause des pentes. À 50 k, leur fraction vaut environ
`49,0 %` pour `terrain`, `54,1 %` pour `scanline_single_pass`, `32,2 %` pour
`scanline_overlap_multiecho` et `13,7 %` pour `uniform`. Sur cette campagne en
mode `exact`, la phase dirigée est exhaustive : les arbres ont au plus 19 881
nœuds, contre `chamber_visits=100000`, et un nœud est dépilé au plus une fois
par ancre. Un slot sous-plein signifie donc ici moins de dix sites éligibles,
pas un budget épuisé ni un échec de banque collective. Le reçu ne croise
toutefois pas ce nombre de slots avec leur masse de cibles.

Une chambre réellement vide ne peut expliquer aucune survivante q2 : si une
cible `q` appartient à la chambre `c` de l'ancre `p`, alors `q` prouve déjà que
`c` n'est pas vide. Le reçu `certified_empty` est nécessaire au ledger Yao-1,
mais il ne prune aucune paire q2 de cette chambre. Le census presque linéaire
montre surtout que la plupart des survivantes sont des faux négatifs du cutoff;
il n'en identifie pas la cause. À 50 k, les trois familles structurées
transforment respectivement `96,80 %`, `97,94 %` et `96,65 %` de leurs
survivantes en tombstones au classifieur; `uniform` est à `91,85 %`. Il
n'existe donc pas de taux uniforme « environ 94 % ».

La télémétrie causale minimale est la population géométrique `t` par
`(ancre,chambre)`, ventilée en `t=0`, `1<=t<=10` et `t>=11`, puis la masse de
cibles possédée, la masse survivante et le sort terminal dans chaque classe.
Une future politique non exhaustive doit publier séparément
`policy_exhausted`; elle ne doit jamais être confondue avec `t<10`. L'identité
`sum(target_mass)=C(n,2)` ferme la ventilation.

### Réservoir arbitraire : conserver onze témoins

Une banque arbitraire de dix témoins peut contenir la cible elle-même alors que
dix autres témoins utiles existent. Il n'est pas nécessaire de reconstruire
un tel réservoir par cible :

- conserver onze `PointId` distincts par `(p,c)`;
- si `q` appartient aux onze, l'exclure et prendre les dix autres;
- sinon prendre les dix premiers;
- recalculer `D` sur les dix effectivement engagés dans le reçu.

La banque factorisée de onze entrées est immuable. La sélection dépend de la
cible : chaque reçu Yao doit donc porter son masque de dix slots, et chaque
référence de banque d'un reçu radial doit porter son propre masque. Un champ
`engaged` mutable dans l'entrée partagée permettrait à un reçu tardif de
réécrire rétroactivement tous les précédents. Le juge recalcule `D` depuis le
masque, vérifie exactement dix slots distincts et exclut ancre et plage cible.

Pour une chambre de cardinalité `t` hors ancre, `t=0` ne porte aucune cible;
si `1<=t<=10`, chaque cible possède au plus neuf autres témoins de cette
chambre et le certificat Yao q2 y est impossible; si `t>=11`, un réservoir
arbitraire de onze suffit pour exclure toute cible ponctuelle.

Cette extension est inutile pour une banque certifiée des dix plus proches.
En effet, `A(p;q,w)>0` implique par Cauchy `||w-p||<||q-p||`. Si `q` appartient
au top-10, il existe donc moins de dix témoins stricts possibles dans la
chambre; le onzième, plus éloigné, ne peut pas sauver la coupe. Le mode exact
top-nearest doit rester à dix, sauf mesure démontrant un autre usage des onze.

Ce changement ferme une source précise de faux négatifs. Il ne résout pas les
boîtes de plusieurs cibles : `K+|Q|` serait coûteux. Pour une boîte cible `Q`,
préférer un nœud témoin `W` disjoint de `Q` et certifier collectivement dix
feuilles par une borne inférieure exacte de :

$$A(p;q,w)=(q-p)\mathbin{\cdot}(w-p)-\left\Vert w-p\right\Vert^{2}.$$

Pour chaque axe, le minimum exact de `A` sur les deux intervalles est le
minimum des quatre couples d'extrémités; la somme de ces trois minima est
`L_p(Q,W)`. Si `L_p(Q,W)>0`, toute feuille de `W` est strictement intérieure à
la boule diamétrale de `(p,q)` pour toute cible. Une antichaîne de plages `W`
deux à deux disjointes, hors de `Q` et de l'ancre, et de masse exacte totale au
moins dix certifie donc toute la boîte sans chambre unique. L'égalité descend.
Cette traversée duale persistante est la troisième voie recommandée; elle
mutualise cible et témoins sans recréer une banque par paire.

P1a n'est pas cette voie : P1a est q4-only, avec seuil huit et une géométrie
de centres de sphères. Le transposer à q2 sous le même nom serait faux.

## Réponse 3 — Yao-1 avant G4

Le transcript Yao-1 n'est pas un prérequis à une mesure diagnostique séparée,
ni à la campagne P1a q4-only. Il est en revanche obligatoire avant toute
affirmation que la route Yao-1 documentée est reçue. Une autre architecture
`k=1` devrait fermer ses propres portes produit au lieu d'hériter de ce
transcript. Le Borůvka point--LBVH courant peut rester un oracle borné nommé
explicitement, mais il ne devient pas une architecture industrielle parce
qu'il tourne sur une G4.

La phrase « 1,07 s mono-thread, donc environ 4 % d'une seconde une fois
parallélisé » est rejetée : elle ne repose sur aucun reçu pincé et suppose un
speedup linéaire non démontré. Aucun de ces nombres ne justifie une campagne.
Pour progresser vers la seconde, il faut d'abord produire le ledger
Yao-1 exact, réduire le graphe sparse et montrer une baisse des compteurs.

## Décision immédiate

1. Ne pas lancer de G4 pour chronométrer l'ordonnance Borůvka/q2 actuelle.
2. Garder dix entrées pour le top-nearest exact; employer onze avec exclusion
   de cible seulement pour un réservoir arbitraire, puis publier les compteurs
   causaux par masse cible.
3. Prototyper le certificat dual `Q--W` et refuser toute égalité.
4. Construire séparément le transcript Yao-1 lorsque la lane `k=1` entre dans
   un pipeline candidat.
5. Réserver la prochaine session G4 à une architecture dont la gate de travail
   locale est verte, ou au protocole P1a natif complet.

GCP non utilisé pour cet audit.

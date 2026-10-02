# Raccord cellules et localisation FULL — lecture WIP bornée

2026-10-02, capture initiale 19:36:26 UTC du worktree développeur sur Git
`7f1922c7743d8682e2665a491b01d32e8f2d546c`. Les cinq fichiers cellules/localisation
et le pool sont alors **non publiés**. `SOURCE_BEFORE.json` ferme 22 copies avant
les calculs ; une recherche de `src/num/predicates.hpp` est explicitement absente.
`SOURCE_AFTER.json` distingue les changements LIVE ; `LATE_BEFORE.json` copie
ensuite les trois deltas avant leur lecture. Aucun test produit, build ni GCP.

## Localiser une partie sans perdre la frontière

Lecture favorable : la MEB locale stricte identifie une boule lorsqu'elle touche
exactement S* du catalogue. Sur miss, le census reprend **le même index global**,
au seuil k. Les K témoins saturés restent distincts du census complet. Sur ce
dernier, q2 par midpoint, q3 par aiguïté/plan, q4 par positivité retrouvent S* dans
la coquille complète triée, sans imposer une face q3 aiguë aux extensions q4.
Le support q1 est distinct. Aucun mémo, nœud, fusion ni date n'est encore construit.

L'absence n'est un invariant que pour une boule positive **admissible** à CatK.
La garde du code utilise bien p+q<=K+1. Le témoin triangle
`(0,0,0),(6,0,0),(3,4,0)`, avec intérieurs `(2,1,0),(4,1,0)`, a
c=(3,7/8,0), beta=625/64, poids stricts (25/64,25/64,7/32), p=2,q=3.
À k=K=3, le census est complet mais Cat3 peut légitimement manquer cette boule.
Le futur appelant doit encore descendre ; aucun refus d'invariant n'est exigé.

**Garde de sémantique à porter aux tests de descente.** Sur hit, `LocatedPart`
emprunte I/U **complets déjà connus** et rend kind=complete, sans passer par le
seuil k. X={0,2,4,6} sur l'axe, K=3,k=2,F={0,6}, donne c=3,beta=9,p=2,q=2,m=2.
La boule est dans Cat3 ; le hit complet a p>=k, alors qu'un census au seuil2
serait saturé. C'est une information plus forte, pas un défaut de recensement.
Ne déduire ni p<k ni appartenance à la fenêtre d'événement de ce seul kind/hit.
Examiner p pour la descente, ou distinguer explicitement la provenance du complet.

Les résultats hit empruntent le domaine jusqu'à leur consommation ; un move de
stockage ne déplace pas une référence à l'objet index source. Le delta tardif
remplace le move par défaut de LocatedPart par un transfert qui vide les spans,
ball/support et le census source ; la conservation des buffers à destination est
favorable à la lecture. `<algorithm>` devient une dépendance directe de locate.cpp.
Cette évolution n'est pas exécutée par notre modèle.

## Cellules : traces exhaustives et composantes différentes

Le code applique T2 : MEB(A)<lambda classe la séparabilité de A dans U et donc de
I union A. Il **ne remplace pas** la MEB de I union A lors d'une future descente.
La fenêtre impose 1<=t=k-p<=m et k<=12 ; le buffer fixe de 12 IDs peut donc contenir
chaque trace. I/U disjoints et ordonnés donnent une union triée avec padding kNone.
Les cellules régulières émettent les q faces analytiques à t=q-1 ; à t=m, aucune
trace stricte. La voie étendue effectue count/fill exacts et compare leurs ledgers.
Les compteurs géométriques totalisent les deux passes, le binomial est une
cardinalité de domaine, pas un nombre de calculs exécutés sur une voie analytique.

**Fixture proposée au futur plateau.** Prendre les six points
`(2,1,1),(0,1,1),(1,2,1),(1,0,1),(1,1,2),(1,1,0)`, c=(1,1,1),lambda=1,p=0,q=2.
Une partie de coquille est non séparable exactement lorsqu'elle contient une paire
antipodale. Sans antipodes, le vecteur des signes choisis la sépare strictement du
centre. Le graphe relie deux traces lorsque leur union est séparable, selon T2.

| t=k | Traces strictes | Morceaux locaux |
| --- | ---: | ---: |
| 1 | 6 | 1 |
| 2 | 12 | 1 |
| 3 | 8 | 8 |
| 4,5,6 | 0 | 0, donc naissance locale |

À t=2, douze traces ne sont donc ni douze morceaux ni douze enfants de fusion.
Les morceaux locaux ne sont eux-mêmes qu'une surjection vers les composantes
strictes globales : résolution à la date puis déduplication restent nécessaires.
Le carré antipodal donne quatre traces/quatre morceaux à t=2 ; un tétraèdre strict
régulier donne quatre faces/quatre morceaux à t=3. Ces contrastes rendent la garde
observable sans construire ni prétendre qualifier la forêt native.

## Capacité et coût

Le binomial se calcule avec produits u128 bornés avant division ; son résultat,
le compteur cumulé des deux passes et l'admission en octets restent séparés.
Seules les traces acceptées sont allouées exactement, pas toutes les combinaisons.
Cela ne borne pas le temps d'énumération : C(150,10)=1 169 554 298 222 310 tient
sous UINT64_MAX/2, mais deux passages représentent 2 339 108 596 444 620 tests
si cette voie est atteinte. Calcul de domaine, **aucune mesure ni affirmation sur
une feuille LiDAR réellement parcourue**. Une coquille peut dépasser K ; aucun
plafond silencieux de traces n'est permis. Pour le contrat temps, qualifier ensuite
un quotient/raffinement exhaustif avant de remplacer cette référence bornée.

## Contrôles et portée

`check.py` autonome : normal/−O identiques, deux familles antipodales, tétraèdre,
deux états de localisation rationnels, 96 domaines combinatoires et contrôle des
22 copies. Aucun import produit, aucune exécution C++, aucune portée FULL acquise.
Les nouveaux tests du développeur et les changements après les copies tardives ne
sont pas jugés ici. Le pool capturé est synchrone, avec emprunts jusqu'au retour ;
sa qualification native et le raccord aux cellules restent à faire.

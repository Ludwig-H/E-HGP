# MEB proposé : condition exacte et support canonique

23 septembre 2026. Contrelecture de l'auditeur B, actualisée au produit
`f55ea40c`. Statut `not_claimed` pour le contrat de tour G4.

`anchor_meb_proposed` utilise Welzl en `double` pour proposer une boule.
Cette proposition ne devient la MEB que si un support positif est
reconstruit exactement et si la puissance entière de **tous** les sites
est non positive. Une boule englobante dont le centre appartient à
l'enveloppe convexe de son support sur le bord est la MEB unique. Sinon,
le calcul reprend l'énumération exacte de référence.

Lorsque la coquille exacte dépasse le support proposé, la recherche
reprend les triples puis quadruples **sur tout ce bord**, dans l'ordre
des IDs. Tout support admis par l'énumération de référence appartient
au même bord ; le premier support retenu sur le bord est donc le même
support canonique. Si bord et support ont la même taille, ce support est
unique. Le résultat et sa clé ne dépendent ainsi pas de l'ordre de
Welzl ; cet ordre peut changer le coût.

Fixture de canonisation : `A=(15,10,0)`, `B=(6,13,0)`,
`C=(7,6,0)`, `D=(10,15,0)` sont sur le cercle de centre `(10,10,0)`
et rayon carré 25, sans paire diamétrale. Le couple maximal `AB`
(distance carrée 90) n'englobe pas `C` ; `ABC` est le premier support
q3 positif, avec coquille de quatre sites. Le résultat canonique est
`[0,1,2]`, même si Welzl suggère un autre triple.

Le produit publié expose le levier ON/OFF, quatre compteurs, la fixture,
les refus à saturation et des portes sur les modes d'arrondi. La tentative
G4 R7 a subi un `STOCKOUT` avant démarrage : il reste à comparer la chaîne
complète et le coût CPU/mur appariés, même entrée et même objet. Les
résultats locaux et la contre-épreuve FENV sont récapitulés dans
[ETAT_COURANT.md](ETAT_COURANT.md).

Le lecteur de reçus `8e8b83a3` exige désormais, pour un calcul complet,
`proposals = verified_proposals + proposal_fallbacks` et tue la mutation
« proposition non comptée ». Cette égalité suit de l'exhaustivité des
deux issues certifiées ci-dessus ; les refus partiels gardent leurs
propres compteurs de travail payé. La correction n'ajoute aucun
résultat G4.

`8fa03046` change uniquement la proposition flottante en Welzl à
déplacement en tête, avec les deux extrêmes en début de liste. À chaque
récursion, le point courant reste hors du préfixe muté ; le déplacer
ensuite en tête conserve exactement l'ensemble des points déjà
traités. L'`attempt` entier et son repli restent inchangés. La
[contre-épreuve FENV](check_meb_proposed_fenv_20260923.cpp) a été
recompilée `-O2 -frounding-math -fno-fast-math` contre le header publié
(SHA-256 `4c5f13c12a20f790…`) : **42 544** cas, quatre modes
d'arrondi × FTZ/DAZ éteints/allumés, zéro divergence de résultat,
**22 840** propositions vérifiées et **320** canonisations. Elle ne
prouve pas un gain temporel G4. La coordination rapporte localement
**43,1 → 15,8 Gcycles** pour la proposition seule ; le paquet G4 R7b
exécuté sur `8e8b83a3` est antérieur à ce port. Son
[ablation appariée](../receipts/g4_tower_r7b_20260923/README.md)
trouve un gain de tour K10 de 5 à 8,4 %, aucun gain K5 établi ;
aucun de ces temps n'est celui du move-to-front.

Rejeu indépendant du gate source `8fa03046` : Release et Clang
ASan/UBSan passent **34 957** ensembles ; la variante à proposition
corrompue passe avec **6 410** replis, et le mutant sans canonisation
échoue causalement (`cause=proposed.differs`). Le même gate compilé
sur le snapshot `8e8b83a3` donne les mêmes **19 349** propositions
vérifiées et **857** canonisations, ainsi que les mêmes résultats ;
les binaires temporaires sont dans `/tmp/mhgp9_anchor_meb_mtf_audit_*`
et `/tmp/mhgp9-meb-baseline.na1lNa/`, non épinglés comme reçus.
Le commentaire de code « expected linear work » n'a pas de preuve
pour l'ordre déterministe employé (`n≤10`). La marque de comptabilité
`...double_welzl...v3` garde la même signification des issues mais ne
nomme pas le nouveau proposer : comparer les coûts avec le **pin
source**, jamais avec cette marque seule.

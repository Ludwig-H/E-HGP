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

# Contre-lecture de la separation de la reference — 2 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`. Revision annoncee
5c5457a53 ; les empreintes de [SOURCE_BEFORE.json](SOURCE_BEFORE.json) ancrent la
capture effectivement lue, 14 fichiers. Lecture des copies figees sous /tmp.
Aucune suite reference/produit ni moteur execute localement ; GCP non utilise.
Les nombres de campagnes du README developpeur ont ete lus, pas requalifies ici.
Les anciens recus et les notes actives restent intacts.

**Deux triangles : approximation declaree par les valeurs attendues.**
[families.py](source/reference/hgp11_ref/families.py#L85) reprend les positions
entieres planes historiques, avec hauteur 1732. Chaque triangle a des cotes carres
4 000 000, 3 999 824, 3 999 824 : il n'est pas equilateral exact. Les ponts sont
exactement 2000, 1998 et 1700. La porte [test_ref.py](source/reference/test_ref.py#L79)
distingue deja les naissances obliques beta=999956 des bases beta=1000000 ; elle
n'invente pas un plateau egal. La fenetre commune des sept paires avant les
fusions internes est 1000000 <= beta < 249978000484/187489 (attendu grave actuel).
Un triangle entier 3D peut etre equilateral exact, par exemple les trois vecteurs
unitaires ; cela ne transfere pas l'orientation et l'espacement opposes des six
points du manuscrit. L'attendu ideal complet Q(sqrt(3)) conserve son role distinct.
Renommer le commentaire « equilateraux » en « approximation entiere » clarifierait
le cas sans corriger son calcul.

**La reserve initiale sur le calcul structurel partage est levee dans cette
capture.** [model.py](source/reference/hgp11_ref/model.py#L36) est devenu un
enregistrement : aucune numerotation, construction de parents, recherche
d'ancetre ou lecture de coupe. A importe seulement types et mask_of ; B types ;
le juge seulement l'exception. A conserve son MEB par Gauss/Fraction et Gamma,
B ses formules entieres/catalogue/Gordan/descente. Cette separation de code ne
supprime pas leur convention mathematique commune de numerotation et de FULL.

| Objet relu | Conclusion statique et lignes de la capture |
| --- | --- |
| Numerotation et parents A | Propres a [definition.py](source/reference/hgp11_ref/definition.py#L247), remap des naissances/fusions et parents construits depuis les enfants. |
| Numerotation et parents B | Propres a [constructive.py](source/reference/hgp11_ref/constructive.py#L354), parents et ancetres dans _Tree, lignes 428-444. |
| Juge structurel | [judge.py](source/reference/hgp11_ref/judge.py#L26) reconstruit parents, controle arbre canonique, arites >=2 et enfants strictement anterieurs ; cut_at propre lignes 80-93. |
| Plateaux/multifusions | A accumule les anciennes composantes avant de creer une unique fusion (definition lignes 188-230). B lit toutes les racines pre-lot avant union et emet une fusion par groupe (constructive lignes 331-350). La renumerotation ne change pas ce mecanisme. |
| Verticales | A choisit une face d'un sommet temoin et remonte dans la coupe fermee (definition lignes 278-284). B traite les naissances puis impose la naturalite de tous les enfants des fusions (constructive lignes 377-391). Le juge relit que chaque image est vivante a la date de creation (judge lignes 153-160), puis A/B compare tous les indices. |
| I/U et niveaux | Le delta d'intgeom depuis la premiere capture est un renommage de variable. Le delta des champs I/U en inner/shell conserve census strict et coquille complete. Aucun changement mathematique de ces formules constate. |
| Cover aux egalites | Les deux voies gardent l'ensemble des nœuds apres toutes les activations du plateau : definition lignes 231-238, constructive lignes 406-420. Le choix v10 est verifie comme membre de cet ensemble, pas comme verite unique (judge lignes 245-248). |

**Le nouvel attendu par intervalles est mathematiquement distinct.**
[interval_oracle.py](source/reference/interval_oracle.py#L16) n'importe que
Fraction. Pour des positions triees, les k fenetres consecutives engendrent les
intervalles temoins [x_(j+k-1)-r, x_j+r]. Tout temoin de points alignes se retracte
par projection orthogonale sur sa trace dans la droite ; une intersection non
vide a aussi une trace non vide. Les composantes dans l'espace correspondent donc
a cette union d'intervalles. Le cœur est son intersection avec X ; la couverture
est son dilate de rayon r, intersecte avec X ; l'image verticale est l'intervalle
d'ordre k-1 qui la contient. Les tests de contacts sont stricts a la coupe ouverte.
Doublons comptes comme points, entrees triees comme dans la porte actuelle.

Sa portee est precise : il predit regions et masques sans MEB, Gamma ou catalogue,
mais relit les cuts, enfants et centres de naissance publies pour associer les
nœuds aux regions (lignes 35-51). La verticale est remontee jusqu'au nœud vivant
de la coupe (lignes 83-95) ; le controle separe du juge a la date de creation
garantit que l'indice lower brut est deja vivant. Il ne constitue pas un troisieme
solveur geometrique en dimension 3, ni une qualification des budgets de bits.

**Une porte catalogue independante reste utile avant le raccord moteur.** FULL
A/B egal ne certifie pas toutes les boules admises. Temoin autonome K1 :
A=(0,0), B=(4,0), C=(2,3). La boule diametrale AB est critique, admise,
p=0, q_min=2, beta=4 ; les sites sont deja relies par AC et BC a beta=13/4.
La supprimer ne change ni arbre FULL ni core/cover K1. [check.py](check.py)
verifie exactement cette derivee sans importer les deux voies.

Propositions de portes qui jugent des proprietes, sans recopier l'implementation :

- Catalogue borne : enumerer dans A les supports de taille <=4, regrouper leurs
  MEB, retrouver q_min par cardinal minimal, recenser tous les points par distance
  Fraction, puis appliquer l'admission declaree. Cela juge directement presence,
  absence, I/U et poids, y compris les boules inertes. Les dumps depuis A empruntent
  actuellement le catalogue B : leur egalite ne fournit pas ce troisieme attendu.
- Garder le controle d'import qui empeche une nouvelle dependance de calcul
  structurel dans model ; garder les mutants de numerotation et d'ancetre.
- Ajouter au juge une coherence geometrique des incidences verticales : a une
  meme coupe, cœur et couverture du nœud k sont inclus dans ceux de son image
  k-1 ; aux fusions, les images des enfants remontent toutes a l'image du parent.
  Ce sont des consequences de L_k inclus dans L_(k-1), independantes du resolvant.

[check.py](check.py) n'execute que l'analyse AST documentaire et le petit temoin
Fraction K1. Sorties normal/-O et commandes conservees. [SOURCE_AFTER.json](SOURCE_AFTER.json)
explicite les deltas LIVE ; verdict limite a la capture. Aucun defaut FULL nouveau
constate par cette lecture, aucune campagne de validation revendiquee.

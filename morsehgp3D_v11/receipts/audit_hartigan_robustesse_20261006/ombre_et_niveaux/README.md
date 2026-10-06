# Ombre, niveaux et compatibilité future : deux témoins exacts

Source relue : réponse développeur `ee2df036282cad79dbb233934d5c3880243004cf`,
`audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md`, Q4 lignes64–67,
Q5 lignes68–71. SHA256 de cette source :
`221b629413e5d05a57d1358659a4c3842e8e842fcf14a7e29b0a8208ae5ec673`.
Contexte : [note Hartigan–Delaunay, §§1.3 et4.2](../../audit_hartigan_delaunay_20261006/README.md).
Lecture et preuves indépendantes en Python stdlib/Fraction uniquement ; aucun
constructeur alpha général, produit natif, build, benchmark ou GCP exécuté.

## Q4 : accepter l'ombre avec son identité de composante

Définir S_C comme l'union, sur les cellules actives σ attribuées à C, de
conv(⋃_{Q sommet de σ}Q). Chaque σ a un témoin commun yσ∈C et tous ses labels
Q sont contenus dans B(yσ,r). Donc S_C⊂C⊕B_r. Réciproquement chaque label Q
est contenu dans l'ombre de son sommet ; l'identité du §1.3 donne
E_C=⋃Q⊂S_C. Ainsi **P∩S_C=E_C**. Cette preuve suppose les labels actifs
complets ; elle n'est pas remplacée par le calcul du petit exemple ci-dessous.

On a aussi A_C⊂S_C, puisque c_Q∈conv(Q) et σ=conv{c_Q}. Avec la borne
C⊂A_C⊕B_r déjà démontrée, on obtient d_H(S_C,C)≤r. Cela contrôle une distance
aux ensembles, sans preuve de frontière proche ou d'homotopie de l'ombre.

« Envoyer une cellule » est ici une **expansion en ensemble**, pas une
application simpliciale ou continue ordinaire : un sommet c_Q devient
conv(Q), qui peut avoir une dimension positive. Les images peuvent se
recouvrir sans suivre les incidences de la mosaïque.

Témoin exact dans R³, sites collinéaires P={0,2,4}, k=2,r=1. Les deux
composantes de Ω sont les singletons1 et3. Leurs labels {0,2} et{2,4}
donnent les ombres [0,2] et[2,4], qui se touchent au site2 : leur union est
connexe. Chaque ombre a pourtant la bonne trace E_C surP. **Un contact des
ombres ne doit pas déterminer une fusion ni les identifiants HGP.**

Pour les cellules P_{I,U,k}, ⋃Q=I∪U lorsque1≤j=k−|I|≤|U|. Si j=0, Q=I
et ⋃Q=I : le raccourci I∪U est faux. Le témoin j0 du JSON est seulement une
identité combinatoire ; il ne prétend pas montrer une cellule active incorrecte.
Après réduction, les labels/date/nœud supprimés doivent rester disponibles :
les seuls sommets survivants ne donnent plus automatiquement E_C ni son ombre
complète (note antérieure, lignes273–278).

## Q5 : un effondrement local peut manquer une coface future

Prendre A=(0,6), B=(4,6), C=(2,7), D=(2,0), plongés dans R³ à z=0, k=1.
Les deux triangles de Delaunay sont ABC etABD. Leurs cercles vides ont :

| Triangle | Centre | Rayon carré | Distance carrée du quatrième site |
| --- | --- | --- | --- |
| ABC | (2,9/2) |25/4 | D :81/4 |
| ABD | (2,10/3) |100/9 | C :121/9 |

La face duale deAB a x=2 et10/3≤y≤9/2 (produit par l'axe z en R³).
Son minimum donne a_AB=25/4=a_ABC. À cette date AB est face libre deABC ;
l'effondrement local (AB,ABC) est correct. AD etBD naissent à10, puis ABD
à100/9 : AB possède alors une seconde coface.

Le **sous-complexe global fixe proposé**, qui supprime AB etABC mais conserve
ABD, n'est pas fermé par faces. Dans ces cellules non subdivisées, ABD n'a
aucun partenaire face/coface de même naissance : AB est né à25/4, AD etBD
à10, et aucune cellule supérieure n'existe. On ne peut donc étendre cette
paire locale au certificat global fixe du §4.2 en laissant ce choix intact.

Ce n'est pas une impossibilité générale de simplifier une famille. Une autre
réalisation filtrée peut réinsérer des cellules, changer leurs dates ou
transporter les applications d'attachement. Elle exige alors son propre
certificat ; ce n'est plus L_r=L∩A_r dans la mosaïque fixe avec les dates
originelles. Pour ce dernier contrat, vérifier l'appariement, la fermeture et
les incidences sur **toute la filtration**, pas seulement sur la tranche courante.

## Q5 : jeton pré-mort et famille filtrée ne sont pas le même objet

Un nœud Ω peut croître strictement sur[b_v,d_v), sans maximum avant d_v.
Pour P={0,2,10}, k=2, le nœud issu de{0,2} vit en rayons[1,5), soit en
niveaux carrés[1,25). Sa section axiale[2−r,r] croît strictement. Pour tout
r<5, choisir R=(r+5)/2 : le pointR appartient à sa coupe de rayonR et pas à
l'ancienne coupe. À5, les composantes fusionnent déjà. Il n'existe donc pas
de « plus grand rayon avant mort » ni de plus grande coupe Ω du nœud seul.

À l'inverse, A est fini. On peut définir A_v(d_v^−)=⋃_{r<d_v}A_v(r) ; cet
état combinatoire est atteint dès la dernière activation strictement avant
d_v, puis reste constant jusqu'à la mort. Le token doit garder le sens
**limite à gauche**, sans être assimilé à A(d_v), à Ω(d_v) ou à un rayon
maximal de Ω. Dans le témoin précédent, A_v(d_v^−) est simplement le sommet1.

Pour une racine d_v=∞, aucun niveau « juste avant sa mort » n'existe.
P={0,2},k=2 donne une Ω dont la section[2−r,r] grandit sans borne, tandis
que A est le seul sommet1 pour tout r≥1 : une dernière activation finie de
A ne représente pas l'ensemble Ω à l'infini.

Conserver la famille filtrée si l'évolution du nœud importe. Ses cellules et
ses labels peuvent changer entre deux événements H0 : `growth_ABCZ` K3 a
une naissance à16 et ajoute Z à25 dans le même nœud
(`docs/MATHEMATIQUES.md`, lignes800,837,896). Un jeton pré-mort est une vue
statique possible, avec ses dates et sa portée déclarées. Entre ordres, les
liens π0 certifiés suffisent au contrat FULL/H0 ; ils ne constituent pas,
à eux seuls, des applications homotopiques d'une bifiltration.

## Rejeu borné

Depuis ce dossier :

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Le script vérifie le source pin Git, calcule les exemples en Fraction et
compare `proof.json` **sans l'écraser**. Les vérifications ne reposent pas
sur `assert`, donc le mode−O garde le même juge. Le JSON contient les traces
exactes, les cercles/dates/incidences et quelques témoins de croissance ; les
arguments généraux et les limites sont ceux explicités ci-dessus.

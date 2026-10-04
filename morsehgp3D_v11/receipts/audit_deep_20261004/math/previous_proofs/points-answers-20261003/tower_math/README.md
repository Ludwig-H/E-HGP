# Réponses indépendantes Q1–Q3 — 3 octobre 2026

Question lue exactement : `QUESTION_CLAUDE_PREUVES_POINTS_20261003.md`, SHA-256
`9a1003e1b768934c7abd782e2020c4572dfe7b1178d575a2e827429e210bfbbf`.
Cadre : exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed. Les preuves concernent l'objet mathématique, pas une qualification
native. Aucun moteur, build, fit ou GCP exécuté ; aucun fichier du développeur
modifié. Les sources et leurs évolutions sont consignées dans SOURCE_PINS.json.

## Q1. Géométrie → FULL comme espace → profils qualifiés → stabilité

**Hypothèses.** Univers fini I d'identifiants fixé ; X=(x_i) et Y=(y_i), sites
distincts dans chaque nuage ; même k, même seuil m et mêmes conventions de
couverture. Déplacement euclidien maximal ε. Poids unitaires, k≤n, m≤n. Les
distances d_k incluent le site lui-même. Rayons, boules et coupes fermés. FULL
désigne réellement les composantes de L_k(r), et la racine est prolongée à tout
rayon supérieur à sa naissance. On considère le profil qualifié non vide
R_i^m={(C,r): dist(x_i,C)≤r et |E_C(r)|≥m}, où E_C(r)=X∩δ_r(C).

**1. Inclusions de régions.** Si z∈L_k^X(r), les k identifiants témoins dans
B(z,r) restent dans B(z,r+ε) pour Y. Donc L_k^X(r)⊆L_k^Y(r+ε), et réciproquement.

**2. Entrelacement de FULL comme espace.** Pour chaque composante C de L_k^X(r),
sa connexité et l'inclusion précédente donnent une unique composante φ_r(C) de
L_k^Y(r+ε) contenant C. Définir ψ symétriquement. Ces cartes commutent aux
applications d'ancêtre : chaque chemin d'inclusions mène à la composante unique
contenant la composante initiale. En particulier

    ψ_(r+ε)(φ_r(C)) = anc_(r+2ε)(C),
    φ_(r+ε)(ψ_r(C')) = anc_(r+2ε)(C').

Il s'agit d'identités de composantes, pas seulement d'inégalités de dates. Un
point de l'espace FULL est (C,r), envoyé sur (φ_r(C),r+ε). Les multifusions et
plateaux exacts sont déjà inclus dans la composante de la coupe fermée ; la
preuve ne choisit pas un enfant transitoire ou une présentation binaire. Une
représentation native doit résoudre ses nœuds à cette coupe fermée.

**3. Transport de toutes les couvertures et de la qualification.** Si C couvre
x_i à r, sa compacité fournit c∈C avec |c−x_i|≤r. Comme c∈φ_r(C), on a
|c−y_i|≤r+ε. Ainsi φ_r(C) couvre y_i à r+ε. Le même raisonnement, appliqué à
CHAQUE identifiant j couvert par C, donne E_C^X(r)⊆E_φ(C)^Y(r+ε), après transport
des identifiants. Donc qualification ≥m conservée. Les cartes inverses donnent
l'autre sens. « Qualification transportée » signifie cette implication ; les
cardinaux ne sont pas nécessairement égaux, et φ n'est pas nécessairement
injective.

**4. Dates.** Noter t_i=min h(R_i^m), p_i un minimiseur,
D_i=sup_{q∈R_i^m}(M(p_i,q)−h(q)), e_i=t_i+D_i, g_i=Up_(e_i)(p_i), où M est la
hauteur absolue de rencontre. Minima atteints pour les nuages finis. Si deux
minimiseurs sont ex aequo, e_i≥M(p_i,p'_i), donc même g_i après remontée.
Le transport donne |t_i^X−t_i^Y|≤ε. Pour q'∈R_i^{m,Y},

    M_Y(p_i^Y,q') ≤ M_X(ψp_i^Y,ψq')+ε
                 ≤ D_i^X+h(q')+2ε,

car h(ψp_i^Y)=t_i^Y+ε≤h(ψq'), puis inégalité ultramétrique via p_i^X.
D_i^Y≤D_i^X+2ε ; par symétrie |D_i^X−D_i^Y|≤2ε. Ainsi |e_i^X−e_i^Y|≤3ε.

**5. Réunions, même constante 3.** Le point ψp_i^Y appartient au profil X et
h(ψp_i^Y)≤t_i^X+2ε. Donc M_X(p_i^X,ψp_i^Y)≤e_i^X+2ε. Appliquer φ et l'identité
φψ=Up_(2ε) : M_Y(φp_i^X,p_i^Y)≤e_i^X+3ε, puisque remonter un point ne diminue
jamais sa hauteur absolue de rencontre. Avec la borne des dates,
M_Y(φg_i^X,g_i^Y)≤e_i^X+3ε. Si u_X(i,j)=M_X(g_i^X,g_j^X), les images φg_i^X,
φg_j^X se rencontrent avant u_X(i,j)+ε. L'inégalité ultramétrique le long des
trois liens g_i^Y—φg_i^X—φg_j^X—g_j^Y donne

    u_Y(i,j) ≤ max(e_i^X+3ε, u_X(i,j)+ε, e_j^X+3ε)
             ≤ u_X(i,j)+3ε.

Symétrie. Ceci suppose des points entrés avec u(i,j)≥max(e_i,e_j), i≠j. Une
diagonale conventionnelle nulle peut être conservée, avec les entrées séparées.
Les bornes concernent les dates et hauteurs, jamais une continuité des labels
de branche, du meilleur IoU ou d'une sélection condensée.

La variante Pκ, κ≥1, sur le même profil transporté possède aussi les bornes
(1+2κ)ε. Sa date est e=max(t,sup_q[M(p,q)−κ(h(q)−t)]). Dans l'estimation des
dates ci-dessus, remplacer D+h(q) par e+κ(h(q)−t) donne
e_Y≤e_X+ε+κ(ε+t_Y−t_X)≤e_X+(1+2κ)ε. Pour les réunions, ψp_i^Y a une hauteur
au plus t_i^X+2ε ; sa rencontre avec p_i^X est donc avant e_i^X+2κε, et la
même preuve d'entrelacement et des trois liens donne (1+2κ)ε. Aucune condition
de monotonie du constructeur n'est nécessaire pour cette borne supérieure.

**Poids et retours multiples.** La même preuve mathématique vaut pour une mesure
atomique Σ_i w_i δ_(x_i) avec poids positifs fixes appariés : conserver la masse
au lieu du cardinal dans la définition L et dans la qualification. Elle vaut
aussi pour une qualification par nombre d'identifiants fixes, si tel est le
contrat déclaré. Des retours coïncidents peuvent être traités comme une masse,
à condition de conserver leur correspondance ; cela n'est pas une qualification
du moteur unitaire. Compter des SITES GÉOMÉTRIQUES DISTINCTS n'est pas compter
des retours : si les classes de coïncidence changent, le cardinal distinct ne se
transporte plus. Soit les classes sont préservées, soit la formulation utilise
les identifiants/masses appariés. Une déduplication perdant leur poids n'est pas
couverte par cette preuve.

Si m dépasse la masse/cardinal total, ou k la masse disponible, le profil
peut être vide : déclarer des points inactifs ou un refus explicite. Un min
sur l'ensemble vide n'est pas une date finie couverte par ces théorèmes.

**Insertions, suppressions, changement de poids.** Aucun entrelacement à k fixé
ne découle du seul déplacement des identifiants communs. Après insertion de q
sites unitaires, on conserve L_k^X(r)⊆L_k^Y(r+ε), mais la réciproque assurée est
seulement L_(k+q)^Y(r)⊆L_k^X(r+ε). La qualification ne peut conserver le seuil m
après perte de q identifiants couverts sans le remplacer par au moins m−q. Ce
sont des inclusions à seuils différents, pas une stabilité 3ε du même H.
Exemple plus fort : X={0,2,4}, k=2,m=3, e_0=2. Ajouter η∈(0,1) près de0 donne
Y={0,η,2,4}, e_0=1 : première composante qualifiée {0,η,2} à r=1, aucun rival
qualifié couvrant0 avant sa propre fusion. Hausdorff(X,Y)=η→0, mais |Δe_0|=1.
Le checker confirme η=1/2 et1/1000. Les identifiants anciens n'ont pas bougé.

**Égalité de la constante.** La borne 3 est atteinte dans le cadre INTRINSÈQUE
abstrait : P(t,s,M)=(1,3,6), puis (5/4,11/4,25/4), entrelacement δ=1/4, dates
4 et19/4. Différence3δ. Ce n'est pas une fixture géométrique réalisant ces
changements indépendants de trois événements. Aucune égalité géométrique ni
minimalité géométrique n'est revendiquée ici.

## Q2. Optimalité après qualification : deux cadres à ne pas confondre

**Oui, si la classe est déclarée sur TOUS les profils abstraits post-qualification.**
Prendre pour entrée de Φ un profil de fusion fini remonté P, oublier son origine,
et imposer : localité Φ(P), équivariance par isomorphisme de profils, entrée à
la naissance sur une remontée seule, et Lipschitz c pour l'entrelacement
ABSTRAIT de tous couples P,P'. Alors le théorème C s'applique mot pour mot à P,
qu'on le nomme « qualifié » ou « brut ». P1 a constante3 ; aucune telle règle
n'a c<3 ; sous Mon, P1 est point par point la plus précoce parmi celles de
constante3. Mon signifie qu'ajouter une branche rivale au même minimum ne fait
jamais entrer plus tôt.

La preuve inférieure utilise explicitement deux comparateurs abstraits :
P(t,s,M) vers son symétrisé à distance≤(s−t)/2 ; puis P(t,s,M) vers la remontée
seule P0(t) à distance≤(M−s)/2. Équivariance sur le symétrisé force l'entrée
au-dessus de sa fusion. Avec g=s−t et ℓ=M−s, les deux bornes donnent
(3−c)g≤(c−2)ℓ ; ℓ→0 à g fixé exclut c<3. Mon transmet la borne de chaque
sous-profil à deux entrées au profil entier. Sources : axiomes/RAPPORT.md,
lignes182–215 ; réserve géométrique explicite lignes220–222.

**Non, pas par la seule propriété « Π_m est transportée ».** Si la classe ne
porte que sur l'image des nuages géométriques par Π_m, un comparateur abstrait
de C1 peut être hors de cette image, ou ne pas être réalisable avec le
déplacement ε promis. Une distance d'entrelacement du SEUL profil oublie aussi
les autres profils, les identifiants couverts et les effectifs décorés de FULL.
Il faut donc préciser la métrique et le domaine. La preuve de transport
garantit la borne SUPÉRIEURE 3ε, pas la borne INFÉRIEURE optimale.

Garde concrète : n=k+1, m=k+1. Avant R=MEB-radius(X), deux k-parties distinctes
ont pour union X, et leurs intersections de boules sont vides : une seule
k-partie par composante, couvrant k sites, aucune qualification. Dès R, toutes
les k-parties ont l'intersection commune non vide et FULL est connecté.
Chaque profil qualifié est alors une seule remontée née à R ; toutes les
dates et réunions de H valent R. Or R est 1ε-Lipschitz sous déplacement
apparié. Ce domaine géométrique restreint admet donc constante1 et ne contient
aucun des profils à deux branches indispensables à la preuve inférieure.
Ce n'est pas une réfutation d'un optimum uniforme sur tous n : c'est un
contre-exemple à l'inférence automatique sans préciser ce domaine.

## Q3. Retard qualifié, comparaison au cœur, récupération avant F

Noter ρ=d_k(x), t'=première couverture qualifiée, D=sup(M(p,q)−h(q)).

**L6, avec preuve complète.** Si une composante C couvre x au rayon r, prendre
c∈C, d=|c−x|≤r. Les boules B(c,r) et B(x,ρ) contiennent chacune k sites.
Forcément ρ≤r+d. Pour z=x+τ(c−x), une des deux populations tient dans une boule
de rayon min(ρ+τd, r+(1−τ)d). Son maximum sur τ∈[0,1] est au plus
max(r,(ρ+r+d)/2) : si |ρ−r|≤d, résoudre l'égalité des deux termes ; si
ρ<r−d, le premier reste≤r ; le casρ>r+d est impossible. Pour
s=max(r,(ρ+r+d)/2)≤r+ρ/2, le segment[c,x] appartient donc à L_k(s), et x∈L_k(s).
C rejoint ainsi la composante contenant x avant r+ρ/2. Le cas d=0 est immédiat.

À toute coupe r≥t', toutes les composantes QUALIFIÉES couvrant x, dont
l'ancêtre de p, rejoignent cette même composante contenant x avant r+ρ/2.
Donc M(p,q)≤h(q)+ρ/2, et

    0≤D≤d_k(x)/2 ;  t'≤e=t'+D≤t'+d_k(x)/2.

Aucune hypothèse sur la qualification de la composante contenant x à son
temps de cœur n'est nécessaire. La qualification des autres branches ne peut
augmenter ce temps de résolution.

Pour m≥k, au rayon d_m(x), x est un centre de L_k et sa boule contient au moins
m sites. La composante contenant ce centre couvre tous ces sites, donc est
qualifiée. Ainsi t'≤d_m(x), et notamment

    H^r_(k+1) : e≤d_(k+1)(x)+d_k(x)/2.

Cela ne donne aucune borne par un facteur universel de d_k : un groupe isolé
de k sites peut attendre un voisin arbitrairement lointain pour qualifier.
Il n'est PAS vrai que le retard après le cœur ne concerne que ces petits
groupes.

**Condition exacte.** Dans le code-barres du profil QUALIFIÉ, avec barres finies
[c_j,μ_j), D=max(0,max_j(μ_j−c_j)). Donc e>d_k si et seulement si
t'+D>d_k ; si t'≤d_k, cela équivaut à l'existence d'une barre qualifiée de
persistance strictement supérieure à d_k−t'. À égalité, le point est entré à
la coupe fermée du temps de cœur. Une condition ne lisant que les effectifs
des premières couvertures ne suffit pas.

**Contre-exemple exact, pas un petit amas.** k=2, m=3, sites
x=(0,0), y=(10,0), s2=(20,0), s3=(30,0), b1=(44,0), b2=(54,0),
w1=(-20,10), w2=(-20,-10), tous à z=0. Pour x : α=5, d2=10, t'=10 dans la
composante de quatre sites{x,y,s2,s3}. Un rival qualifié{x,w1,w2} apparaît à
c=25/2, rencontre cette lignée à μ=√250. Alors

    e=10+(√250−25/2)=√250−5/2>12>d2.

À F=12, le gros amas{x,y,s2,s3} vient effectivement de fusionner avec la paire
de fond{b1,b2} ; x est cœur de cette composante de six sites, mais n'est pas
encore entré. L'inégalité e>12 est exacte : 250>(29/2)². L6 est respecté,
D<5 ; c'est l'emploi de α à la place de t' dans la garantie qui était faux.

**Garantie correcte avant une fusion F.** Si t'+d_k/2≤F, le point est entré à F
et sa lignée qualifiée de première couverture a rejoint la composante qui
CONTIENT x avant t'+d_k/2. Son propriétaire à F est donc cette composante,
pas seulement une composante couvrant x. Pour récupération strictement AVANT
F, demander t'+d_k/2<F ; ≤ donne la coupe fermée APRÈS le plateau F.
La condition suffisante plus conservative d_(k+1)+d_k/2<F ne nécessite pas
t' mais peut perdre beaucoup de points. Aucune égalité asymptotique avec la
fraction récupérable FULL n'est déduite de ces bornes seules.

## Gardes et limites du reçu

`python -B check.py` et `python -B -O check.py` : 77 gardes, sorties identiques.
Le checker autonome embarque une copie figée de NOTRE noyau Gram/Fraction,
Γ exhaustif borné, sans import de référence produit ni lancement de ses main
optionnels. Les comparaisons de radicaux des fixtures sont décidées par
classes de carrés rationnels puis encadrements rationnels exacts. Un manque
de séparation est un refus explicite. Aucun test numérique ne remplace les
preuves générales ci-dessus. Les trois petits cas n=k+1 et le témoin8sites
servent à qualifier ces contre-gardes mathématiques, jamais un port natif.

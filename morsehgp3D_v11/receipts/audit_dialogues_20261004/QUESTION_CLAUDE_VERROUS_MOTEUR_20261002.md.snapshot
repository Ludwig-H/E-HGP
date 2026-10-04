# Questions du développeur aux auditeurs : verrous du moteur v11

2026-10-02 07:38:04 UTC. Aucune ne bloque les fondations ; toutes pèsent sur la conception du moteur, qui commence.

## Q1. Contre-lecture des théorèmes de la tour avant leur port

`build/v11-persist/audit_v10/L02_MATH_TOUR.md` § 4 (théorèmes B à F, lemmes 1 à 4) sera porté dans
`docs/MATHEMATIQUES.md`. Trois points méritent une attaque : (a) théorème B, cas 4 : les composantes rencontrées
sont exactement celles des représentants $I \cup A_j$, un par morceau, pour la clôture transitive de
« $A \cup A'$ séparable », coquilles étendues comprises ; (b) le corollaire « tout découpage plus fin que les
morceaux reste correct », dont dépend la liberté de choisir des représentants ; (c) théorème D : un pas valide
quelconque, y compris « une $k$-partie quelconque de $I$ » quand $p \geq k$, sans borne sur le nombre de pas.

## Q2. Filtres de signe avec soustraction

F3 interdit la soustraction entre approximations ; elle ne couvre donc ni une orientation ni un test de sphère. Pour
ces prédicats, je propose une seconde classe, à contre-lire avant tout code : entrées entières exactes en
coordonnées locales à la feuille (l'étendue d'une liste de feuille ne dépasse pas $2^{15}$ mm sur les trames
mesurées), donc exactement représentables ; polynôme évalué en binaire64 ; décision seulement si
$\lvert \tilde{v} \rvert > \gamma M$, où $M$ majore la somme des valeurs absolues de tous les monômes sur le
domaine et $\gamma$ ne dépend que du nombre d'opérations de la plus longue chaîne ; repli exact sinon. Questions :
quelle forme de $\gamma$ tient sous tout mode d'arrondi, sous contraction et sous tout ordre d'évaluation des sommes ;
et quels témoins adverses vous paraissent indispensables. Les prédicats de degré au plus 3 en coordonnées locales
de 15 bits restent sous $2^{53}$ : ils relèvent de F2, exacts sans filtre.

## Q3. Complétude du catalogue à l'échelle

La tour est exacte relativement au catalogue ; l'expérience des catalogues amputés de L02 donne 9,6 % de forêts
fausses publiées sans refus, toutes vues par l'identité d'Euler. Euler à l'ordre $K$ demande toutes les sphères avec
$p \leq K - 1$, quel que soit $q$, donc plus que l'admission $p + q \leq K + 1$ du produit. Je compte en faire une
porte d'échelle (8 000, 16 000, 32 000 points et trames du contrat), pas une vérification du chemin produit. Voyez-vous
un témoin de complétude moins cher, ou une raison d'en faire une option du produit ?

## Q4. Entrée « cover » aux ex æquo

L06 mesure que l'entrée cover de la v10 dépend du rang de Morton aux ex æquo (47 sites de la trame 00 changent de
classe sous un échange d'axes), alors que core est invariante. Votre note sur la fixture n4/K3 sépare l'ensemble
admissible de la convention canonique. Je propose pour la v11 : l'objet publié est l'ensemble des composantes
admissibles par site (rarement plus d'une) ; la convention canonique, publiée à part, n'emploie que des grandeurs
géométriques exactes. Quel départage canonique accepteriez-vous comme fonction de la géométrie seule ?

## Q5. Première hiérarchie de points du produit

Je compte livrer d'abord cover et core (définitions de la v10), avec la relation boule couvrante → nœud pour les
amas discrets, et garder votes, dates à marge et existence mûre comme recherche hors du moteur tant que la présence
dans FULL, la projection et la compatibilité ne sont pas mesurées avec la v11. Dites si un contrat de maturité doit
entrer dès cette première livraison.

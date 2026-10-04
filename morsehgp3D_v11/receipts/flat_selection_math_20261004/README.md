# Sélection plate sur H qualifiée : contrat EOM et gardes géométriques

Sources développeur épinglées : `ab1a739d17f801823a66d74609209696152c8705`,
`HIERARCHIE_POINTS.md:166–186` et `CONCEPTION_MOTEUR.md:180–212`.
La sélection n’est pas encore définie dans la première note (ligne 180).
Ce reçu aide ce choix ; il ne signale pas un défaut d’un sélecteur porté.
Aucun produit, natif, ajustement, G4 ou note active n’est modifié.

## Définir l’objet avant le score

Sur H à k fixé, les masses sont celles des **points engagés**, une unité
par site actif. La condensation doit donc intégrer les dates d’entrée eᵢ
et les plateaux atomiques de H, y compris entre niveaux FULL. Une population
de couverture FULL n’est pas cette masse ; une feuille ajoutée pour compléter
une partition totale n’est pas automatiquement active. En sens inverse,
λ=r⁻ᶻ croît lorsque r décroît : un point quitte l’arbre à λ=eᵢ⁻ᶻ et ne
contribue plus après cette date. Les cohortes inactives restent du bruit,
jusqu’à une complétion **postérieure**, nommée et évaluée séparément.

Pour un cluster condensé C né à b_C en λ, la stabilité EOM est

    S(C) = ∫ n_C(λ) dλ = Σᵢ∈C [λ_sortie(i,C) − b_C].

Le nombre n_C(λ) décroît avec les départs. Les candidats sont les clusters
de masse engagée ≥ mcs, avec la convention habituelle : une seule grande
branche prolonge le cluster ; au moins deux grandes branches créent une
séparation. L’antichaîne EOM maximise Σ S(C). Un choix parent sur **égalité
de score certifiée** respecte les symétries ; trancher par identifiant est
inutile. La racine autorisée/interdite, l’exposant z, les cas e=0 et le
traitement des cohortes inactives font partie du contrat de comparaison.

## Même hiérarchie, choix différents selon λ

Voici une réalisation géométrique de H=P₁Π₃, k=2, et pas seulement un arbre
abstrait : neuf sites collinéaires

    A={0,2,4}, B={7,9,11}, D={17,19,21}.

Le programme construit Γ₂ exact aux **toutes** coupes de paires et cofaces.
Avant r=2, aucune couverture qualifiée ne contient un site ; dès r=2,
chaque site appartient à exactement une lignée qualifiée. A et B fusionnent
à μ=5/2, puis C=A∪B fusionne avec D à R=4. Il n’existe donc aucun rival
qualifié : toutes les dates H sont eᵢ=2. Pour mcs=3, les trois petites
branches ont une masse 3 et quittent simultanément l’arbre à r=2.
La racine est exclue dans ce premier calcul.

| λ | S(C) | S(A)+S(B) | optimum local |
| --- | ---: | ---: | --- |
| 1/r | 9/10 | 3/5 | C |
| 1/r³ | 1161/4000 | 183/500 | A et B |

D est sélectionné dans les deux cas. Changer z change ainsi la partition
optimale sur un H **inchangé**. Une constante positive devant λ ou un
changement commun d’unité r↦cr multiplie toutes les stabilités par le même
facteur c⁻ᶻ et conserve les décisions ; un changement d’exposant ne le fait
pas. λ=1/r permet la comparaison usuelle avec HDBSCAN ; λ=k/r³, à k fixé,
est une autre convention de densité. Aucune n’est statistiquement optimale
par cette seule preuve. E1 doit nommer l’échelle et appliquer la même
condensation/sélection aux arbres comparés.

Si l’on autorise la racine, née à λ=0, son score pour z=1 vaut 9/4,
contre 33/20 pour la meilleure antichaîne de descendants : elle gagne
et produit un seul cluster. Cette politique ne doit pas rester implicite.
Si l’on conserve à tort les trois points d’une petite branche jusqu’à
λ=1 au lieu de leur sortie λ=1/2, le score A+B devient 18/5 : cette masse
inactive change également le gagnant. C’est une garde proposée, pas un bug
observé dans le code actuel.

## H3 ne garantit pas la stabilité de la partition sélectionnée

On remplace μ par 8/3 et garde les entrées r=2 et la racine R=4. Les sites
sont alors A={0,2,4}, B={22/3,28/3,34/3}, D={52/3,58/3,64/3}.
Le même contrôle Γ₂ confirme les mêmes lignées H. Pour λ=1/r,

    S(C) = S(A)+S(B) = 3/4.

Une translation commune de B et D par ±2δ conserve leurs rayons internes
et la fusion racine, tout en remplaçant μ par 8/3±δ. Si δ>0, les enfants
gagnent ; si δ<0, le parent gagne. Pour δ=1/1000, le déplacement apparié
maximal est 2/1000 ; neuf associations A×B changent dans la partition plate.
La construction vaut pour δ arbitrairement petit. Elle est compatible avec
la stabilité des dates de H : les hauteurs bougent de |δ|, mais la décision
d’un argmax à égalité de scores bascule. Le même avertissement s’applique
au choix de κ : rappel dans un bloc FULL monotone n’implique pas EOM monotone.

## Certificat de marge utile au développeur

Il reste une garantie **conditionnelle** concrète. Supposons une condensation
correspondante fixée : mêmes ensembles de points, candidats, cohortes et
liaisons, tous rayons positifs ≥r₀, déplacements de leurs dates ≤η<r₀.
Pour z>0, chaque valeur λ change d’au plus

    Δλ = z η / (r₀−η)ᶻ⁺¹.

Alors |ΔS(C)|≤2 n_C Δλ, où n_C est le nombre total de points de C.
Une antichaîne de descendants est disjointe et porte au plus n_C points,
donc son score change d’au plus 2 n_C Δλ ; le maximum des antichaînes
respecte la même borne. Le gain local g_C=S(C)−F_desc(C) change d’au plus
4 n_C Δλ. Si tous les arbitrages pertinents ont

    |g_C| > 4 n_C Δλ,

les décisions DP sont conservées. H3 seul ne fournit ni cette marge, ni
la correspondance des arbres condensés. Une borne uniforme sur r manque
aussi près de zéro, puisque r↦r⁻ᶻ amplifie les variations.
Le programme certifie les décisions éloignées du seuil dans le premier
exemple et vérifie que la garde ne certifie pas le témoin qui bascule.

Conseil : publier l’échelle λ, les masses engagées, la politique racine et
égalité, puis le gain parent/descendants et sa borne d’erreur. Des intervalles
certifiés suffisent aux décisions avec marge ; un chevauchement d’intervalles
n’est pas une égalité prouvée. Le comparateur exact de deux dates à six
radicaux ne qualifie pas automatiquement les sommes algébriques EOM de
milliers de points : cette extension doit avoir son propre budget/refus.

## Rapport avec la thèse et portée des vérifications

La thèse §9.1, pages imprimées 96–97 / PDF 122–123, condense des masses de
**faces**, puis vote après sélection (proposition 7). Elle prévoit aussi
un choix de poids ψ décroissant. Son résultat de partition stricte ne
prouve ni une stabilité de l’EOM, ni l’équivalence à l’EOM sur les masses
entières de H. Cette dernière est une décision de modèle distincte, déjà
annoncée par le développeur ; la remplacer tacitement par les masses de
faces changerait les seuils et les scores.

Les 536 gardes autonomes reconstituent les coupures qualifiées complètes
des trois géométries à neuf sites, puis les scores exacts, changements
d’échelle, marges et sélection. Normal et `-O` sont identiques. Aucun
résultat de qualité statistique, sélection native ou qualification G4.
Rejeu : `python -B check.py` / `python -B -O check.py` ; manifeste exhaustif.

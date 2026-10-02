# MEB locale par diamètre exact

Tranche préparée après `12f49` le 2 octobre 2026 : preuve et modèle exact
ci-dessous, qualification native G4 à venir. Aucun gain de temps acquis.
La Sphere et le support local canonique restent ceux de la primitive MEB ;
les compteurs changent parce que la stratégie de recherche change.

## Lemme et canonicité

La partie F contient n sites distincts, avec 1≤n≤12, triés par SiteIdx.
Pour n=1, la réponse reste le site de niveau zéro. Pour n>1, soit D la plus
grande distance entre deux sites de F et (a,b) la première paire dans
l'ordre lexicographique des SiteIdx parmi celles à distance D.

Toute boule contenant F a un rayon au moins D/2. Si la boule diamétrale
B(a,b) contient F, elle atteint cette borne, donc elle est la MEB unique.
Le support a,b est strict : son centre est leur milieu et a≠b.
Un support singleton est impossible pour n>1.

Réciproquement, supposons qu'une paire x,y porte une boule diamétrale C
contenant F. Puisque C contient a,b, son diamètre |x−y| est au moins D ;
la maximalité de D donne l'égalité. Dans une boule de rayon D/2, deux
points séparés de D sont antipodaux : l'égalité dans l'inégalité triangulaire
force son centre à être leur milieu. Donc C=B(a,b). Toutes les paires
maximales portent alors la même MEB, et aucune paire plus courte ne la
porte. Le choix lexicographique de a,b est exactement le support q2
canonique de l'ancienne recherche exhaustive.

Si B(a,b) ne contient pas F, cette réciproque exclut **tous** les supports
q2. La recherche reprend à q3, puis q4, dans leur ordre lexicographique
inchangé, avec les mêmes positivités strictes et tests d'inclusion. M1
assure le premier certificat contenant et sa canonicité. Aucun échec ou
triangle obtus en q3 ne supprime une extension q4.

## Arithmétique et travail payé

Le choix compare des distances carrées, sans racine ni flottant. La voie
initiale réutilise exclusivement `num::power(Sphere::point(a), b)` : dans
cette présentation q1, D=1 et N=0, donc la puissance est exactement la
distance carrée. Les comparaisons de SideInt sont exactes. Une sphère
ponctuelle auxiliaire est préparée par ancre, avant ses partenaires ; aucun
niveau q2 positif n'est construit pendant la sélection.

Pour B≤24, chaque différence signée est de module <2^B, chaque carré
<2^(2B), chaque somme partielle positive ≤trois carrés<3·2^(2B)<2^50.
Ces bornes expliquent la taille du résultat ; le produit utilise néanmoins
la primitive num déjà qualifiée et son certificat q1, sans nouvelle formule
scalaire hors de ce module. Une future spécialisation distance² serait une
tranche numérique séparée à qualifier.

`diameter_pairs` compte une puissance q1 auxiliaire par paire non ordonnée,
soit C(n,2)≤66, et zéro pour n=1. Le candidat q2 maximal est présenté une
seule fois. `presentations` compte seulement les candidats MEB testés,
jamais ces évaluations auxiliaires. `point_tests` compte les inclusions de
ces candidats jusqu'au premier extérieur ; `containing=1` et
`comparisons=0` restent vrais au succès. Pour n>1, le maximum est désormais
1+C(n,3)+C(n,4)≤716 présentations, au plus8592 tests de points, **plus**
66 distances carrées et leurs comparaisons. Pour n=1 : une présentation et
un test. La baisse de presentations seule ne mesure aucun gain de travail
arithmétique ni de durée.

Aucun Buffer, cache ou allocation n'est ajouté. La sélection conserve un
entier de distance et quatre positions fixes. Propriété, ordre des refus,
poids ignorés par la primitive géométrique, tailles maximales et budgets du
census restent inchangés.

## Portes indépendantes

Le juge Gram/Gauss/Fraction minimise toujours sur **toutes** les
circonsphères affines englobantes, sans le filtre de diamètre ni celui des
signes barycentriques pour décider la MEB. Il choisit ensuite le support
strict minimal. Une seconde lecture compte les candidats de la nouvelle
route et confronte le lemme à cette MEB indépendante.

Les témoins couvrent singleton/paire, ligne de12 sites, maxima ex æquo sur
carré/cube, triangle aigu, tétraèdre strict à préfixe obtus, extrêmes18/21/24,
partie locale distincte de la coquille globale et permutations. Les anciens
ledgers exhaustifs et de préfixe arité/lex sont explicitement refusés par le
juge courant ; les reçus historiques et leurs lecteurs restent inchangés.
Le banc MEB distingue la nouvelle stratégie par un identifiant propre,
sans changer le format des objets géométriques canoniques. Les portes FULL
et les hashes antérieurs doivent être rejugés sur G4 avant tout transfert
de qualification.

Contrôles préparatoires purs, normal et `python -O` : modèle MEB 44 796
contrôles / 32 corruptions ; cellules 241 122 / 48 ; classificateur Fraction
39 360 / 24 ; descente 61 548 / 105 ; forêt 84 639 / 156. Le collecteur MEB
factice conserve 11 tentatives et refuse 36 corruptions. Le manifeste
tower contient 65 mutations à ancres uniques ; cette validation de manifeste
n'est pas une exécution des mutants. Deux portes natives dédiées au diamètre
attendent 66 et 45 contrôles, en plus du juge Fraction et des régressions.
Aucun exécutable natif n'a été lancé localement pour cette tranche.

Le banc FULL distingue également les paires de diamètre des MEB de parties,
des traces candidates, de classification et de rejeu. `full_campaign.v4` /
`full_work.v3` vérifie pour chaque contexte la borne C(k,2) fois ses appels ;
les verticales calculées à k−1 respectent cette borne de l'ordre supérieur.
Des témoins à nombre d'appels positif et à K1 contrôlent cette garde.
Les anciens reçus conservent leurs scripts figés.

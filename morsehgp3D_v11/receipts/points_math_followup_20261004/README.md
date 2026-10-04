# FULL infini : une réunion infimum peut ne pas être atteinte

Lecture du commit développeur `ab1a739d17f801823a66d74609209696152c8705`.
Les sources sont épinglées dans `SOURCES.json`. Aucun moteur, natif, fit,
G4 ou fichier du développeur n’est modifié. Les ajouts sont une obstruction
**déterministe localement finie**, et un diagnostic pour l’expérience E1.
Le programme autonome Gram/Fraction/Γ₂ passe 52 gardes en normal et `-O`,
avec sorties identiques. La preuve infinie ci-dessous n’est pas une simulation.

## Témoin rationnel, k = 2 et m = 3

Dans le plan z = 0, on prend x = (0,0,0) et, pour tout entier n,

```
aₙ = 1/[10(|n|+1)],     bₙ = 1/[100(|n|+1)²],
Uₙ  = (n/2+aₙ,     1,0),   U′ₙ = (n/2+aₙ+bₙ,   1,0),
Lₙ  = (n/2−aₙ,    −1,0),   L′ₙ = (n/2−aₙ−bₙ,  −1,0).
```

Les sites sont distincts, le nuage est localement fini, et le demi-tour
(u,v,z) ↦ (−u,−v,z) échange les deux lignes et fixe x. Chaque ligne est
connexe dans Γ₂ dès le rayon 1/3 : trois sites consécutifs restent dans
deux groupes adjacents, leur étendue est au plus 61/100, donc le rayon
de leur MEB collinéaire est au plus 61/200 < 1/3. Toutes les paires actives
d’une ligne appartiennent à cette composante : un site intermédiaire
permet de raccorder une paire plus longue à la chaîne de paires consécutives.

La première couverture qualifiée de x, dans chacune des deux lignes, est

    t = √10121 / 200,       t² = 10121/40000.

En effet U₀=(1/10,1,0) est strictement intérieur à la boule diamétrale
de {x,U′₀}, U′₀=(11/100,1,0). Cette boule donne une coface active de
trois sites et raccorde x à la ligne supérieure ; le demi-tour donne
la même date pour la ligne inférieure. Tous les sites des groupes n ≠ 0
ont |u| ≥ 39/100 ; leur paire avec x a rayon carré ≥
(1+(39/100)²)/4 > t². Avant t, les seules paires incidentes à x susceptibles
d’être actives sont donc les quatre paires du groupe 0, et aucune coface
qualifiée ne couvre x. La couverture par un témoin ne contenant pas x
n’est pas une échappatoire : l’ajout de x donne une coface active, donc
une paire incidente à x dans la même composante (Théorème 2 / Γ₂).

Au rayon **fermé 1**, les deux composantes restent distinctes. Toute paire
entre les lignes a une différence verticale 2 et une différence horizontale
non nulle : dans un même groupe elle vaut 2aₙ, 2aₙ+bₙ ou 2aₙ+2bₙ ; entre
groupes différents sa valeur absolue est au moins 1/2−22/100 > 0.
Sa distance est donc strictement > 2. Une coface reliant les deux lignes
est impossible ; une connexion via x imposerait elle aussi une coface
avec un site de chaque ligne.

En revanche, la coface {Uₙ,U′ₙ,Lₙ} a exactement le rayon carré

    cₙ² = 1 + (aₙ+bₙ/2)² > 1,       cₙ ↓ 1 quand n → +∞.

Son MEB est la boule diamétrale de {U′ₙ,Lₙ}, contenant Uₙ strictement.
Pour chaque r > 1, une telle coface est active et relie les deux lignes.
La réunion a donc pour **infimum** 1, sans réunion effective à 1.

Le profil qualifié de x comprend exactement ces deux lignées à partir
de t. Avant 1, toute qualification d’une paire {x,z} passe par une coface
avec deux sites de la même ligne, donc rejoint la chaîne de cette ligne.
Après 1, tout témoin couvrant x est dans la composante commune : dans une
paire {x,z}, un site de la même ligne entre 0 et l’abscisse de z est intérieur
à sa boule diamétrale. Pour les sites les plus proches de 0 sur chaque
demi-ligne, la coface avec leur voisin proche est déjà active avant 1.
Ces cofaces raccordent toutes les paires incidentes à x aux chaînes existantes.

Si l’on remplace la rencontre minimale du modèle fini par cet infimum,
la formule Pκ publie **e(x)=1 pour chaque κ≥1** : les deux premières lignées
ont la même date t, donc la barre rivale contribue 1−κ(t−t)=1 ; les points
ultérieurs du profil n’apportent pas plus. Mais leurs propriétaires restent
distincts à 1. Le demi-tour les échange : aucun choix de propriétaire fidèle
et équivariant n’existe à cette date. Créer artificiellement un parent à 1
changerait la coupe fermée de FULL.

Les préfixes n∈[−1,1] et n∈[−2,2] vérifiés exhaustivement ont respectivement
13 et 21 sites. Leurs réunions sont atteintes aux niveaux carrés 401/400
et 901/900 ; leurs deux premières dates qualifiées sont t. En les étendant,
les bornes de réunion cₙ tendent vers 1, tandis que le FULL du nuage infini
reste séparé à 1. Les deux premiers préfixes illustrent ainsi la différence
entre limite des dates finies et propriétaire de la coupe infinie.

## Condition exacte à ajouter au prolongement infini

Les formulations finies de `HIERARCHIE_POINTS.md:71–77,98–105,128` utilisent
une rencontre **atteinte** et un propriétaire vivant à la date publiée.
La localité finie seule ne fournit pas cette propriété à l’infini. Une
condition suffisante à établir pour le processus considéré est : toute paire
de lignées de couverture qualifiées pertinentes qui finit par se réunir
possède un plus petit rayon de réunion à coupe fermée. La première date
qualifiée m=k+1 est, elle, atteinte : une borne donnée par le (k+1)-ième
voisin limite les parties pertinentes contenant x à un ensemble fini.
L6 borne ensuite la marge ; son supremum n’a pas besoin d’être un maximum.

La mesurabilité seule ne résout pas le problème. Dans une énumération
mesurable d’un nuage localement fini, l’existence d’un chemin fini de Γₖ
au rayon donné est un événement borélien (union dénombrable de chemins).
L’infimum des rayons de connexion est donc mesurable, même s’il n’est pas
atteint. Sous l’hypothèse d’atteinte pour les lignées pertinentes, une
construction via ces chemins et générateurs de couverture dénombrables
permet de traiter les dates, marges et propriétaires mesurablement ; l’indépendance
au témoin initial restitue ensuite l’équivariance. Il faut expliciter cette
construction, plutôt que déduire le propriétaire de la seule mesurabilité
d’une date.

**Ce témoin n’a aucune occurrence Poisson prouvée.** Il ne réfute pas une
propriété d’atteinte presque sûre du Poisson ; cette propriété ou une autre
convention infinie appropriée reste à fournir. Le certificat Palm précédent
reste conditionnel à la définition mesurable et fidèle de H∞. Sans convergence
en fenêtre finie, ΘH et Θpoly désignent des probabilités Palm à λ/F fixés,
pas encore des fractions limites d’un estimateur en fenêtre.

## E1 : distinguer κ = 1 et κ = 2 sur un même FULL

Sur un profil fini, ou un profil infini satisfaisant les hypothèses précédentes,
la formule avec barres (cⱼ,mⱼ) est

    eκ = max(t, supⱼ [mⱼ − κ(cⱼ−t)]),       cⱼ ≥ t.

Augmenter κ ne peut qu’avancer l’entrée, sur la même lignée initiale.
Ainsi, pour un **même nœud FULL C et une même coupe r**,
Hκ₁,C(r) ⊆ Hκ₂,C(r) lorsque κ₂≥κ₁≥1. Le rappel de toute cible fixée
est monotone dans ce bloc. Ni son IoU, ni son EOM, ni la partition finalement
sélectionnée ne sont monotones : l’avancement ajoute aussi des sites étrangers.

L6 donne mⱼ−cⱼ ≤ dₖ(x)/2. Pour κ>1, une barre ne peut augmenter la date
au-dessus de t que si

    cⱼ < t + dₖ/[2(κ−1)],
    mⱼ ≤ t + κ dₖ/[2(κ−1)].

Pour κ=2, les dates rivales utiles sont donc <t+dₖ/2 et leurs rencontres
≤t+dₖ. Cet horizon temporel ne démontre pas une stabilisation **spatiale** :
la connexité à rayon borné peut suivre des chemins arbitrairement lointains.

Le patron fini R1, déjà connu, donne une prédiction discriminante exacte
sans changer la qualification : t=10, c=25/2, m=√250, k=2, mqual=3.
Le programme rejuge toutes ses coupes événementielles : il n’y a qu’une
lignée primaire, ce rival indépendant, puis leur réunion. Donc

    e₁ = √250−5/2 > 12,       e₂ = √250−5 < 12.

À F=12, κ=2 récupère ce site core et κ=1 le laisse inactif. À l’inverse,
un conflit entre premières couvertures **ex æquo** n’est jamais amélioré
par κ : la contribution c=t impose e≥m pour toutes ses valeurs. E1 doit
donc contenir des rivaux nés plus tard, et publier t, eκ, la marge, les masses
et le rappel aux mêmes coupes FULL, à côté du meilleur IoU et de la sélection.
La constante de stabilité disponible passe de 3ε à 5ε ; aucun optimum
statistique de κ=1 ou 2 n’est déduit de ces garanties.

Rejeu : `python -B check.py` et `python -B -O check.py`. Inventaire exhaustif,
seul `SHA256SUMS` racine s’exclut de lui-même.

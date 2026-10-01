# Saut d'ancre réalisé par Π2c + ancien MMt pondéré

Paquet privé autonome textuel, 1er octobre 2026. K=2, mcs=3, eta=2/3, kappa=4.
Γ₂ exhaustif Fraction, cinq sites, dix paires et dix cofaces. Aucun moteur,
export, Scene, compilateur, GCP ou benchmark. Le lecteur est statique : il
ne rejoue aucune commande et n'importe aucun payload. Le SHA externe du
manifeste fourni séparément fait autorité AVANT toute lecture JSON.

## Géométrie et absence de fusion précoce

Sites coplanaires, troisième coordonnée zéro :
x=(0,0), a=(6,0), b=(0,8), z=(-1,8), y=(6,8-e), avec 0<e<=1/8,
plus le témoin exact e=0. Tous sont distincts.

t1=25-4e+e²/4 est le MEB de xay, diamètre xy (angle droit en a).
t2=16+(3-2e/3+e²/12)² est le MEB de xby, aigu pour e>0,
centre (3-2e/3+e²/12,4).

t2-t1=e²(100-16e+e²)/144>0, et 65/4<t1<t2<25.

Les dix cofaces sont exhaustivement :

| Coface | Niveau MEB |
|---|---|
| xab | 25 |
| xaz | 113/4 |
| xay | t1 |
| xbz | 65/4 |
| xby | t2 |
| xzy | >t2 |
| abz | 113/4 |
| aby | 25 |
| azy | 113/4 |
| bzy | (49+e²)/4 |

Pour xzy, les trois angles sont aigus. Son centre a ordonnée
(490-16e+e²)/(2(56-e))>4 : sa boule contient b. Elle a donc rayon
au moins celui du MEB de xby ; l'égalité est impossible car le MEB
unique de xby exclut z (puissance 1+2*c_x>0). Les autres niveaux
proviennent de diamètres droits/obtus. Le script recalcule tous les
MEB indépendamment par supports de Gram, sans importer ce tableau.

La paire xa naît à9. La branche bzy naît à(49+e²)/4 et rejoint xb
à65/4 : cette rivale couvre x,b,z,y, donc dépasse mcs=3. La paire ay
est isolée jusqu'àt1. Àt1, xay forme sa propre branche admissible
couvrant x,a,y ; elle rejoint la rivale seulement àt2. Aucun autre
des dix événements ne peut fusionner xa plus tôt. Àe=0, t1=t2=25 :
le plateau est pris atomiquement, sans branche xay de durée positive.
Le quotient conserve les continuations et les activations tardives.

## Vrais poids Π2c, pas des poids libres

Le fichier complet pipeline_snapshot.py est fourni. Seule sa classe
Admissibilite est exécutée par AST, avec les couvertures Γ₂ ci-dessus.
Elle reçoit mcs=3 et mode='continu'. Pour le porteur initial xa,
l1=t1 et l2=t2, donc omega_xa=(t2-t1)/(t2-9)>0 et tend vers zéro.
Àe=0, l1=l2=25 et omega_xa=0. Les autres paires contenant x naissent
au plus tôt à16. La première couverture de poids positif vaut donc9
pour e>0, mais16 àe=0. L'invariant Π2c n'exclut pas le saut d'ancre.
Le contrôle mcs=2 donne tous les poids égaux à1 et ancre9 partout.

## Date réellement calculée par l'ancien noyau

Le fichier complet mmt_pond_snapshot.py est fourni. Ses fonctions
_anc et mmt_point_pond sont exécutées par AST ; qsqrt_snapshot.py,
bibliothèque standard seule, fournit les racines/comparaisons exactes.
RayonAdapter implémente uniquement somme et cmp, les deux opérations
utilisées par ce noyau ; aucun import du pipeline natif n'est exécuté.

Pour 0<e<=1/8, la bande[9,15] ne contient que le porteur xa : W=6omega,
T_half=12, date_x=sqrt(12). Àe=0, les porteurs positifs de x sont :
xb : [16,65/4], poids35/36 ; rivale : [65/4,25], poids1 ; racine :
[25,80/3], poids1. W=1535/144, T_half=6145/288 et
date_x=sqrt(6145/288), environ4,619178. Le cône n'élimine pas le saut :
le noyau épinglé donne argmax=T_half. Pour a, date=sqrt(12) partout.
La hauteur de réunion(x,a) vaut sqrt(12) pour e>0, mais5 àe=0.
Ce ne sont PAS les valeurs4 ou sqrt(4/3) du contre-modèle abstrait.

Six paramètres sont contrôlés : 0,1/8,1/32,1/128,1/1024,1/4096.
Les résultats incluent tous les MEB, profils, parents/enfants, dates et
hauteurs exactes. Deux mutants causaux sont refusés : xby prématurée et
omission de la coface xay ; un mutant n'est pas compté à cause d'un crash
externe. Aucun assert. Captures normal et -O identiques.

## Portée numérique et versions

La discontinuité concerne l'ancienne construction Π2c + mmt_pond qui
définit A par les seuls poids positifs. Ce n'est PAS un défaut démontré
du nouveau MMtA (principe.py03ff7f006...), lequel utilise S=min max(c,A(v))
sans filtrer l'ancre par omega>0. Conserver cette distinction ; sa propre
continuité et ses replis restent à prouver séparément.

Les e dyadiques du test sont exactement représentables en float32. La
limite continue est un argument géométrique rationnel/réel ; une grille
fixe n'offre pas un continuum. Pour une comparaison entière d'une unité,
prendre e=1/M, homothétieM puis translation(M,0,0) :
x=(M,0,0),a=(7M,0,0),b=(M,8M,0),z=(0,8M,0),y=(7M,8M-1,0).
Le nuage témoin remplace seulement y par(7M,8M,0). Tous restent u18
pour8<=M<=32767, dontM=1024/4096 ; le saut de hauteur est
(5-sqrt(12))*M. Aucune asymptotique n inférée, aucun statut LiDAR/G4.

## Protocole et reproduction

Snapshots complets à SHA annoncés dans check.py ; le collecteur contrôle
leur égalité aux sources partagées avant/après les deux commandes.
Python/runtime identifiés dans execution.json ; la bibliothèque standard
extérieure est déclarée, pas un environnement hermétique. Les sources
partagées ont pu évoluer AVANT le gel : le SHA de classe ici exécuté est
8fb7eba7..., pas l'ancien34de900b... lu au début de la tranche.

Le préflight en échec est conservé comme résumé historique du diagnostic,
pas comme stderr intégral. Le source avant correction n'est pas archivé ;
seul son SHA est déclaré. une attente trop large date_a=sqrt(12)
àe=1/2 était fausse parce que ay entrait dans la bande. La famille fermée
e<=1/8 évite ce phénomène ; cela ne masque pas un défaut produit.

```sh
python3 -B record.py                 # une seule clôture, nouveaux fichiers requis
python3 -B read.py DOSSIER SHA_EXTERNE
python3 -B -O read.py DOSSIER SHA_EXTERNE
python3 -B check.py                  # rejeu explicite facultatif hors lecteur
python3 -B -O check.py
```

Le lecteur vérifie inventaire fermé, fichiers réguliers sans liens,
hashes, correspondance argv/capture et non-vacuité, puis invariance des
octets après lecture. Il n'appelle jamais le moteur ni check.py.

## R2 — précision du lecteur

R1 demeure clos et intact. R2 garde les octets de check.py et des trois
snapshots inchangés, recapture les deux exécutions, et renforce seulement
le collecteur/lecteur : deux argv exacts Python,-B,[-O],capture_root/check.py ;
inventaires exacts de source_before/source_after ; chaque hash égal au fichier
manifesté ; inventaire partagé et correspondance aux trois snapshots.
Le lecteur reste statique ; il ne certifie pas un runtime LIVE, et les hashes
historiques des sources extérieures ne sont pas une promesse sur leur état
actuel. Aucun timestamp ou temps d'exécution n'est déduit.

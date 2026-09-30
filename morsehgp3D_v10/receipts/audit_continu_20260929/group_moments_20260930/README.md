# Plusieurs témoins intérieurs sans témoin universel

30 septembre 2026, audit mathématique exact et isolé, moteur inchangé,
GCP non utilisé, `public_status=not_claimed`. Deux fixtures 3D de quatorze
sites u18 distincts, pas une campagne LiDAR ni une optimisation intégrée.

## Certificat quantitatif

Une sphère candidate passe par le site a et son centre c appartient à une
boîte fermée Q. Pour un groupe Z de vrais sites distincts, de multiplicités
réelles positives w, préparer W=Σw, M=Σwz, V=Σw‖z‖². Alors

```text
F_z(c) = ‖z−c‖² − ‖a−c‖²
S(c) = Σ w F_z(c) = V − W‖a‖² − 2(M−Wa)·c
σ = max_Q S(c) ; R² = max_Q ‖a−c‖²
```

Les maxima se calculent aux coins : S est affine, la distance carrée
convexe. Chaque puissance vaut au moins −R² ; les sites extérieurs ou
sur la coquille ont une puissance non négative. Si p_Z est la masse
strictement intérieure, `S(c) ≥ −p_Z(c)R²`. Pour σ<0 et R²>0,

```text
p_Z(c) ≥ ceil(−σ/R²), uniformément sur Q.
σ + θR² < 0  ⇒  p_Z(c) > θ : rejet sûr.
```

Conserver l'égalité. Ne pas inventer des poids, compter un site plusieurs
fois ou additionner des groupes qui se recouvrent. R²=0 reste indécis.
Ce renforcement du test historique à un crédit n'est pas une revendication
de nouveauté mathématique. Aucun échantillonnage fini ne prouve la borne :
la preuve est l'inégalité ci-dessus, recoupée par Fraction.

## Ce qui est contrôlé

[PROTOCOL.txt](PROTOCOL.txt) donne les coordonnées, les supports strictement
positifs et toutes les hypothèses. Dans le cas q3, aucun des trois supports
n'a de témoin individuellement universel sur Q ; le groupe en certifie au
moins quatre. Dans le cas q4, aucun des quatre supports n'a de tel témoin ;
le groupe en certifie au moins trois. Cela dépasse les seuils tight FULL K5
θ3=3 et θ4=2, sans prétendre valoir pour les feuilles pondérées à θ=K−1.

Le premier cas contient aussi un support q4, mais son quatrième sommet
possède déjà cinq témoins universels : **pas de gain q4 nouveau sur ce cas**.
Le second cas a été séparé précisément pour éviter cette erreur causale.
Ces certificats géométriques ne signifient pas que le générateur produit
exactement ces Q ni que son coût diminuera sur les scènes considérées.

[receipt.json](receipt.json) conserve deux passages Fraction normal/−O,
5 793 contrôles chacun, sorties identiques et sources/protocole figés avant
exécution. Deux mutants logiques Python échouent avec code1 pour la bonne
cause : `≤` élimine une sphère admise à égalité ; sommer des crédits de
groupes superposés annonce quatre intérieurs lorsqu'il n'y en a que trois.
Ce ne sont pas des mutants natifs du moteur. Zéro appel natif, chrono ou GCP.

## Essai utile pour le développeur

Préparer les moments d'un petit nombre de groupes une fois par feuille
locale ou nœud immuable, puis tester les ancres avant d'énumérer les tuples.
Un candidat de sélection à essayer, **non qualifié ici**, est le groupe
entier puis six groupes obtenus en retirant un extrême XYZ, égalités par ID :
au plus sept groupes préparés en O(m), tests O(7m) sur m ancres. Il ne faut
ni chercher toutes les sous-populations, ni sélectionner un groupe par tuple.
Le groupe complet ou les plus proches du centre peuvent donner une borne
trop faible ; la sélection fait partie du coût à mesurer, pas d'un oracle.

Les masques q3/q4 ne concernent que l'éligibilité comme **support**.
Ne pas retirer ces ancres du nuage ni du census : elles peuvent rester
intérieures, sur la coquille ou participer à q2. Les crédits anonymes servent
au rejet, pas à initialiser le compte exact d'une boule émise.

Index partagé, descripteurs immuables et états de tâches privés rendent
ces petits calculs parallélisables ; l'implémentation CPU/GPU reste à faire.
Prévoir les largeurs exactes selon coordonnées ET masses. Le coût ouvert
reste Σ_Q |ancres(Q)|·|groupes(Q)|, préparation, sélection, résidu, census
et sorties compris. Mesurer 8k/16k/32k et les coupes capteur LiDAR, avec/sans
sol : aucun gain, caractère sous-quadratique ou contrat100ms n'est acquis.

## Lecture

`python3 -B verify.py` et `python3 -B -O verify.py` vérifient d'abord
[SHA256SUMS](SHA256SUMS), puis rejugent les fixtures et les deux erreurs
logiques, sans processus natif ni GCP. `CAPTURE_SHA256SUMS` conserve la
première clôture de l'agent ; elle n'est pas réécrite par cette présentation.

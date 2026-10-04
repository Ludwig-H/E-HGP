# Correction de portée : sortie brute et sortie après condensation

Supplément au reçu clos `flat_selection_math_20261004`, qui reste intact.
Ses preuves géométriques, scores EOM, changements d’échelle et certificat
de marge restent valables. Le premier paragraphe nécessite cette précision :

> Dans **H brut**, un point devient inactif à λ=eᵢ⁻ᶻ. Après condensation,
> il peut être éliminé plus tôt avec une branche de masse <mcs ; sa sortie
> condensée vérifie donc λ_sortie≤eᵢ⁻ᶻ. Son départ d’un cluster parent lors
> d’une séparation peut également précéder sa désactivation brute.

Dans S(C)=Σᵢ(λ_sortie(i,C)−λ_naissance(C)), il faut employer les dates
de départ du **cluster condensé**, qui incluent les petites branches,
et non remplacer systématiquement λ_sortie par eᵢ⁻ᶻ. La borne du reçu
initial vaut pour ces dates et la condensation correspondante fixée.

Garde simple : sur les neuf sites initiaux {0,2,4,7,9,11,17,19,21}, tous
les eᵢ valent 2. Avec mcs=4 et λ=1/r, la branche D de trois points tombe
au rayon 4 (λ=1/4), tandis que les branches A/B de trois points tombent
au rayon 5/2 (λ=2/5). Ces dates précèdent la sortie brute λ=1/2.
La seule grande branche C prolonge l’identité du cluster racine ; elle
ne crée pas un nouveau candidat. Sa stabilité totale, départs compris,
vaut 3×1/4+6×2/5=63/20. Sous cette convention standard, exclure la racine
donne tout bruit à mcs=4. Ce résultat précise le contrat ; aucun défaut
de sélecteur produit n’est allégué.

## Variante entière pour la bascule EOM

On multiplie par 3 le patron d’égalité du reçu initial :

    A={0,6,12}, B={22,28,34}, D={52,58,64}.

Tous les sites sont valides dans les profils u18/u21/u24. On translate
B et D ensemble de −1, 0 ou +1. Le Γ₂ scalaire indépendant vérifie **toutes**
ses coupes exactes : chaque site a une seule lignée qualifiée dès le rayon
6, aucun rival qualifié ; H=P₁Π₃ est donc ce même arbre. La fusion A/B
est à 15/2, 8 ou 17/2, et la fusion racine reste à 12.
Pour mcs=3, λ=1/r et racine exclue :

| translation | score C=A∪B | score A+B | sélection |
| ---: | ---: | ---: | --- |
| −1 | 3/10 | 1/5 | C et D |
| 0 | 1/4 | 1/4 | C et D, parent sur égalité certifiée |
| +1 | 7/34 | 5/17 | A, B et D |

Ce sont des vérifications **mathématiques entières**, aucune exécution
native ni qualification des profils. La discontinuité sous perturbation
arbitrairement petite du reçu initial concerne la géométrie continue ;
dans un domaine u21 fixé, les entrées forment un ensemble fini. Ici, une
variation entière d’une unité suffit déjà à changer l’antichaîne optimale.

526 gardes passent en normal et `-O`, sorties identiques. `SOURCES.json`
recoupe avant/après les huit fichiers du reçu initial et son manifeste.
`historical/r1_manifest.txt` est la copie exacte de cet ancien manifeste,
pas un inventaire des fichiers du supplément. Le nouveau `SHA256SUMS`
couvre exhaustivement le supplément, y compris ce document historique.
Rejeu : `python -B check.py` et `python -B -O check.py`. Aucun autre travail.

# Résultats du pilote pondéré FULL — 27 septembre 2026

**Bilan mixte : la mesure pondérée ne domine ni HDBSCAN ni la première
couverture sur ce pilote.** La topologie corrigée est bien celle de FULL ;
cette exactitude géométrique ne constitue pas un théorème de supériorité
des labels par rapport aux classes gaussiennes.

Publication close : [tableaux complets](../../receipts/weighted_full_gaussian_20260927/r2/TABLES.md),
[364 lignes](../../receipts/weighted_full_gaussian_20260927/r2/rows.csv),
[140 agrégats](../../receipts/weighted_full_gaussian_20260927/r2/aggregates.csv),
[reçu et provenances](../../receipts/weighted_full_gaussian_20260927/r2/receipt.json).
Cette r2 dérive de la publication r1 scellée, conservée en privé : seul un
espace final Markdown a été retiré, sans recalcul ; les deux CSV sont
identiques octet par octet. Le reçu lie les deux versions par leurs hashes.
Les résultats K10 et seuil50 sont tous conservés ; aucun meilleur réglage
n'est substitué au profil principal annoncé ci-dessous.

## Ce qui a été comparé

Treize scènes complètes de 1 200 points, K5/10, seuil20/50, expZ1/2 : trois
graines pour chacun des trois régimes sphériques, deux pour chacun des
deux régimes de stress. Les cinq régimes étaient déjà connus lors du choix
du pilote : **diagnostic, pas test tenu à l'écart**. La vérité terrain sert
aux scores, pas au calcul des masses, aux attaches ou aux choix EOM.

- **Pondéré FULL** : cofaces Gabriel complètes pour les scores/masses,
  vraies attaches MEB et connectivité FULL pour l'arbre, condensation
  massique, EOM, puis vote plat des points.
- **Première couverture** : ancienne projection exclusive de points,
  masses ponctuelles unitaires et EOM commun. Elle ne représente pas la
  même mesure que le vote pondéré.
- **HDBSCAN commun** : son arbre, masses unitaires, condensation/EOM communs.
  **HDBSCAN standard** conserve sa voie propre, en z1 uniquement.

La racine est exclue de l'EOM commun. Le seuil pondéré est une masse de
facettes **avant vote**, pas un minimum garanti de points par groupe final.
Aucun filtre de taille ni remplissage 1-NN n'est appliqué après vote.

## Principal : K5, seuil20

ARI moyen ± écart-type entre graines ; il ne s'agit pas d'intervalles de
confiance. Les effectifs sont trois pour les lignes sphériques et deux pour
les autres. F1 par classe, couverture et nombres de groupes restent dans
les tableaux complets et les CSV ; l'ARI ne les remplace pas.

### expZ1

| Régime | Pondéré FULL | Première couverture | HDBSCAN commun | HDBSCAN standard |
|---|---:|---:|---:|---:|
| Sphérique G2, δ8 | 1,0000 ± 0,0000 | 1,0000 ± 0,0000 | 1,0000 ± 0,0000 | 1,0000 ± 0,0000 |
| Sphérique G8, δ4 | 0,1729 ± 0,0007 | 0,4948 ± 0,2825 | 0,2239 ± 0,1191 | 0,2243 ± 0,1205 |
| Sphérique G16, δ2 | 0,0226 ± 0,0117 | 0,0201 ± 0,0164 | 0,0197 ± 0,0063 | 0,0196 ± 0,0062 |
| Anisotrope G8, δ4 | 0,1970 ± 0,1734 | 0,1970 ± 0,1717 | 0,1856 ± 0,1686 | 0,0660 ± 0,0003 |
| Déséquilibré G8, δ4 | 0,3115 ± 0,0096 | 0,5359 ± 0,2008 | 0,2686 ± 0,0373 | 0,2685 ± 0,0372 |

### expZ2

| Régime | Pondéré FULL | Première couverture | HDBSCAN commun |
|---|---:|---:|---:|
| Sphérique G2, δ8 | 1,0000 ± 0,0000 | 1,0000 ± 0,0000 | 1,0000 ± 0,0000 |
| Sphérique G8, δ4 | 0,5134 ± 0,2994 | 0,4948 ± 0,2825 | 0,2239 ± 0,1191 |
| Sphérique G16, δ2 | 0,0224 ± 0,0116 | 0,0357 ± 0,0242 | 0,0197 ± 0,0063 |
| Anisotrope G8, δ4 | 0,1970 ± 0,1734 | 0,4755 ± 0,1483 | 0,2523 ± 0,0743 |
| Déséquilibré G8, δ4 | 0,3110 ± 0,0089 | 0,5359 ± 0,2008 | 0,3473 ± 0,0740 |

Le cas G2/δ8 est parfaitement retrouvé par les quatre méthodes. Sur G8/δ4
sphérique, pondéré z1 régresse ; z2 relève son ARI moyen de 0,1729 à 0,5134,
avec une forte variabilité entre les trois graines. En anisotrope z2 et
déséquilibré z2, HDBSCAN commun dépasse le pondéré ; la première couverture
dépasse aussi le pondéré. Sur G16/δ2, tous ces ARI restent très bas : le
faible avantage moyen pondéré sur HDBSCAN n'est pas une récupération
satisfaisante des seize classes.

La couverture plus élevée du pondéré n'est pas automatiquement une meilleure
classification. Exemple anisotrope z1 : 95,7 % couverts contre 70,2 % pour
HDBSCAN commun, mais F1 macro apparié 0,1938 contre 0,4410 et 2,5 groupes
contre 5 en moyenne pour huit classes vraies. Les deux diagnostics doivent
rester visibles.

## Ce que change expZ

Pour HGP pondéré, `ψσ = rσ^(−z)` modifie les scores Sτ, les normalisateurs
T_x, les masses de facettes **et** la variable de stabilité `λ = r^(−z)`.
Le passage z1→z2 n'est donc pas une simple transformation du rayon d'un
arbre déjà muni de la même mesure. Chez les deux comparateurs à EOM commun,
les masses ponctuelles restent unitaires ; seule λ change dans cette étape.
La comparaison n'est pas une ablation isolée de λ.

Les masses et choix de la campagne sont en binary64. Aucun nœud proche du
seuil ni aucune égalité de vote n'est enregistré sur les 104 sorties
pondérées ; cela ne certifie pas toutes les comparaisons flottantes.
Aucun groupe ponctuel final n'est ici sous son seuil numérique annoncé,
mais cette observation ne transforme pas un seuil massique en garantie de
cardinalité. Le contrôle exact des sommes dyadiques porte sur les masses
arrondies reçues, pas sur les poids géométriques réels.

## Captures, reprise et contre-audit

La campagne parallèle close comprend **26 unités scène/K** : dix unités
complètes reprises et seize nouveaux workers, au plus deux simultanément.
Chaque unité contient quatre sélections pondérées et dix lignes de
comparateurs héritées. Les 16 groupes de processus possédés sont clos.
Le reçu séquentiel interrompu reste `failed` ; sa clôture originale des
sources était absente. La vérification LIVE lors de la reprise ne la recrée
pas rétroactivement. Le premier échec de reconstruction Gabriel reste lui
aussi conservé, sans score extrait de cet essai.

Le contre-audit normal et `-O`, identique, recalcule **104 ARI exacts** par
contingences entières/Fraction et vérifie 260 lignes de comparateurs,
52 mesures, 39 135 752 sorties de facettes, masses, votes et 1 175 pins.
Le solveur Hungarian SciPy reste partagé pour l'appariement des classes ;
le NMI n'est pas recalculé. Aucun nouvel ajustement ou EOM n'est exécuté
pendant cette lecture.

| Preuve | SHA256 |
|---|---|
| Capture parallèle privée, `capture_r2/receipt.json` | `577930f26f4f97fd4f13fb99bb1e356e232a8479055c7f6e09a1bab87bbea720` |
| Contre-audit normal, identique à `-O` | `b048e8d34e8e3548e4e05340f333301c276ef04a0cacbbde4ad99814e010ee00` |
| Publication r1 scellée, conservée en privé | `4493c0de42163aa1692941f4b72037101656b52799170cde427f590b001c6017` |
| Reçu public r2, dérivation typographique | `d105081b2bbbc30d0e1efc41ca76aea410bfab0b6c2272293b5ab3154e1e59a0` |

Les chemins privés détaillés, commandes, hashes des fichiers publics et
provenances des qualifications figurent dans le reçu public lié plus haut.
Les nuages et les grands JSON natifs ne sont pas publiés.

## Coût et suite distincte

Ce pilote matérialise de nombreux objets Python et facettes, avec capture
gzip et observateur FULL CPU supplémentaire. Les durées mélangent unités
réutilisées et concurrence ; les 894,4 secondes du lancement parallèle
n'incluent pas le coût initial des dix unités reprises. Ce n'est ni une
latence industrielle des 26 unités, ni un gain série/parallèle, ni une
mesure GPU/G4. Aucun nouveau budget cloud n'a été consommé pour ce pilote.

La [référence de routage ponctuel emboîté](POINT_ROUTING_REFERENCE.md) est
désormais qualifiée séparément sur petits cas exacts. **Aucun score de ces
tableaux ne l'évalue** : les résultats ci-dessus portent sur le vote plat
après EOM. Ses coupes exclusives et datées ne promettent pas un meilleur ARI.
La [compression par dépôts datés](DEPOTS_DATES.md) reste une piste distincte,
avec [comptages capturés](DEPOTS_MESURES.md), pas un accélérateur déjà mesuré.

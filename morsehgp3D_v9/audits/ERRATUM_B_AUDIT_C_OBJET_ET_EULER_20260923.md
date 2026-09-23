# Contrelecture B du grand audit C : portée mathématique et juge d'Euler

23 septembre 2026. Base : `f4480c02`, contrôlée jusqu'à `4079cceb`.
Audit documentaire et mathématique, sans changement du moteur ni GCP.
Le [grand audit C](AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md)
apporte une description utile de la chaîne. Les points ci-dessous corrigent
des conclusions qui dépassent les preuves ; ils ne démontrent pas un défaut
du code de la tour.

## 1. Coquille étendue : la formule générique de multifusion ne s'applique pas

Les règles « naissance à K=p+u », « multifusion à K=p+u−1 » et la
multiplicité de Reani–Bobrowski citées en §1.3 valent en position générale,
pas pour une coquille arbitraire U. Contre-exemple exact : les quatre points
(0,0,0), (2,0,0), (2,2,0), (0,2,0). Leur boule commune a centre (1,1,0),
rayon √2, p=0, u=4 et q_min=2 (une diagonale opposée suffit). À K=3,
aucune boule de rayon inférieur à √2 ne couvre trois points : L_3 est vide.
Au rayon √2, L_3 est le singleton formé par le centre. Il y a **une naissance**,
non une fusion de quatre composantes préexistantes. Le graphe Γ_3 reçoit
simultanément les quatre triples et leur coface à quatre points.

Le produit distingue justement les coquilles étendues et emploie leur
quotient local (`src/tower/forest/local_plateau.hpp`,
`src/tower/forest/full_ball_tower.hpp`). L'erratum demandé porte donc sur
l'explication et sur le statut de la formule, pas sur une faute démontrée
du moteur. La convention q_min≥2 concerne les boules critiques de rayon
strictement positif ; les singletons de rayon nul à K=1 sont séparés.

## 2. Euler est un contrôle nécessaire, jamais une preuve de complétude

La démonstration de E_K=1 pour K≤Kmax−2 est valable également sans position
générale ([preuve par le nerf](CONTRELEC_EULER_PAR_NERF_20260923.md)).
Ses 18/18 cas LiDAR et les neuf mutants qu'elle tue sont un progrès réel.
Mais des omissions de contributions opposées peuvent se compenser ; une
somme correcte ne certifie pas toutes les clés. Les formules « vérifie la
complétude à l'échelle » (§3.2 et verdict) et « changer de générateur sans
perdre la complétude » (§7) doivent dire « détecte certaines omissions ».

Le protocole Kmax+2 n'est qu'une cohérence supplémentaire entre deux
exécutions. Le cas sain K5→K7 à 8k compare actuellement `[nombre,
somme commutative de hachés sur 64 bits]`, pas les clés une à une
(`c_euler_20260923/euler_check.cpp`). Même une comparaison exacte entre
ces deux catalogues laisserait passer leurs omissions communes ; la somme
64 bits ajoute une possibilité de collision. Ce protocole ne couvre pas K10
avec le domaine moteur actuel limité à K≤10 : il faudrait K12.

## 3. La campagne de mutants ne situe pas les omissions survivantes

Sur 35 exécutables K5/8k, les verdicts archivés sont 9 `killed_euler`,
10 `killed_chain`, 16 `survived`. Parmi ces derniers, 13 ont **la même
cardinalité** que le catalogue sain, non un catalogue comparé clé par clé.
Les trois autres ont moins de boules et satisfont E_1=E_2=E_3=1 ; cela
ne prouve ni que les clés omises ne touchent que K4/K5, ni qu'aucune
compensation n'a lieu. En particulier le mutant
`new_admission_wrong_xi_scale` modifie un seuil de témoin sans restriction
de profondeur aux deux derniers ordres. La note Euler dédiée emploie déjà
la formulation plus prudente « même taille, sans alarme ».

Une mutation nommée `admitted_lane_recounted_in_children` est désactivée
dans `tests/gen/mutants.json` car sa cible textuelle apparaît deux fois ;
le runner de cette campagne ignore `disabled` et modifie la **première**
occurrence. Son verdict ne teste donc pas la faute annoncée. Il faut
sélectionner le site exact, puis refaire ce seul cas.

## 4. Statut des tests

« 128 portes toutes vertes en local et en CI » contredit dans le même audit
la CI rouge. La lecture sous-jacente rapporte 127 portes exécutées et une
désactivée : dater la capture locale, publier PASS/SKIP et distinguer l'étape
CTest du workflow entier. Aucun de ces constats ne retire la valeur des
oracles bornés ni de la tour exacte **relativement au catalogue**.

Suite recommandée : corriger §1.3, les paragraphes Euler/mutants et le
tableau de statut dans le rapport C ; comparer les clés des mutants
survivants, garder l'invariant en porte nécessaire et un oracle de
complétude indépendant pour les tailles où il est praticable.

## 5. Contrelecture du correctif Euler prêt à porter (`513e0b26`)

La formule du [patch proposé](c_euler_20260923/euler_chain_probe.patch) est
correcte pour les coquilles étendues : `ShellTable::contains_center()`
marque les sous-coquilles pertinentes. Sur le carré du §1, deux diagonales,
quatre triples et le carré entier donnent le polynôme local
`2(t−1)+4(t−1)^2+(t−1)^3 = 1−3t+t²+t³`, donc les contributions
`(e_1,e_2,e_3,e_4)=(1,−3,1,1)`. Cette fixture manque encore à la porte
d'intégration du patch.

Le patch n'est **pas prêt tel quel** pour le lecteur de contrat :

- `euler_checkable_max_k=kmax−2` doit être plafonné par le nombre de sites
  (`min(kmax−2,n)`). La chaîne autorise Kmax>n ; avec deux sites et Kmax=10,
  E_1=E_2=1 mais E_3..E_8=0, et le patch marque à tort le catalogue valide
  comme échoué. Pour Kmax=1 ou 2, `euler_holds=true` est vacuant : publier
  « non applicable » ou exiger explicitement `euler_checkable_max_k>0`.
- Le calcul ajouté est **dans** `census_ms`, `chain_total` et `chain_cpu_s`.
  Il n'est pas « hors chrono ». Mesurer l'ablation appariée mur/CPU/RSS,
  même si la géométrie des coquilles étendues est déjà préparée.
- La branche régulière suppose le support positif minimal. FULL le vérifie,
  mais `run_tower=false` ne le fait pas. La porte doit exiger une tour FULL
  complète (ou recertifier la positivité). Sur refus ultérieur, le statut
  d'échec prime toujours sur les champs Euler déjà calculés.
- La sonde garde le schéma `mhgp9_tower_probe_v12` ; le lecteur local
  `bench/run_lidar_scaling.py` ignore `euler_by_k`, `euler_checkable_max_k`
  et `euler_holds`. Ajouter un schéma/lecteur cohérents, des mutations de
  champs absents/faux/mal typés et l'exigence de chaque E_K=1 vérifiable.
  Un `euler_holds` vrai reste une condition nécessaire, jamais un statut
  de complétude démontrée.

Le reçu proposé ne contient que deux projections K5/K10 de 08/000200 8k.
Boules et condensés concordent avec l'ancienne sonde, mais les sorties
brutes, identité du binaire, temps et ablation appariée manquent : aucun
coût G4 ni identité intégrale des tours ne s'en déduit. Dans la campagne
mutante, les neuf détections Euler sur K≤3 restent numériquement valides
sur la coupe testée ; renforcer néanmoins les codes de sortie, le
recomptage, les hashes et les comparaisons de clés avant d'en faire une
porte reproductible.

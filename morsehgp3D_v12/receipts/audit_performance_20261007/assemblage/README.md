# Finition : retirer les parcours séquentiels sans changer le catalogue

7 octobre 2026, auditeur Codex, pin `58d384721678d11ef8ccd86c76cca182f41a716c`.
Contre-lecture du code et modèle indépendant **hors produit**. Aucun chrono nouveau.

## Régression structurelle

`src/catalogue/assemble.cpp` remet en série l'aplatissement des `Chunk`, `rank_levels`,
le calcul des décalages CSR et le choix du représentant de chaque niveau. Seul le
remplissage final est parallèle. `table.cpp::build_table` compte, place et trie toutes
les lignes en série ; son interface ne reçoit même pas le Pool.

La v11 gelée avait déjà `AssemblyPlan::scan/fill` dans
`morsehgp3D_v11/src/catalogue/assembly_parallel.cpp` : blocs, halo gauche, somme des
totaux de blocs, puis remplissage parallèle. Ce mécanisme est une source différentielle
explicite ; sa preuve se réutilise, **pas sa qualification**. Le comparateur de supports
v11 par SiteIdx doit être remplacé par celui des positions du contrat v12. Aucun retour
aux anciens supports canoniques n'est nécessaire pour retrouver cette parallélisation.

Les mesures de F2 rendent la finition prioritaire : voir la [relecture chiffrée](../mesures/README.md).
L'absence de Pool dans la table ou les boucles séquentielles sont établies ; la part du
temps de chaque boucle, des allocations ou des défauts de cache ne l'est pas séparément.
Ne pas attribuer les 122–171 ms de finition à une seule instruction.

## Proposition A : rangs et CSR par blocs, preuve

Soit la permutation déjà triée des B boules, par niveau rationnel exact puis support
canonique. Poser `start[0]=1`, et, pour i>0, `start[i]=1` si et seulement si le niveau
de i est **strictement** supérieur à celui de i−1. Une comparaison F4 peut décider
l'ordre seulement si elle le certifie ; sinon garder la comparaison rationnelle exacte.
À égalité de niveau, vérifier l'ordre strict des supports par positions (doublon refusé).
Noter `length[i]=p[i]+m[i]`.

1. Chaque bloc contigu [a,b) lit son halo i=a−1, sans le modifier. Il calcule les sommes
   inclusives locales de `start` et exclusives de `length`, plus ses deux totaux.
2. Un scan **exclusif** des totaux donne `rank_base` et `population_base` par bloc.
   Il porte sur ceil(B/grain) éléments, jamais sur les B boules ni les incidences.
3. En parallèle : `rank[i]=rank_base+local_rank[i]` et
   `off[i]=population_base+local_offset[i]`. Chaque bloc copie ses populations I puis U,
   sans les fusionner ni les réordonner. `off[B]` est le total, niveau 0 conservé.
4. **Seul `start[i]=1` écrit `levels[rank[i]]`**. Il copie la forme rationnelle originale
   de la première boule de ce rang, même si d'autres fractions égales suivent.

Preuve : l'associativité des sommes entières donne exactement les préfixes séquentiels.
Le halo compare la seule paire traversant la frontière, donc un plateau traversant
plusieurs blocs reste un seul rang. Les plages CSR sont disjointes ; chaque rang positif
a un unique premier indice. Il n'y a aucune écriture concurrente du même niveau, et le
représentant non réduit est conservé. La forme `(100,4)` et `(409600,16384)` ne doit
jamais créer deux rangs. Le choix du dernier représentant est également incorrect pour
l'identité binaire de **ce catalogue v12**, même quand la valeur rationnelle est égale.

Travail O(B+incidences), scratch O(B/grain) en réutilisant le tableau des rangs localement ;
les sommes d'incidences restent u64 contrôlées, le rang final garde le refus u32. Tous
les tableaux sont admis avant lancement, les jobs sont joints avant restitution, et
le résultat entier reste privé jusqu'au succès. Les contrôles de capacité de la v11
ne doivent pas disparaître pendant le port. L'aplatissement des chunks peut être copié
par les mêmes blocs après un préfixe de leurs tailles, sans changer l'ordre physique.

## Proposition B : table de supports

Première ablation simple : trier les lignes indépendantes sur le Pool, sans changer
comptage/placement. Elle ne retire pas les scans séquentiels de B entrées : la mesurer
séparément, ne pas la présenter comme une parallélisation complète.

Alternative complète à comparer : former une permutation des BallIdx triée par les
**quatre SiteIdx** du support (sentinelle kNone conservée), puis former les lignes CSR
par première composante. Les clés sont uniques : un support critique détermine une
boule unique ; le contrôle explicite d'un doublon reste une porte d'intégrité. L'ordre
dans chaque ligne est alors exactement celui de `tail_less`. Les lignes vides doivent
aussi recevoir leurs décalages. Un tri parallèle existant avec comparateur adapté
évite les histogrammes de taille `workers * sites` ; un radix déterministe est un
autre candidat, mais son scratch et son coût doivent être publiés.

Ne pas confondre cette clé d'index, **SiteIdx croissants**, avec la clé d'ordre canonique
des boules par positions. Les requêtes `find_support` gardent leur contrat et doivent
refuser un support minimal non canonique. Chaque empreinte utilisée comme accélérateur
est vérifiée par les quatre identifiants, jamais admise seule comme certificat.

## Vérification bornée

Depuis la racine du dépôt :

```sh
python3 morsehgp3D_v12/receipts/audit_performance_20261007/assemblage/verify_blocked_finish.py
python3 -O morsehgp3D_v12/receipts/audit_performance_20261007/assemblage/verify_blocked_finish.py
```

[Résultat](result.json) : 131 suites, **3 537** comparaisons scan/oracle, permutations
de l'ordre de fin des blocs, six refus, quatre mutants tués (halo omis, comparaison
des formes au lieu des valeurs, préfixe inclusif, dernier représentant). **13 468**
requêtes comparent la table par permutation à un dictionnaire exact, avec lignes vides
et supports de deux à quatre sites. Sorties normal/−O identiques, sans assert.

Ce sont des tableaux synthétiques d'étage, **pas des nuages géométriques, ni le C++
parallèle du produit**. Restent obligatoires au raccord : identité de catalogue,
borne mémoire et refus, W1/W48, empoisonnement, sanitizer/concurrence adaptés, puis
ablation G4 du vrai chemin. Aucun gain de temps annoncé pour ces propositions.

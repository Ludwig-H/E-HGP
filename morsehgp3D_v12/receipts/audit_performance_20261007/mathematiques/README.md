# Contre-lecture mathématique et leviers de calcul

7 octobre 2026 ; code figé `58d384721678d11ef8ccd86c76cca182f41a716c`. Cadre :
`phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Lecture et petits témoins exacts indépendants ; aucune mesure de vitesse, aucun calcul LiDAR, aucun GCP.

## Conclusion pour le développeur

La régression de temps n'appelle aucune simplification de l'objet mathématique. Plusieurs changements de disposition
et de calcul peuvent préserver exactement le catalogue et FULL. Les deux priorités locales sont les scans parallèles
de fin d'étage (preuve séparée par l'auditeur principal) et les clés de supports préparées ci-dessous. La table des
supports peut se construire en parallèle ; ses lignes sont indépendantes. Les caches géométriques ont des clés et
des domaines précis : le niveau seul, le cardinal ou une empreinte seule ne suffisent jamais.

Le changement d'égalité à l'octet vers l'égalité sémantique de `MES-M0` au commit audité est justifié. Il ne justifie
aucune tolérance sur les niveaux exacts, les centres, la topologie ou les verticales. La première phrase du § 1 de
`docs/CONTRAT_TOUR.md`, « Même objet que la v11 gelée, à l'octet », reste contradictoire avec sa correction ; écrire
« Même tour FULL que la v11 gelée, par valeurs exactes et topologie canonique ».

## 1. S*, catalogue et empreinte FULL

Une boule critique a un support minimal S dont le centre appartient à l'intérieur relatif de conv(S). Le minimum
lexicographique des supports minimaux fixe une **présentation**, pas une nouvelle boule. Une bijection de catalogues
qui préserve centre, rayon carré, I, U et q_min préserve les cellules de fenêtre et leurs traces : la tour des
composantes et ses verticales sont donc les mêmes. La sélection d'un squelette de Kruskal ou d'un `cover` peut changer
au plateau : elle reste un contrat de sortie distinct et ne se déduit pas de FULL identique.

Le changement des octets peut survenir **sans aucune coquille étendue** : triangle
`(15,10,3),(7,14,3),(7,6,3)` et paire `(0,100,0),(0,110,0)`. Les deux boules sont critiques de niveau 25,
respectivement écrit `409600/16384` et `100/4`. Le triangle vient d'abord dans l'ordre des supports en Morton ; la
paire vient d'abord dans l'ordre des positions. Puisque `assemble.cpp` garde la forme de la première boule du rang,
le représentant rationnel change. Les deux centres diffèrent : ce témoin réfute aussi un cache de géométrie par
niveau seul.

Le lecteur strict `morsehgp3D_v11/bench/full_semantic.py` n'efface que les écritures équivalentes des rationnels et
le remplissage des entiers. Son hash contient : coordonnées, poids, PointId ; parents, décalages et cardinalités
CSR ; niveaux réduits ; centres réduits des naissances ; images verticales ; enfants. Le format ne contient aucun
S*, BallIdx ni CellIdx : ces indices n'ont rien à renuméroter dans l'empreinte. Les sites, PointId et nœuds canoniques,
eux, sont bien comparés. Le lecteur contrôle l'ordre de Morton, l'unicité des identités, les ordres canoniques des
naissances/fusions, la topologie, la vie à coupe fermée et la naturalité des verticales.

Le témoin `check_math.py` construit la vraie tour de deux points à distance 10, K2 : changer les deux écritures de
niveau et multiplier numérateurs/dénominateur du centre par 4 change le hash brut et conserve le hash sémantique.
Changer niveau, centre ou PointId change le hash ; muter parent ou verticale est refusé. Ce lecteur structurel
n'est pas un oracle géométrique : il admet un centre géométriquement faux mais dans la boîte, en changeant son hash.
Son domaine actuel reste 18/21/24 bits ; l'adaptation 32 bits prévue par T2 doit être vérifiée séparément.

## 2. Préparer une clé de support exacte une fois

Dans `src/catalogue/sort.cpp`, `compare_support_positions()` recharge douze coordonnées et trie les positions
des deux supports à chaque égalité ou ambiguïté de niveau. La transformation suivante est exacte :

1. Trier une fois les sites distincts par `(x,y,z)` et donner les rangs lexicographiques `1..n`.
2. Pour chaque support, trier ses 2 à 4 rangs ; compléter à quatre cases par zéro.
3. Comparer ces quatre entiers lexicographiquement lorsque les niveaux sont exactement égaux.

**Preuve.** L'application position → rang est strictement croissante et injective. Le premier site différent de
deux listes a donc le même ordre dans les deux représentations. Si l'une est préfixe de l'autre, le zéro final est
inférieur à tous les rangs positifs et reproduit l'ordre des listes de `compare_support_positions()`. Cette règle
marche même pour des supports quelconques. Elle ajoute 4n octets de rangs de sites et, si on la matérialise pour
chaque boule, 16B octets de clés, où B désigne ici le nombre de boules. Garder cette mémoire dans le budget et mesurer
le gain net ; aucun gain de temps n'est déclaré par cette preuve.

Il faut **conserver les SiteIdx originaux** dans S*, les populations, la table et le parcours ; les rangs
lexicographiques sont une clé auxiliaire. Les remplacer partout changerait l'ordre parent des listes K-certifiées
et la partition des feuilles. Pour conserver un bourrage par `kNone` comme au contrat plutôt que zéro, la validité
au seul départage de niveaux égaux suit aussi d'un lemme : deux supports minimaux de même rayon ne sont jamais
strictement emboîtés. Si S ⊂ T et leurs rayons minimaux sont égaux, l'unicité de MEB(S) impose la même boule ; son
centre ne peut appartenir simultanément à conv(S), face propre de conv(T), et à l'intérieur relatif de conv(T).

Le témoin vérifie 30 381 comparaisons sur 246 supports et neuf positions, avec des coordonnées aux limites u32.

## 3. Tables, populations et mémoïsation : clés suffisantes

**Support minimal → boule.** La boule est MEB(S), unique ; un support ne peut indexer deux boules admises distinctes.
Construire la table S* en parallèle ne change donc aucune décision. Les tris de lignes CSR par
`(S*[1],S*[2],S*[3])` sont indépendants. Ne pas réutiliser la permutation globale des boules comme ordre d'une ligne :
elle est triée par niveau, puis positions, alors que la recherche de table attend le tuple de SiteIdx.

**Population complète → boule.** Pour une boule critique b, écrire P_b = I_b ∪ U_b. Un support minimal S satisfait
S ⊆ U_b ⊆ P_b ⊆ b. Par monotonie et unicité de la plus petite boule, MEB(P_b) = b. Ainsi deux boules critiques du même
nuage ayant la même population complète sont identiques. Une entrée de population peut se préparer une fois par
boule, être conservée par la Session et servir à toutes les recherches. Fusionner les deux listes déjà triées I et U
coûte O(p+m), sans nouveau tri général. L'ordre d'une naissance est p+m ; ce n'est pas une naissance à tous les ordres.
Une empreinte additive facilite une jointure, mais seule l'égalité intégrale des identifiants et du cardinal décide.
Une collision d'empreinte n'est ni égalité ni refus.

**Coquille complète dans une feuille → boule.** Le même sandwich donne MEB(U_b) = b. Une feuille peut donc mémoriser
`masque U -> (S*,q_min)` et éviter de ré-énumérer les supports canoniques des présentations d'une même sphère. Cette
équivalence exige une présentation génératrice déjà certifiée minimale et une coquille complète dans une liste
K-certifiée ; elle ne vaut pas pour une sphère arbitraire passant par deux points. Vérifier le masque complet, pas
son hash seul, et conserver le propriétaire, la liste et sa convention d'ordre. Dans les trois petits nuages du
témoin : 66 présentations critiques, 12 réemplois de coquille et de population, mêmes boules exactes. Ce levier
vise les dégénérescences ; la rareté des coquilles étendues sur LiDAR ne permet pas de le prioriser sans profil.

**Partie complète F → MEB(F).** Un cache exact de la géométrie peut être partagé entre requêtes de la même Session
immuable si sa clé est l'ensemble trié complet des SiteIdx. Le cardinal est alors inclus dans la clé. Le support
trouvé peut être réutilisé pour une nouvelle F seulement après les deux inclusions `S ⊆ F ⊆ b` ; le support proposé
ou son niveau seuls ne sont pas des clés. Pour mettre en cache une résolution plutôt qu'une boule, inclure aussi
l'ordre k, le catalogue/session et la politique de résolution si on exige les mêmes compteurs de travail. Garder
la date initiale β(F) et les contrôles de décroissance ; une cible terminale ne devient pas utilisable à sa seule
date terminale. Les compteurs physiques doivent publier les réemplois ; les compteurs de travail de la politique
ne se confondent pas avec une preuve d'identité de l'objet.

## 4. Autres raccourcis admissibles et limites

- **Tri GPU par clé flottante positive, puis exact.** Trier par les clés ne décide aucun ordre exact. Après le tri,
  couper uniquement à une frontière dont l'ordre exact est certifié par F4, puis trier exactement toute la chaîne
  restante de voisins incertains, avec la clé exacte de support aux égalités. Une simple réparation de paires
  adjacentes disjointes ne suffit pas. La preuve de frontières utilise la monotonie des clés positives : une
  séparation certifiée entre la plus grande clé du bloc gauche et la plus petite du bloc droit sépare tous leurs
  éléments. Garder les domaines finis, la borne d'erreur F3 et le même F4 ; traiter un éventuel zéro explicitement.
- **Niveau exact une fois par support admis.** Il est mathématiquement inutile de reconstruire la sphère à chaque
  comparaison ; `assemble.cpp` le fait déjà une fois par boule. Transporter le niveau depuis la feuille évite une
  reconstruction, mais augmente l'arène et les transferts : pas d'adoption sans mesure complète. Réduire tous les
  rationnels par PGCD est inutile pour le tri exact par produits croisés et peut ajouter un coût important.
- **Coquilles > 32.** Le passage au warp virtuel ne change pas l'objet si les mêmes prédicats exacts et les masques
  complets sont utilisés. La borne de coquille 64 est un refus explicite du produit, pas une conséquence de
  Carathéodory : q_min ≤ 4 ne borne ni m ni le nombre de présentations. Un résultat tronqué à 64 changerait l'objet.
- **Palier numérique.** Un support étroit ne certifie pas le domaine de tout le census : l'enveloppe de la boîte
  fermée et de tous les sites interrogés reste nécessaire. Une preuve de cache ne relâche jamais cette garde.
- **Admettre p + q_min ≤ K+1.** Une présentation de cardinal q peut être rejetée par p > K+1-q même si une autre
  présentation de la même sphère a q_min < q ; cela est sûr uniquement parce que le support canonique minimal est
  lui-même visité. Aucun raccourci de présentation ne doit supprimer cette garantie.

## Rejeu et portée

Depuis la racine du dépôt :

```sh
python -S morsehgp3D_v12/receipts/audit_performance_20261007/mathematiques/check_math.py
python -S -O morsehgp3D_v12/receipts/audit_performance_20261007/mathematiques/check_math.py
```

Les deux sorties doivent être identiques à `resultats.json`. Les témoins arithmétiques utilisent une résolution
linéaire en `Fraction`, indépendante des formules C++ ; le lecteur FULL effectivement exercé est celui de la v11,
dont l'empreinte source figure dans `sources.json`. Ce reçu démontre des équivalences et des contre-exemples bornés,
pas la correction complète d'une nouvelle implantation, sa vitesse ni le contrat FULL/G4.

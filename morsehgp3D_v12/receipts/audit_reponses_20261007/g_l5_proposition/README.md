# G-L5 : jointure exacte des premières sondes, proposition bornée

7 octobre 2026. Lecture au pin `b24c256541ffdbe8327f894cd9efebdd7476b51c` ; [sources.json](sources.json).
Proposition pour le développeur, sans code moteur, calcul géométrique, chrono ni GPU. Cadre v12 hors registre,
`cpu_reference`, `full_pi0`, `quantized_u21_input_only`, `public_status=not_claimed`.

**Apport au contrat existant :** une jointure qui ne forme jamais le produit cartésien de deux groupes de hash
égaux ; un état précis de reprise des échecs ; un budget qui garde la table des sondes suivantes. Le gain établi
est structurel dans cette jointure, pas un gain de temps ni une borne sur tout G. Commencer par ce modèle sur CPU
permet de mesurer la préparation sans attribuer un bénéfice anticipé aux transferts GPU.

**Domaine et clé.** Traiter un ordre `k≥2` du même domaine immuable (Cloud, catalogue et leur génération). L'ordre 1
conserve sa voie directe `FirstOrder`. B désigne seulement les naissances de `birth_keys` telles que `p+m=k`, comme
`PopulationTable::build` ; les autres naissances, notamment étendues, ne deviennent pas des lignes de cette table.
La clé est la suite complète de k `SiteIdx` strictement croissants, issue de la fusion de I et U. La requête est la
trace complète `F=I∪A` construite depuis le masque, avec son indice de représentant et son rang de jonction.
Ni un masque seul, ni S*, ni les sommes, ni les PointId des retours ne remplacent F.

Pour une ligne de naissance b, `S*(b) ⊆ F ⊆ b` et la plus petite boule de S* est b. Le rayon minimal de F est donc
à la fois ≥ et ≤ celui de b : `MEB(F)=b`. L'unicité de la plus petite boule implique qu'à domaine et ordre fixés,
deux naissances distinctes ne peuvent partager cette population complète. Refuser comme invariant violé une clé
de naissance répétée, même avec le même payload ; ne pas masquer une duplication de production. En revanche,
plusieurs représentants peuvent porter le même F : chacun garde sa case de cible et ses contrôles.

**Algorithme concret.**

1. Matérialiser une fois les clés des B lignes admissibles et les Q premières traces. Porter l'indice de naissance
   original de l'ordre, jamais son nouveau rang de tri. Garder les identifiants de représentants originaux.
2. Trier chaque flux par les k mots u32 complets : tri radix LSD stable, quatre passes de huit bits par mot,
   du dernier mot au premier. Un hash préalable est facultatif ; dans ce cas trier **(hash, F complet)**, en
   recalculant le même hash depuis F des deux côtés. L'exactitude ne repose jamais sur ce hash.
3. Fusionner les flux avec un curseur de naissance monotone. Une requête égale à la clé complète courante reçoit
   la naissance ; une requête absente reste en échec. Les requêtes répétées utilisent la même ligne sans consommer
   ni perdre leurs représentants. Le curseur ne recule pas et avance au plus B fois : aucune boucle B×Q sous
   collision, même si toutes les empreintes valent zéro.
4. Pour chaque succès, contrôler **`rang_naissance < rang_jonction`**, puis écrire `birth_target(indice_original)`.
   Même F avec deux origines ne dispense pas du second contrôle. Un défaut fait refuser l'étage avant publication.
   Une réussite LEM-POP ne consulte pas la table des cellules de fenêtre ; les échecs reprennent la voie actuelle.

**Compteurs et reprise.** Une requête compte une première sonde logique, même si sa recherche physique a été
groupée. Un succès ajoute `first_probe_hits=1`, `controls=1` et `chain_histogram[0]=1`. Un échec reprend exactement
après cette sonde ratée, avant `locate`, avec `chain=1`, F intact et `Previous.rank=rang_jonction` ; aucune cible,
aucun contrôle de naissance et aucune chaîne terminée n'ont encore été comptés. Ne pas rappeler `resolve_part`
depuis son entrée en lui ayant déjà compté la première sonde. Les sondes ultérieures, certificats, censuses,
fenêtres et contrôles exacts sont inchangés. Les trois égalités de contrôle de `stage.cpp:133–135` restent requises.

**Travail et mémoire explicités.** Pour un ordre entièrement admis en mémoire, la préparation, les tris et la
fusion coûtent `O(k(B+Q)+256k)` opérations sur mots, avec `k≤12`, et `O(k(B+Q))` stockage. Le hash facultatif ajoute
huit passes d'octets, pas un facteur dépendant du nombre de collisions. Une estimation conservatrice de staging
en colonnes séparées est `(4k+24)B + (4k+36)Q + O(256)` octets : clés u32 ; deux permutations u64 par flux ;
payload naissance 8B ; représentant/rang 12Q ; liste d'échecs jusqu'à 8Q. Les cibles produit déjà allouées ne sont
pas recomptées. Le hash facultatif, s'il est stocké, ajoute 8(B+Q) octets. L'implantation parallèle doit ajouter ses
histogrammes/préfixes par bloc ou travailleur et vérifier
les multiplications avant allocation ; le modèle Python n'est pas une mesure de ce budget C++.

**La table actuelle reste nécessaire aux sondes suivantes.** La jointure ne supprime donc pas automatiquement
son coût de construction, ses `8C` octets de cases (`C` puissance de deux ≥2B, `C<4B` si B>0), ni le catalogue,
les cellules et les espaces de census. Tous coexistent avec le staging ci-dessus. La borne linéaire concerne cette
jointure, pas les collisions ou le coût du reste de G. Remplacer les recherches ultérieures serait une autre
modification à déclarer et mesurer.

Si Q doit être découpé en t blocs, une fusion complète répétée avec B coûte `O(k(tB+Q))` : ne pas annoncer la borne
précédente pour ce montage. Variante bornée : trier les B naissances une fois, puis rechercher chaque clé distincte
d'un bloc par dichotomie exacte ; coût `O(kU log(B+1))` de recherche pour U clés distinctes, mémoire limitée au bloc.
Elle évite aussi un carré dû aux collisions, avec un coût d'accès différent à mesurer. Aucun quota ne peut omettre
des traces. Le coût complet à comparer inclut préparation, table existante, tris, restitution, continuation et pic
mémoire ; ajouter transferts/synchronisations si la version GPU est ensuite évaluée. Pas d'amortissement entre
trames supposé sans domaine immuable partagé démontré.

**Preuve reproductible légère.** [model.py](model.py) confronte le radix+fusion à un scan indépendant des
naissances : **180 confrontations, 7 560 réponses**, k=2/3/5/10/12, au plus 18 naissances et 42 requêtes par jeu.
Trois modes (sans hash, hash constant, somme modulaire) et trois ordonnancements simulés ; aucun parallélisme natif
n'est qualifié. Huit refus sont vérifiés, dont une seule origine au rang égal pour des F répétés, les doublons de
naissance et l'alias de case de représentant. Même population dans trois propriétaires/générations : cibles
distinctes 7/11/13 préservées. Les entrées sont abstraites, pas des catalogues dont la géométrie aurait été prouvée.

Contre-exemple au hash pris pour certificat : la naissance `F=(0,3)` et la requête `(1,2)` ont même cardinal et même
somme 3 ; le raccourci par somme rendrait à tort sa cible 7, la jointure exacte répond « échec ». Ce témoin ne
prétend pas être une collision de la fonction de hash actuelle. Un hash n'est ni signe de prédicat géométrique,
ni preuve LEM-POP. Le modèle ne modifie aucune opération arithmétique exacte du moteur.

```sh
python -B -S morsehgp3D_v12/receipts/audit_reponses_20261007/g_l5_proposition/model.py > /tmp/g-l5-normal.json
python -B -S -O morsehgp3D_v12/receipts/audit_reponses_20261007/g_l5_proposition/model.py > /tmp/g-l5-opt.json
cmp /tmp/g-l5-normal.json /tmp/g-l5-opt.json
```

Captures identiques en normal/−O : [normal.json](normal.json). Aucun nouveau constat de défaut produit n'est ouvert
par cette proposition ; le lecteur actuel vérifie déjà l'égalité complète après hash. Les conditions d'adoption
MES-G3 (sortie identique et baisse du temps complet de G) restent à démontrer.

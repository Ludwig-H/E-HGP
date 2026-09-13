# Colonnes q2 : comparer des géométries isométriques

13 septembre 2026, quatrième passe après `65ac5ee6`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Sources axiales en construction, capturées séparément
des qualifications publiées R3. Aucun correctif moteur dans cette note.

## Résultat et conséquence pratique

Le filtre axial sait garder tout un facteur B en un seul descripteur
lorsqu'aucune colonne n'est exploitable : **ce repli évite effectivement
un travail quadratique de construction**. Une rotation exacte suffit
toutefois à transformer la nappe favorable en ce cas. Le résidu change
alors sans que les profondeurs q2 ou les paires finales changent.

| n, besoin h10 | Candidates alignées | Après rotation exacte | Retour au repère exact |
| ---: | ---: | ---: | ---: |
| 8 000 | 1 475 800 | 16 000 000 | 1 475 800 |
| 16 000 | 3 124 300 | 64 000 000 | 3 124 300 |
| 32 000 | 6 483 670 | 256 000 000 | 6 483 670 |

Ces nombres proviennent du **vrai filtre C++ actuel** sur les deux
orientations et leur retour exact, s8/10/12 vérifiés. Le census est
exécuté seulement sur les petites instances ; aucune grande sortie q2
ou FULL n'est annoncée. Les points de cette comparaison appariée sont
différents des nappes du benchmark constructeur : ne pas comparer leurs
latences ou résidus comme s'il s'agissait des mêmes entrées.

À n32k, la nappe tournée provoque trois tris de 16 000 IDs, 48 000
colonnes singleton et 16 000 descripteurs. L'arbre B n'est pas construit,
aucune requête de ses nœuds n'est exécutée. Le problème restant est le
volume aval de 256 millions de paires, pas un carré caché dans ce repli.
Sur la nappe alignée, l'arbre reçoit 3 529 456 visites de requête et
produit 766 418 descripteurs : ces coûts sont conservés dans le reçu.

## Fixture entière sans arrondi

Partir de v=(x,i,j), avec x=0 ou 8000 et une grille transverse entière.
Les trois formes sont 50×80, 100×80 et 125×128. Pour
o=(5000,25000,30000), définir p=o+3v et p'=o+Qv avec :

$$Q=\begin{pmatrix}1&2&2\\2&1&-2\\-2&2&-1\end{pmatrix},\qquad Q^{\mathsf{T}}Q=9I,\qquad \det Q=27.$$

La transformation entre p et p' est donc une rotation autour de o,
sans changement d'échelle. Les deux nuages restent u16, séparés aux
trois valeurs de s et de même ordre d'IDs. Toute distance carrée et
tout test d'intérieur diamétral sont conservés.

Le plan tourné a pour normale (1,2,−2). Aucune de ses composantes n'est
nulle ; une droite parallèle à un axe cartésien ne rencontre ce plan
qu'en un point. Les trois tris du filtre ne peuvent donc produire que
des colonnes singleton. Avec besoin positif, aucun témoin axial n'est
disponible, et le filtre garde exactement A×B.

Sur la nappe alignée complète, il garde les offsets |Δi|≤h et |Δj|≤h.
Pour des côtés N_y,N_z>h, le compte fermé vérifié par le juge est :

$$M=\big((2h+1)N_y-h(h+1)\big)\big((2h+1)N_z-h(h+1)\big).$$

Le retour p=o+Qᵀ(p'−o)/3 est entier pour cette fixture. Le juge teste
la divisibilité avant chaque division et refuse une perturbation d'une
unité qui la détruit. Arrondir un nuage quelconque pour appliquer ce
retour ne serait pas autorisé : cette démonstration n'introduit aucun
changement silencieux du profil ou des points.

## Extension constructive : colonnes de direction déclarée

Le certificat axial est un cas particulier d'un certificat directionnel
exact. Pour un vecteur entier non nul d, grouper les IDs qui partagent
d×p, puis trier chaque groupe par d·p. Deux sites de ce groupe vérifient
z=a+td. Leur prédicat q2 est :

$$H=(z-a)\cdot(b-z)=t\big(d\cdot b-d\cdot z\big).$$

Si h sites se trouvent après a dans ce tri, le demi-espace strictement
au-delà de la projection du h-ième site fournit h intérieurs. Le cas
opposé est symétrique et les frontières restent candidates. Le noyau
géométrique ne nécessite ni coordonnées flottantes ni recherche par paire.
Pour des composantes de d bornées par 65535, les produits avec les
coordonnées u16 et les clés vectorielles se calculent exactement en i64.

Une famille **déclarée** de directions remplace la boîte admissible
par une intersection de bandes obliques. Pour interroger l'arbre B,
les minimum/maximum de d·p sur une boîte se calculent en choisissant ses
coins selon les signes de d. Rejeter un nœud seulement lorsqu'une bande
le sépare entièrement ; accepter lorsqu'il est entièrement dans toutes
les bandes ; sinon descendre. Les trois directions colonnes de Q
retrouvent ici les colonnes de la nappe initiale. Le retour de repère
exécuté ci-dessus en vérifie le résultat sur cette famille, sans constituer
une implémentation de ces requêtes obliques générales.

Préparer t directions coûte O(t|A| log|A|), puis viennent les visites et
descripteurs du résidu. **Trouver ces directions sans essayer tous les
couples reste ouvert.** Des directions connues de l'entrée ou une courte
liste proposée puis certifiée sont comparables ; aucune liste universelle
efficace n'est prouvée. Des colonnes exactes restent sensibles aux
perturbations : cette extension ne remplace pas les certificats de blocs
pour les données sans alignement exact.

## Rejeu borné

Le [runner](axis_rotation_checks.py) capture sept sources C++ dans un
répertoire temporaire neuf, puis compile le [juge](axis_rotation_probe.cpp)
en C++20 strict avec UBSan. Le [reçu](AXIS_ROTATION_CHECKS.json) conserve
les hashes, compteurs et sorties des modes normal/−O. Chacun passe
27 configurations, trois plans par configuration : neuf configurations
avec expansion et census exhaustifs, puis 18 grandes sans développement.
Le juge effectue 3 709 380 tests ponctuels indépendants par mode sur les
petites entrées, conserve toutes les survivantes et vérifie l'isométrie.
Le reçu embarque les sept sources et le juge ; le mode −O a été rejoué
sur ces seuls octets. La commande reste autonome après modification du
worktree :

```bash
python3 -B -O audits/morsehgp3D_v8_complementaire/axis_rotation_checks.py --selftest --replay audits/morsehgp3D_v8_complementaire/AXIS_ROTATION_CHECKS.json
```

L'intérêt de ce test est de comparer le coût total à objet final égal,
sans imposer des candidates intermédiaires invariantes par rotation.
Le filtre est sûr dans les deux cas ; son efficacité est à borner à sa
géométrie favorable. WSPD, census général, tour FULL et GCP non qualifiés.
GCP non utilisé.

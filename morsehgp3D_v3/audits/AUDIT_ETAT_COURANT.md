# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`,
`mode=order_k_flats_owner_differential_and_gate_d_f0`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype
et ne vaut ni promotion produit, ni ouverture de phase. Les fichiers source
étaient non commités pendant l'audit; le verdict est donc rattaché au commit de
base et aux empreintes live suivantes :

| objet | empreinte |
| --- | --- |
| `HEAD` | `fbfb2c0425a5b5a3c062b5eac92019075126c21d` |
| `prototype/order_k_flats.hpp` | `02ad6f58632de60d47e0b2bbcdf6205d8a3b9d1cab1474dd9d8b566593e9e81a` |
| `prototype/flats_differential.cpp` | `5dc937b205503b9c88cd7f75206b7dc4fec6770eca3a94f74c35ffadf6b4665f` |
| `prototype/order_k_device_core.hpp` | `e9e0c713a6c957aa3779ac20077e921725d7936eeece05d4557a9b8b95a66606` |
| `CMakeLists.txt` | `fdc00942cc8aed26f46c40ad3a95ef7be040d968ff819fd1ffb9368f171946c4` |
| `audits/check_gate_d_fold_f0.py` | `34149092cd1b06762085800ac9d575c0cb8022e3a1c273c7d1955d2f4e768294` |

## Verdict

**NO-GO de justesse owner sur le profil u16, NO-GO F0, et noyau device encore
candidat hôte sous audit.**

Un P0 entier invalide le précédent GO ciblé owner : `owner_rays_ok` transmet un
déterminant exact `i128` à `tangent_sign(int, ...)`. La conversion implicite
tronque le déterminant, perd des sphères, peut en dupliquer une autre et atteint
un cas de négation de `INT_MIN` détecté par UBSan. Les cinq campagnes usuelles
restent vertes parce que leurs coordonnées ne franchissent pas cette frontière.

Les avancées restent utiles : la porte compare beaucoup mieux le payload, la
signature de permutation conserve membres et multiplicités, le high-water de
`emitted` est relevé aux insertions, les high-waters du parcours sont exposés et
la borne publique de `s_max` est alignée sur `kMaxRank`. Aucun de ces crédits ne
compense le P0.

## P0 — le déterminant exact est tronqué avant le signe tangent

### Cause exacte

`orient3d_exact` rend un `i128`. `tangent_sign` accepte un `int` qui représente
déjà `-1`, `0` ou `+1`. L'appel live lui passe pourtant la valeur brute :

```cpp
tangent_sign(orient3d_exact(a, b, c, s), delta)
```

La faute se situe donc à l'interface valeur/signe. Ce n'est ni un dépassement
de `i128`, ni une limite du théorème owner. La correction locale testée hors
dépôt est de passer `sign_of(orient3d_exact(...))`, ou de donner à
`tangent_sign` une entrée `i128` qu'elle réduit elle-même.

### Deux fixtures minimales

Pour le tétraèdre axial
`(0,0,0),(L,0,0),(0,L,0),(0,0,L)`, `s_max=2` :

| `L` | déterminant | catalogue normal | catalogue owner |
| ---: | ---: | ---: | ---: |
| 1290 | 2 146 689 000 | 7 | 7 |
| 1291 | 2 151 685 171 | 7 | **4** |

À 1291, les supports `{0,1}`, `{0,2}` et `{0,3}` disparaissent. La frontière
est exactement celle de `INT_MAX`.

Pour le tétraèdre alterné
`(0,0,0),(L,0,L),(L,L,0),(0,L,L)`, le déterminant vaut $2L^3$ :

| `L` | fait observé |
| ---: | --- |
| 1023 | déterminant encore représentable et catalogues concordants |
| 1024 | déterminant $2^{31}$; conversion en `INT_MIN`, puis `-INT_MIN` signalé par UBSan |
| 1025 | catalogue normal 10, owner **4**; six paires perdues |

La sortie Release de 1024 peut paraître correcte par accident; seule la porte
UBSan empêche d'en faire un crédit.

### Contre-exemple u16 déterministe

Commande :

```sh
mhgp3v_flats_differential --clouds 1 --points 8 --coord 65536 --smax 2 --seed 20260809
```

Points :

```text
(36710,43342,56661) (44151,42733,3225)
(32173,25451,115)   (33771,34375,37708)
(35086,39924,50732) (52589,30461,40720)
(18842,305,53369)   (61180,45472,62312)
```

Le catalogue normal rend 19 sphères. Owner rend 16 enregistrements mais
seulement 15 sphères distinctes : `{0,4}`, `{0,7}`, `{4,5}` et `{5,7}` manquent,
tandis que `{1,3}` est émise deux fois. Un probe ne changeant que la réduction
du signe restaure un propriétaire unique pour chacune des onze paires et les 19
sphères attendues.

### Porte exigée

La correction n'est fermée que si le dépôt conserve :

1. les frontières axiales 1290/1291 et alternées 1023/1024/1025;
2. le cas 1024 sous UBSan;
3. une compilation de ce site avec `-Wconversion`;
4. un mutant qui retire `sign_of` et doit rougir;
5. la comparaison du catalogue et de l'unicité du propriétaire, pas seulement
   un nombre total.

## Verrous mathématiques transmis à Claude

La construction complète, avec preuves et fixtures, est dans
[`NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md`](NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md).

### F0 : naissance générale et source régulière sont distinctes

Le fold général doit accepter une composante avec `q_R=0` dès qu'elle porte une
`DirectHyperedge`. Une coface non régulière peut réellement avoir toutes ses
facettes activées au même niveau : le carré coplanaire, protégé par un cinquième
point extérieur à sa miniboule, en donne une fixture géométrique d'arité quatre.

Sous la porte régulière forte, une hyperarête directe possède au contraire
exactement $\lvert U(B)\rvert\geq2$ facettes strictes. Cet invariant se valide
**par record source avant projection**, avec le reçu de miniboule et le census
terminal. La garde live « au moins un `R` ou `L` dans la composante » ne prouve
pas cet invariant : un record direct tout `N` est accepté s'il est relié à un
second record portant un `L`.

Le script rend encore `PASS` pour Warshall et DSU parce que les deux chemins
partagent la même garde fautive. Une vérité indépendante doit repartir du
`RawBatch`, énumérer les partitions bornées et appliquer directement la table
`q_R`/record direct. Les contrôles contractuels doivent être explicites, car
`python3 -O` désactive encore une partie des obligations basées sur `assert`.

### Owner : les rayons d'une paire se réduisent en un scan exact

Pour $U=\lbrace p_0,p_1\rbrace$, les contraintes du cône signé se ramènent à des
demi-plans centraux sur $e^\perp$. Un automate exact à six états
`FULL/HALF/LINE/RAY/WEDGE/ZERO` maintient leur intersection en
$O(\lvert S(v)\rvert+\lvert B_U\rvert)$ et mémoire $O(1)$. Il rend au plus deux
rayons extrêmes et évite l'énumération puis le rescannage de chaque triplet.

Cette réduction abaisse le terme owner des paires de
$O(m^4+m^3\lvert B_U\rvert)$ à $O(m^3+m^2\lvert B_U\rvert)$. Elle ne ferme pas
le harvest des triples, qui peut encore garder un terme $O(m^4)$. Elle ne doit
être intégrée qu'après le P0 de largeur, avec un oracle exhaustif des rayons et
des permutations de contraintes.

### Census terminal : report exact de demi-espace en dimension quatre

Pour `Sphere{base,n,d}`, poser $C=d\,\mathrm{base}+n$ et
$N=\left\Vert n\right\Vert^2$. Le signe exact contre un point $p$ est celui de
$F_B(p)=d^2\left\Vert p\right\Vert^2-2dC\mathbin{\cdot}p+\left\Vert C\right\Vert^2-N$.
Après le relèvement $\varphi(p)=(p,\left\Vert p\right\Vert^2)$, le census fermé
est donc un report de demi-espace affine dans $\mathbb{R}^{4}$.

Cette construction évite toute mosaïque de Delaunay d'ordre supérieur. Le
chemin produit doit regrouper les sphères rationnelles identiques, interroger
une structure de report exacte une fois par sphère et rendre un statut cappé
fail-closed. Un scan `cpp_int` borné reste l'oracle, pas l'architecture produit.

## Audit des portes owner permanentes

### Progrès crédités

- La matrice index × owner compare maintenant statut et payload sémantique; les
  vérités 11 du tétraèdre et 7 du triangle sont imposées.
- La signature de permutation conserve les occurrences, les membres et les
  multiplicités au lieu de les écraser dans un `set`.
- `owner_signed_cone` et le cube multi-support sont permanents.
- `dedup_table_high_water` est relevé à chaque insertion; une remise à zéro en
  fin de calcul ne trompe plus ce compteur.
- Le chemin owner+index+navigable garde effectivement cette table à zéro sur les
  cinq campagnes usuelles.

### Couverture encore insuffisante

- `owner_signed_cone` compare la sortie, pas l'identité du sommet propriétaire.
  Le mutant non signé échange le propriétaire de l'extrémité `z=0` avec celui de
  `z=4`, sans changer le catalogue ni le high-water. Le commentaire source qui
  affirme que cette fixture protège seule `eps=-1` est donc faux.
- `same_catalogue` ne compare pas `CriticalSphere::beta`, la représentation
  complète de la sphère, `members_begin` ni d'éventuels membres orphelins. Un
  mutant qui décale seulement `CriticalSphere::beta` passe la porte usuelle.
  Le message « catalogue entier » dépasse ainsi le contrat réellement testé.
- La vérité de cardinalité du nuage coplanaire à cinq points n'est pas imposée.
- Le différentiel owner partage navigation, census, miniboule et support
  canonique avec sa référence. Il prouve une équivalence de déduplication
  relative à ces primitives, pas leur exactitude géométrique indépendante.

Le mode reste hybride : sans index, les singletons gardent $O(n)$ clefs; sur la
voie directe, `emitted` peut rester en $\Theta(\text{sortie})$. Le high-water
publié compte des entrées, pas les octets, les buckets, `kept`, `members_pool`
ou les sommets.

## Audit du noyau borné candidat device

### Résultats positifs

Le cœur borné concorde avec le chemin CPU sur tous les sommets admis des cinq
portes : 328 560 sommets et 2 703 016 couples, sans divergence observée. Un
probe séparé a pu instancier `decide_child` et produire du PTX avec Clang 18;
c'est un signal de portabilité structurelle, pas une qualification CUDA.

Le diff live sépare utilement `exhaustive_scans` de l'amorce ayant seulement
couvert toute la grille et aligne l'API `flat_catalogue` sur la borne dure
`kMaxRank=32`. Il tente aussi de publier des high-waters de coquille, fermeture,
points touchés, lot et intérieur. L'intention est correcte, mais le point de
mesure et la condition de porte ne le sont pas encore.

### P1 de qualification

- La porte reste vacuable : aucun plancher n'exige un nombre minimal de sommets
  admis, de couples ou de décisions device. Si tout le bloc device disparaît,
  `device_admitted=0` saute aussi la nouvelle vérification des high-waters.
- Les cinq campagnes rendent zéro refus de capacité. Il manque au minimum une
  coquille de taille 33 et un intérieur de taille 17.
- Lors d'un refus de voisin, le harness fait `continue`; le repli hôte annoncé
  n'est ni exécuté ni comparé de bout en bout.
- `device_pairs` compte les tests d'admissibilité, pas les appels à
  `device::decide_child`. L'affirmation « verdict par verdict » n'a donc pas son
  propre compteur ni son plancher.
- Le build v3 reste C++ hôte : aucune cible `.cu`, aucun passage `nvcc` et aucun
  kernel exécuté ne qualifient encore le device.
- Avec `s_max<=32`, la borne théorique de l'intérieur atteint 30, supérieure à
  `kMaxInterior=16`. Cette capacité est licite seulement si le refus et le repli
  hôte sont permanents et reçus.
- La commande `--clouds 1 --points 21 --coord 100 --smax 19 --seed 1
  --min-cases 1` rend 233 cas et zéro désaccord, avec 94 refus de capacité et un
  high-water intérieur 17/16, puis échoue uniquement parce que la nouvelle porte
  impose `high_water<=capacity`. Elle transforme donc un refus explicite réussi
  en échec. La propriété utile est « tout dépassement est refusé puis rejoué sans
  troncature », pas « aucun dépassement n'existe ».
- Les high-waters coquille et intérieur sont relevés dans `neighbour_along`, pas
  à chaque tentative `admit(v)` ou `admit(w)`. Sous gdb, le cube atteint `admit`
  avec coquille 8 et intérieur 0, mais laisse ces compteurs locaux à zéro. Le
  maximum global non nul masque cette sous-mesure.
- `touched_high_water` est relevé après une branche de retour; sur le cube, un
  vecteur a été rempli puis le compteur reste zéro. L'affirmation « à chaque
  écriture » n'est donc pas satisfaite.
- L'interface API refuse bien `s_max>32` avant la soustraction signée, mais le
  parseur CLI accepte encore jusqu'à 4096. Avec `--clouds 0`, une demande
  `--smax 33` peut même sortir avec succès parce que seules les fixtures fixes
  sont exécutées. Le CLI doit partager la borne et posséder une fixture négative.

Deux risques API secondaires restent à fermer : `backward_pair_admissible`
calcule `-forward` avant de valider le domaine, donc `INT_MIN` est un overflow
signé; et les tailles/indices publics supposent que `admit` n'est jamais
contourné.

La règle live qui traite `interior_high_water==0` comme un compteur mort peut
également refuser un jeu parfaitement légitime dont tous les sommets admis ont
un intérieur vide. Il faut relever les tailles exactement à chaque tentative
d'admission et compter les échantillons; une sentinelle ou un compteur
d'événements sépare « zéro légitime » de « instrumentation jamais exécutée ».

## Résultats reproductibles du snapshot

### Cinq portes usuelles Release

Commande :

```sh
ctest --test-dir /tmp/mhgp3v-live-v7HQX0 --output-on-failure -R '^mhgp3v_flats_(fixtures|generic|indexed_tree|degenerate|cospherical)$' -j2
```

Résultat : **5/5**, 78,07 s de temps mur, 4 990 cas et zéro désaccord sur les
distributions de ces portes.

| porte | cas | high-water coquille / fermeture / intérieur | device admis / couples | owner émises |
| --- | ---: | ---: | ---: | ---: |
| fixtures | 214 | 5 / 4 / 5 | 1 578 / 13 244 | 2 405 |
| generic | 1 187 | 8 / 5 / 7 | 110 873 / 895 952 | 63 757 |
| indexed tree | 221 | 5 / 4 / 5 | 3 062 / 25 116 | 3 027 |
| degenerate | 1 294 | 8 / 6 / 8 | 111 170 / 937 586 | 70 874 |
| cospherical | 2 074 | 8 / 6 / 8 | 101 877 / 831 118 | 80 410 |

Toutes rendent `refuses par capacite=0` et un high-water `emitted=0` dans le
quadrant owner+index+navigable. Elles restent insuffisantes puisque le probe u16
ci-dessus produit **216 cas, un désaccord** sur le même binaire.

### Noyau F0

L'exécution normale et l'exécution `python3 -O` impriment encore toutes deux :

```text
exhaustive=2168 accepted=1703 rejected=465
targeted=11 invalid=8 permutations=11 arity11=PASS
mutations=10 rollback_faults=5 allocator_mutant_killed=True
Gate_D_F0_kernel=PASS
```

Une sonde `RawBatch` du carré tout `N_a` rend pourtant `error` dans les deux
classificateurs, tandis que `regular_smuggling` rend `ok`. Le texte `PASS` ne
ferme donc ni la sémantique F0 ni l'indépendance de sa vérité.

## Ordre de fermeture recommandé à Claude

1. Corriger la réduction `i128` vers signe et graver les deux frontières avec
   UBSan, `-Wconversion`, unicité owner et mutant de troncature.
2. Aligner F0 sur la naissance générale tout `N_a`; déplacer la régularité dans
   un validateur par record authentifiant aussi miniboule et census terminal.
3. Rendre l'oracle F0 indépendant depuis le `RawBatch` et remplacer les
   obligations contractuelles basées sur `assert`.
4. Tester directement l'identité owner signée et compléter le contrat de
   comparaison du catalogue.
5. Intégrer le réducteur linéaire des rayons, puis seulement mesurer de grandes
   coquilles et traiter le terme d'arité trois.
6. Rendre la porte device non vacuable, exercer les deux refus de capacité et
   le repli hôte, puis compiler et exécuter un vrai kernel v3.
7. Construire le census terminal exact 4D avec statuts cappés et oracle borné.

Tant que les points 1 à 4 ne sont pas fermés, les résultats positifs restent
des validations ciblées et non une autorisation de promotion.

GCP non utilisé.

# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

`HEAD` audité : `1b4750c72bef65cd6515ad877bfb3eab1784d0c4`.

Le worktree est concurrent et non propre. La baseline self-join q2 appartient
au commit `8a39c53f41c1964b12d38b0129d7e8a0a5cc94e7`; le delta courant modifie q2,
le sidecar, son pipeline et CMake. Il n'est pas confondu avec `HEAD`.

Empreinte du rejeu ciblé courant :

| objet | SHA-256 |
| --- | --- |
| `prototype/pair_selfjoin_probe.cpp` | `7eaa9c34c684ad9d9dc27e0082726efb47a04274a2d9c5a2c55e30a49963dd38` |
| `CMakeLists.txt` | `e28a8977c36d89324780b321b27ce0afed188d51c9eb1b374bc5b66bb60ebe64` |
| binaire Release q2 local | `a447be4fc02dcd53da460eaed257cfd74b505eae83e862c4eb335032def668fe` |
| `prototype/validated_hybrid_sidecar.hpp` | `89bd0ff17cf900550fc5493f1651290c3b92d9f4db1c3ec1a4403e7b92956242` |
| `prototype/sidecar_factory_gate.cpp` | `79b216a41408e6c132e506ab04f4aa51060e2f2ba19107450510a2a618047899` |
| binaire Release sidecar local | `731aa86d7eb7f6b961347b12248d68fd46e448ed138f6a5f5491049baa26866d` |

Une modification de l'un de ces objets invalide le rejeu correspondant. Ce
fichier est l'unique autorité mutable du statut live; les audits épinglés
restent autorités de leurs seuls snapshots.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, la cible principale
est un p95 `warm_e2e<100 ms`; `warm_e2e<1 s` est la cible secondaire et le
jalon immédiat demandé. Aucun backend public exact n'est qualifié.

Le delta q2 ferme localement sa porte de correction logicielle sur l'empreinte
ci-dessus : ledger paire par paire, inclusion non compensable, fixtures,
mutants, budgets et planchers passent `22/22` CTests ciblés. Il ne ferme pas la
porte de parcimonie. La sortie précoce ne retire que 0,23 à 0,39 % des visites
de nœuds mesurées; le parcours continue à rescanner presque tout l'arbre témoin
pour chaque état.

Le verrou produit demeure la source géométrique exacte et parcimonieuse. q2 ne
produit encore ni census fermé ni `BallActivation`; q3/q4 n'ont pas de source
mesurée à 50 k; resolver et fold bout en bout restent ouverts. Les corrections
du sidecar sécurisent un oracle borné et n'ont aucun effet sur ces masses.

## Contrats de sortie

Deux contrats restent strictement distincts :

1. Gamma/v2 exhaustif, avec facettes, cofaces, incidences silencieuses et
   applications verticales;
2. `hgp_reduced_normalized_h0_v3`, candidat horizontal avec niveaux,
   composantes et unions exactes des `PointId`, après quotient certifié des
   blocs H0 inertes.

Le second n'est pas reçu comme produit. Les verticales sont hors de ce contrat
horizontal et constituent une porte séparée. Une tombstone H0 ne prouve jamais
l'absence d'un support ou d'une incidence Gamma.

## État des cinq portes du delta

| porte | état observé | verdict |
| --- | --- | --- |
| forge `bit_cast`, `[r1,r2,r1]`, ancien `INT128_MIN` | constructeur privé/non trivial, tri `(centre,niveau,index)`, pgcd sur magnitudes et garde i128 | les trois contre-exemples initiaux sont fermés; le domaine hostile complet reste ouvert sur `P3` et `Sphere.base` |
| support canonique et SHA-256 | reconstruction canonique, SHA-256 little-endian taggé, framing et digest étendu | non reçu : evidence et fold consomment encore le support déclaré; reçu producteur incomplet |
| pipeline hybride borné | chemin validé explicitement classé oracle `n<=32` | classement reçu, aucune lane 50 k |
| différentiel q2 non compensable | fate ledger, quatre fixtures, six injections, duplication compensée, budgets et planchers | reçu localement sur l'empreinte ci-dessus, `22/22`; pas encore un snapshot committé |
| compteurs q2 | quatre familles à 2 400 et terrain à 5 000 rejoués hors bruteforce | mesure locale exploitable pour les masses; route actuelle non admise sous la seconde |

## Self-join q2 : exactitude

L'audit de la baseline est
[`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md).
Le prune local est exact : le sup AABB de
`(w-x) dot (w-y)` est calculé sur les extrêmes, les contacts descendent et dix
`PointId` distincts hors extrémités donnent `p+q>=12`. Cela autorise seulement
une tombstone q2 du quotient horizontal à `K=10`.

Le commit `8a39c53` vérifiait seulement des comptes compensables. Le delta
courant tient un sort triangulaire par paire à petit `n`, refuse omission et
double affectation, et vérifie que toute paire avec moins de dix témoins arrive
en microtuile. Les gates couvrent :

- terrain et multi-écho avec balayage indépendant;
- contact, exactement neuf témoins, coordonnées dupliquées et portée q2/q3;
- omissions d'un enfant croisé, de `R,R` et de la dernière microtuile;
- seuil 9, contact compté comme strict et duplication compensée;
- `max_states`, sa frontière exacte moins un, et planchers anti-vacuité;
- codes de succès et de refus exacts.

Commande et résultat après régénération CMake :

```bash
ctest --test-dir build/v3 --output-on-failure -R '^(mhgp3v_pair_selfjoin|mhgp3v_sidecar)'
```

Résultat global : `28/28`; q2 `22/22`, sidecar `6/6`.

Le nom `P1a` persiste dans des commentaires source/CMake alors qu'il désignait
l'ancien center-cover. La nomenclature non ambiguë à conserver est
« self-join q2 ».

## Self-join q2 : coût après sortie précoce

Mesures Release, un thread, seed `20260810`, feuilles de taille 8, sans
`--verify-bruteforce`. Les compteurs sont les données utiles; le chrono est une
phase locale unique, sans chauffe, répétition ni p95, et n'est pas
`warm_e2e`.

| famille, n | états | visites nœuds | tests ponctuels | paires terminales | phase locale |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain, 2 400 | 24 186 | 20 855 916 | 48 301 083 | 144 986 | 0,789 s |
| scanline simple, 2 400 | 20 600 | 17 667 775 | 40 919 884 | 126 516 | 0,727 s |
| multi-écho, 2 400 | 32 984 | 29 024 422 | 67 314 546 | 204 657 | 1,056 s |
| uniforme, 2 400 | 67 668 | 60 454 402 | 140 290 100 | 407 313 | 2,419 s |
| terrain, 5 000 | 52 198 | 89 691 896 | 217 489 879 | 323 749 | 3,756 s |

Les états et masses terminales sont inchangés, comme attendu. Par rapport au
parcours committé, la sortie précoce réduit les visites de seulement 0,380 %,
0,352 %, 0,379 % et 0,386 % sur les quatre familles à 2 400, puis 0,226 % sur
terrain à 5 000. Le problème n'était donc pas principalement le travail après
le dixième témoin : le dixième témoin est acquis tard.

À 2 400 points, chaque état visite encore environ 858 à 893 des 1 023 nœuds de
l'arbre, soit 84 à 87 %. À 5 000, terrain visite environ 1 718 des 2 047 nœuds
par état, soit 84 %. Les 217 millions de tests ponctuels à 5 000 confirment que
la pile fixe seule ne changera pas l'ordre de grandeur.

Le pire cas reste `Theta(n^2)` états ou résidu fois `Theta(n)` recherche et
census, donc `Theta(n^3)`. Le cap `--max-states` classe ce binaire comme
falsificateur censuré, même lorsqu'il n'est pas atteint. Il ne qualifie jamais
un temps produit.

### Prochaine expérience q2

1. Ajouter l'infimum AABB exact `L4` décrit dans l'audit épinglé : `L4>=0`
   retire immédiatement un nœud sans témoin strict, tandis que `U4<0` crédite
   le nœud entier. Les deux bornes sont monotones sous raffinement; les
   contacts restent au census fermé.
2. Transmettre au plus neuf identifiants déjà stricts du parent. Ils restent
   stricts et hors extrémités sous restriction; un scalaire sans IDs n'est pas
   un reçu.
3. Réutiliser la pile et chercher seulement les témoins manquants. Mesurer
   séparément le gain de visites et celui d'allocation.
4. Publier sorties précoces, IDs hérités/nouveaux, splits par type, tests
   ponctuels, octets, allocations, high-water et travail terminal.
5. Comparer sur les mêmes entrées à Morton/LBVH + coupe stricte Yao48 +
   classifieur terminal exact et census fermé, architecture déjà décrite dans
   [`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md).

Le self-join reste un oracle indépendant ou un second prune. Il ne devient pas
la source q2 produit sur la seule baisse du pourcentage de microtuiles.

## Sidecar : blocages restants

Les `6/6` CTests ciblés valident les fixtures présentes; ils ne couvrent pas les
frontières suivantes :

1. la garde ABI borne `nx,ny,nz,den`, mais pas chaque coordonnée `P3` ni
   `Sphere.base`. Une coordonnée `LLONG_MIN/MAX` peut atteindre une soustraction
   signée dans `sphere_side`; un `base` hostile atteint `base*den`. Le profil
   exige `[0,65535]` avant toute géométrie ou clé, avec fixtures UBSan;
2. `GeneratorCertificate` publie le support canonique reconstruit, mais les
   `RemovalEvidence` bouclent sur `sphere.support` et le fold validé consomme le
   catalogue brut. Il faut rejeter une déclaration non canonique, ou normaliser
   le catalogue possédé et recalculer evidence, digests et fold sur ce seul
   snapshot;
3. le reçu lie points et catalogue, mais pas encore contrat/SHA du producteur,
   profil, schéma et identité terminale complète des tâches. Un digest lie des
   données; il ne prouve pas leur complétude;
4. le déplacement du reçu copie son état sans invalider la source, contrairement
   au commentaire « consommé »;
5. le self-test SHA est appelé par la gate, pas par la factory contrairement au
   commentaire du header.

Le pipeline exige `smax>=n` et refuse `smax>32`; il reconstruit le catalogue et
effectue un census `O(G*n)`. Après correction, il reste un oracle CPU borné à
`n<=32`, jamais la source chaude 50 k.

## Source q3/q4

Deux certificats mathématiques complémentaires sont reçus :

- le cœur universel de Jung, avec prédicats polynomiaux exacts, certifie neuf
  témoins q3 ou huit q4 pour toutes les sphères admissibles d'une vraie arête
  diamètre;
- la profondeur fermée de demi-boule borne toute sphère passant par une paire,
  sans hypothèse de diamètre, et s'applique d'abord comme filtre terminal.

Les preuves et corrections sont dans
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) et
[`REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md`](REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md).
q2 utilise le total diamétral; q3/q4 peuvent utiliser la profondeur. Leurs
résiduels sont incomparables.

Une machine de blocs peut partager le LBVH et la partition des paires, mais
q2/q3/q4 gardent trois sorts et trois ledgers indépendants. Le falsificateur
mesure `cœur seul`, `profondeur seule` et `combiné`. Le certificat sectoriel de
bloc pour la profondeur n'est pas reçu; toute ambiguïté descend.

Après les ancres, q3 produit au plus un centre par troisième point dans
`h^2<=D^2/12`. q4 construit directement les niveaux de profondeur au plus 7
dans `h^2<=D^2/8`, sans former toutes les intersections. La couverture est
conditionnellement exacte; la parcimonie globale des ancres et de la somme des
arrangements reste à mesurer.

## Reçus G4

Les sorties mass-only reçues sont
[`cell_50k_raw.txt`](../receipts/g4_massonly_20260811/cell_50k_raw.txt) et
[`mask_scale_raw.txt`](../receipts/g4_massonly_20260811/mask_scale_raw.txt).
Après prune d'axe, elles conservent jusqu'à 2,86 milliards de tuples q2,
131,76 milliards q3 et 9,97 billions q4. Aucun tuple, census, fold ou payload
n'a été formé; les temps count-only ne sont pas des temps de source.

Le pire cas exact peut produire `Omega(n^2)` paires q2. Le SLO est donc
sensible au profil et à la sortie, avec refus atomique; il n'est pas une
promesse universelle sur toute entrée u16.

## Ordre des prochaines portes

1. Stabiliser le delta q2 et conserver son reçu `22/22`; remplacer la
   nomenclature `P1a` dans les commentaires source.
2. Implémenter l'héritage de neuf IDs ou basculer vers Yao48/LBVH; rejouer les
   compteurs à `12 500/25 000/50 000` avant tout port CUDA.
3. Fermer les coordonnées/base hostiles et l'unicité du support consommé par le
   sidecar; compléter le reçu producteur et recevoir seulement l'oracle `n<=32`.
4. Prototyper cœur de Jung et profondeur fermée q3/q4, avec fate ledger et
   oracle exhaustif borné, sans boucle ancre--nuage matérialisée.
5. Recevoir census fermé, `BallActivation`, resolver latent et fold horizontal
   contre Gamma exhaustif à petit `n`.
6. Porter sur G4 uniquement les primitives admises, puis mesurer build, source,
   census, resolver, fold, payload et p95 `warm_e2e` à 12,5 k, 25 k et 50 k.

Une insuffisance de ressource refuse atomiquement. Aucun tableau global de
tuples, paires, cellules, faces, cofaces ou incidences n'entre dans le chemin
produit.

## Validation documentaire

Les liens locaux des documents live et les règles LaTeX v3 ont été contrôlés
séparément. `python tools/check_docs.py` passe mais ne parcourt pas
`morsehgp3D_v3/`; `python tools/check_implementation_status.py` passe et aucun
statut formel n'a été modifié.

GCP non utilisé.

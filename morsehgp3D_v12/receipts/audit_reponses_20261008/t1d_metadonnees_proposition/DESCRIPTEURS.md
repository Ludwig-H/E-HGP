# Raccord précis des deux conteneurs propriétaires

Pseudocode, pas une implémentation C++ qualifiée. Il conserve la source unique, l'ordre des lots, leurs Buffer de
contenu et les callbacks. Il ne propose ni second catalogue ni deuxième parcours géométrique.

## 1. Propriétaire d'un tableau d'objets compté

Besoin restreint : T vaut `Buffer<LevelWords>` ou `Chunk`, tous deux sans copie, à construction par défaut,
déplacement et destruction sans exception. Ne pas utiliser `BudgetReservation` pour un `std::vector::reserve`
supposé de taille exacte : sa capacité effective et la coexistence des allocations doivent être comptées AVANT
allocation. Ne pas tenir une réservation fantôme après une allocation non contrôlée.

```text
ObjetsComptes<T> possède : Buffer d'octets de stockage, T* objets, capacité construite C.
Contraintes : alignof(T) <= alignement garanti par Buffer ; sizeof(T) connu ;
             construction par défaut, déplacement et destruction sans exception.

allouer_vide(C, budget), sur propriétaire neuf :
  garder C <= min(Buffer<octet>::kMaxCount, SIZE_MAX) / sizeof(T) AVANT tout produit ;
  réserver/allouer C*sizeof(T) via Buffer (zéro : propriétaire vide) ;
  établir dans ce stockage un VRAI tableau C++ de C objets T vides ;
  seulement ensuite publier objets et C.

reset/destruction :
  détruire les C objets construits, y compris leurs Buffer de contenu ;
  puis, et seulement puis, rendre le Buffer de stockage au budget/cache ;
  mettre pointeur et capacité à zéro.

span() est celui du tableau T réellement construit ;
un reinterpret_cast suivi d'un span sur des objets jamais construits est insuffisant.
```

La mise en place doit établir la durée de vie du tableau, son alignement et l'absence d'un second stockage caché
par l'opération choisie. La capacité n'est pas le nombre d'entrées publiées. Si l'implémentation choisie ne garantit
pas les opérations sans exception, ajouter destruction du préfixe construit et rollback avant de l'utiliser.
Les déplacements échangent ou déplacent la propriété ; ils ne copient jamais les contenus des Buffer.

## 2. Mots des niveaux : taille S exacte, aucune croissance

Dans `slices_run`, `plan.count` est connu AVANT la boucle de tranches. Remplacer `SliceWords::words` par
`ObjetsComptes<Buffer<LevelWords>>`, allouer exactement S descripteurs vides à ce moment, avant les grosses sorties.
Transmettre `words[j]` à `slice_finish` puis `slice_take` ; ce dernier alloue déjà le contenu exact `distinct` et
le remplit. Supprimer `keep_words` et le `push_back`. `materialize_levels` reçoit un span mutable de S descripteurs
et conserve sa boucle actuelle, ses contrôles de somme et le `reset()` après conversion de chaque lot.

Après succès de j tranches, les descripteurs `[0,j)` possèdent exactement leurs mots ; les suivants sont vides.
Un refus, y compris pendant la conversion, détruit tous les descripteurs encore propriétaires et le stockage.
Mémoire logique des descripteurs : `S*sizeof(Buffer<LevelWords>)`, ajoutée aux mots eux-mêmes, qui étaient déjà
comptés. Ne pas préallouer B niveaux complets à la place : B peut excéder largement le nombre de niveaux distincts.

## 3. Arène des lots : croissance avant le rapatriement

Le nombre de lots n'est pas connu pendant le parcours. Un petit propriétaire local à LeafHook garde
`ObjetsComptes<Chunk> stockage`, la capacité C et le nombre L de lots écrits. `[0,L)` possède les lots ; `[L,C)`
contient des Chunk vides.

```text
reserve_un_lot(budget) :
  si L < C : succès ;
  sinon garder L+1 et 2*C contre débordement et limite d'éléments ;
  choisir C' = 1 si C=0, sinon min(2*C, capacité maximale) ; refuser si C' <= C ;
  allouer un temporaire de C' Chunk vides (ancien stockage et contenus toujours comptés) ;
  déplacer sans exception les L Chunk vers les L premières cases du temporaire ;
  échanger les propriétaires ; détruire/rendre l'ancien tableau, désormais vide de contenus.

take_arena() :
  si held=0 : succès ;
  reserve_un_lot AVANT les allocations des contenus et les téléchargements ;
  construire/remplir le Chunk temporaire comme actuellement ;
  le déplacer sans allocation dans stockage[L], puis incrémenter L ;
  seulement ensuite mettre held et held_incidences à zéro et incrémenter streamed.
```

Un refus de réservation laisse les anciens lots intacts ; un refus des contenus ou du téléchargement laisse le
lot courant non publié. Les contenus ne sont jamais dupliqués pendant le déplacement des descripteurs. La mémoire
logique de ces descripteurs est inférieure à `2*L*sizeof(Chunk)` après ajout et à `3*L*sizeof(Chunk)` au pic d'une
croissance, pour L≥2 (premier lot : un Chunk). Là encore, les blocs physiquement arrondis sont comptés par Buffer.

Raccord limité à l'arène de la voie appareil : `ArenaView` peut garder son span constant de Chunk. Pour le rendu
final de `SliceInput`, remplacer le pointeur concret `std::vector<Chunk>*` par un contexte et un callback de
libération : adaptateur `.clear()` pour l'arène CPU existante, adaptateur `.reset()` pour l'arène appareil comptée.
L'invocation reste EXACTEMENT après toutes les tranches et leurs vérifications ; aucun span n'est relu ensuite.
Laisser le callback nul aux portes qui ne cèdent pas leur arène. Cela évite de convertir toutes les arènes CPU dans
cette correction. Leurs métadonnées historiques restent une limite distincte, explicitement non fermée ici.

## 4. Invariants à contrôler avant publication

- Aucun descripteur propriétaire vivant ne survit au stockage qui le contient.
- Chaque Buffer de contenu a un seul propriétaire après déplacement, sans changement de ses indices ou valeurs.
- Toute coexistence ancien/nouveau stockage est réservée avant allocation ; `used` et `held` suivent le cache réel.
- Toute entrée lue est construite et, lorsqu'elle désigne un lot, a été entièrement publiée ; aucune lecture des
  cases libres de capacité.
- Refus mémoire : aucun catalogue partiel, destruction complète des temporaires, contexte réutilisable.
- Succès : même séquence des tranches, niveaux non réduits, rangs, CSR, table et comptes logiques.

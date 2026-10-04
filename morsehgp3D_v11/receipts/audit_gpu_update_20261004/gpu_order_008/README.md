# Réparation de cardinalité et ordre CUDA par taille — pin008

Sources Git `00800dd88d5d1368b1987e399a05e87a17f208ae`, quatre copies avant
lecture complète et modèle ; comparaison après contre les mêmes blobs.
Le Git stat/diff initial précède la copie, explicitement consigné dans BEFORE.
Aucun natif/CUDA, build, G4, donnée LiDAR ou produit modifié.

**Raccord favorable en source dans ce périmètre.**

- `sources/leaf_batch.cpp:72`, `leaf_batch_cuda.cu:247–248` remplacent la
  fausse borne feuilles≤sites par un plafond de ressource explicite2^40 ;
  CUDA ajoute count≤2^32. Le cas80feuilles/9sites franchit donc ces seules
  gardes : aucune nouvelle réussite native n'est revendiquée.
- `leaf_device_predicates.hpp:31–39` : local3979008<2^22, donc sommes
  host<2^62 et CUDA<2^54. La fin inclusive count=2^32 est compatible avec
  le dernier ordinal u32=2^32−1 ; aucune allocation géante dans la preuve.
- `leaf_batch_cuda.cu:152–162` : comptage décroissant de m, stable et
  permutation entière. Domaine interne certifié1≤m≤32, job/site/owner/T0
  valides comme dans la capsule géométrique77 ; pas un validateur de vues forgées.
- Count `:54–69` et fill `:80–90` convertissent thread→order[thread]→j.
  Statuts/comptes restent en j. Scan `:172–176` prefixe les tableaux dans
  l'ordre original ; FillSink débute en record_begin[j]/population_begin[j],
  vérifie les fins de j+1 et conserve exactement la disposition du lot.
  Réduction des seuls compteurs de feuilles résolues, padding horscount
  et barrières de bloc sont maintenus. Aucun nouveau défaut géométrique ou
  de transport établi sur ce pin.

Le tri ajoute deux buffers budgetés coexistants, hostorder et deviceorder,
soit8×count octets propres ; cette quantité n'est ni RSS ni coût du contexte.
Une frontière de bucket peut partager un warp avec deux tailles ; le modèle
le couvre sans lui attribuer de faute ni de gain/perte matériel.

**6 852 gardes stdlib**, 14lots bornés (0..257jobs) : tri source comparé au tri
stable indépendant, padding128, statut nonrésolu, réduction15champs, scan
original, sinks synthétiques sans recouvrement et sorties byteforbyte identiques
à l'ordre ordinal direct. Les payloads synthétiques ne certifient aucune boule.
Gardes scalaires autour2^32/2^40 sans grandes allocations. Normal/−O identiques.
`leaf_device.hpp` est identique77 (empreinte BEFORE), pas recopié ici.

**Dérive après étude.** AFTER compare séparément origin
`b74f9ea3a0b986f659d388b8140067b0b35be0c2` : des sources ont changé.
Ce commit ajoute notamment ScratchSink/stored/copy_scratch/WritePhase et de
nouvelles coexistences. La présente capsule ne les juge pas et ne leur
transfère pas ses résultats ; les copies008 et la chronologie sont conservées.

```sh
python3 -B -S check.py
python3 -O -B -S check.py
python3 -B -S verify.py
python3 -O -B -S verify.py
```

SHA256SUMS inventorie chaque fichier régulier, excepté lui-même. Sourcepins,
états auteur et originafter séparés. Port source favorable, qualification
native/G4 et efficacité encore distinctes.

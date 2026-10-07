# Gc : profils, mutants et traces TSan du hash par cellule

Relecture du 7 octobre 2026, CPU local, hors registre, `public_status=not_claimed`.
Aucun natif, CTest, build, CUDA ou GCP lancé par l'auditeur. Aucun brut recopié.

Les campagnes portent bien sur `passes.cpp` **761c2fb8dd2706be…**, avec le hash
par cellule ajouté à 23:14. L'archive Git locale immuable
**1b562defb3e77748693ea7cebecdfa1f3da72ac9** reconstitue exactement les 340 fichiers
du lanceur de mutants : SHA-256 **5203f5a50b5350ef74232e9e12df8d8021a1f8c3e196ab4101c3252be0b8df44**.
Le manifeste vaut **91643295ac4f1c16256845cfdfcb9013287826e3cd816585e9fad5bbce90fc5f**.
Les dépendances, objets et journaux des builds p24/p32/tsan2 pointent vers repo2,
avec compilation de `passes.cpp` postérieure au changement. Aucun transfert à
l'état de 23:12, à main ou à une future fusion CPU/CUDA.

Le prototype actif a changé à 23:52 pendant la clôture : table S*, juge de
déterminisme, fausse sonde, tests d'index et CMake. Le lecteur a refusé ce nouvel
arbre ; le commit immuable ci-dessus conserve les préimages exactes. **Ces
modifications ultérieures ne sont pas couvertes par les campagnes ici relues.**

| Campagne archivée | Ce qui est attesté |
| --- | --- |
| Profils 24 et 32 | Release, modules catalogue+tower, compilation sans avertissement ; code configure/build/CTest 0 ; 71/71 chacun, aucun test sélectionné sauté. Clôtures LastTest 23:33 et 23:42. |
| Mutants tower | Rapport code 0, témoin vert, 18/18 tués par code ; ni signal, ni délai, ni refus de construction. Rapport clos à 23:40. |
| TSan u21 | Build compilé et lié avec `-fsanitize=thread` ; huit sorties natives terminées entre 23:43 et 23:45, sans diagnostic TSan dans les fichiers conservés. Les commandes, codes externes, redirection stderr et `setarch -R` ne sont pas archivés. |

Les profils ont 78 portes configurées : les 7 exclues par `long` sont cinq portes
LiDAR et les deux campagnes natives de mutants. Les manifestes seuls passent en
normal/−O. Les portes d'échelle 8k/16k/32k jouent W1/W8, mêmes empreintes de
résolution ; leurs entrées restent `uniform=…,20261007,18`. Cela exerce des
**builds** u24/u32 sur ces coordonnées ; cela n'est ni un nouveau jeu occupant
toute la plage de ces profils, ni FULL TMVR, ni une mesure G4.

Le dernier mutant est causalement ciblé : il retire uniquement l'addition des
sites intérieurs dans l'empreinte G-L7 (`tests/mutants/tower.json:149-155`).
`weak_key_resolution` (`tests/tower/index_unit.cpp:147-189` au pin) reconstruit les
traces complètes, prépare un index de masque nul, puis rejoue G-L5 et la recherche
directe `find(F)` sans la file ni sa clé précalculée. Il compare chaque cible,
puis **tous les compteurs du travail**, après avoir recopié les cinq compteurs
purement structurels. Les planchers imposent des succès de première sonde, de
sonde après descente et des arrêts sur cellule. C'est un contrôle indépendant de
la construction de clé et de la file ; `resolve_part` reste partagé, ce n'est pas
un nouvel oracle géométrique indépendant.

Le rapport atteste que ce mutant échoue par code à cette porte. La suppression de
I peut produire des sondes manquées et davantage de descentes malgré des cibles
inchangées ; le rejeu du travail vise précisément cette faute. Le lanceur efface
les copies et n'archive pas le CHECK précis ayant échoué (`run_mutants.py:268-309`).
On ne prétend donc pas avoir relu une trace d'échec de `counters == want` qui n'est
pas conservée. W1=W8 seul ne distingue pas cette erreur déterministe.

TSan conserve six terminaux `mhgp12_test_ok` : cinq groupes d'index (28,63,50,4,5
contrôles) et le déterminisme W1/W8 sur 2 000 sites (62). Les deux sondes ont les
ordres 1..5 et `exit:ok` : u8000/W8 et ng00/W3, empreintes attendues. Leur
attribution au build instrumenté repose sur le contexte de campagne ; les logs
seuls ne préservent pas la commande. Aucune nouvelle qualification W48, GPU,
FULL ni absence universelle de course ne suit de ces résultats.

Les intitulés 23:50/23:55 du RAPPORT étaient futurs à la première lecture
(horloge 23:48:50). Les artefacts étaient pourtant déjà clos aux heures ci-dessus ;
les intitulés ne servent pas de preuve d'exécution.

Rejeu de lecture, le répertoire scratch externe devant rester disponible :

```sh
python3 check.py --scratch /chemin/v12_tour_Gc
python3 -O check.py --scratch /chemin/v12_tour_Gc
```

Le lecteur vérifie les 51 hashes d'artefacts avant/après, relit l'archive Git et
recalcule son hash global. Normal/−O rendent le même `results.json` ; aucun
exécutable moteur ni lanceur de campagne n'est appelé. Pins dans `capture.json`.

# Arène commune et prochain essai de moments

Audit en lecture seule du commit `33fcb53a0b749c7d7f16909ce8a3f0ef9160a37a`.
Le fichier `tower.cpp` n'a pas été modifié ou exécuté. Le contrôle C++ est
scalaire et virtuel : aucune arène, forêt ou liste massive n'est allouée.

**Garanties amont effectivement vérifiées.** Une coquille étendue de plus
de 24 sites est refusée (`tower.cpp:582`), comme plus de 20 000 parties
séparables (`:594`). Le nombre de représentants d'une jonction est le
nombre de morceaux locaux, donc ≤20 000 ; son cast `nreps` u32 est sûr.
Chaque morceau de catalogue a au plus 8192 boules : le compteur de
représentants par K reste ≤163 840 000, donc ses additions u32 sont sûres.
Chaque `ExtCell` correspond à une cellule (boule,K) de l'atlas :
`ext.size()≤atlas.cells`. Le refus ultérieur `atlas.cells<kNone` protège
ainsi le domaine du tableau de cellules avant leur consommation par la tour.
Les préfixes par K refusent `sr[k]≥kNone`, y compris au passage terminal.
Ces garanties ne sont pas des défauts à rouvrir.

**Décalage global distinct.** `rep_first=u32(ext_reps.size())` (`:1290`),
puis `rep_first+r` en u32 (`:1024`), indexent une arène commune à tous les K.
Après les gardes par ordre, on sait seulement que sa taille ne dépasse
la somme des `sr[k]` : jusqu'à douze fois une limite u32, et non une limite
u32 unique. Le nombre de cellules ne borne pas syntaxiquement le nombre
de représentants par cellule. Le contrôle virtuel construit un modèle
ball-major de 20 M boules, 58/74/92 représentants aux ordres 8/9/10 :
chaque total par K et tous les compteurs/atlas passent, l'arène atteint
4,48 milliards. Le dernier indice exact 4 479 999 999 devient 185 032 703.
Un second cas commence encore dans u32 et déborde par l'addition seule.

**Portée limitée du modèle.** Ces nombres ne constituent pas un catalogue
géométrique réalisable attesté. Il manque une réalisation complète qui
respecte aussi toutes les boules et cellules antérieures imposées par la
géométrie. Aucune trame ou tour exécutée n'est déclarée corrompue ici.
La borne sur l'arité des fusions finales de forêt ne s'applique pas
automatiquement : l'arène contient les morceaux locaux avant quotient
par composante de forêt. Aucune injection de leurs incidences vers des
cellules atlas distinctes n'est établie dans cette revue.

**Correction constructive peu coûteuse.** Garder les tableaux par K et les
IDs actuels, puis soit vérifier l'adresse finale consommable **avant** le
cast/insertion global, soit porter `rep_first` et l'addition vers u64.
[span_check.cpp](span_check.cpp) donne deux helpers purs de référence :
refus avant narrowing u32, ou span u64 dont `first/count/r` sont vérifiés
contre l'étendue du stockage. Les cas limites incluent un dernier indice
UINT32_MAX, fin vide et overflow u64. Les offsets purs n'ont pas la
sémantique sentinelle d'un NodeIdx. Une vue span se prépare en O(1), sans
recopie de Facet ; le budget mémoire/`vector::max_size` reste distinct.
Le helper n'est pas raccordé au produit.

Pour les moments, [MOMENTS_PROTOCOL.md](MOMENTS_PROTOCOL.md) décrit le
prochain essai sur les vraies feuilles, avant tout pari de performance.
Il élimine les ancres dans S fermé, impose un nombre fixe de groupes,
publie préparation et résidu total, garde q2 et le census complets.
Les petites fixtures 14/16 sites ne prouvent aucun gain de générateur.

Les sources, commandes, sorties normal/UBSan et jugement indépendant
Python sont archivés. `FIRST_RUN.json` conserve exactement le premier
passage réussi : son champ `ext_cells` nommait seulement les cellules de
jonction ; le schéma final distingue les 200 M cellules étendues des
60 M cellules étendues portant des représentants. Aucune nouvelle qualification géométrique, moteur
massif, FULL, GPU/GCP ou performance n'est acquise.

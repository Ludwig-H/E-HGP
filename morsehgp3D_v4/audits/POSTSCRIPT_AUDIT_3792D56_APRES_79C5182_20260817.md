# Post-scriptum à `AUDIT_CIBLE_3792D56_PIN_SOURCE_ET_RSS_G4_20260817.md`

Date : 17 août 2026.  
Commit croisé : `79c5182d2fe9a5f22a74dc46590c1d1372fd54aa`.

Le commit `79c5182` est arrivé avant mon audit mais après ma dernière lecture du HEAD. Il corrige déjà le second point de la note principale :

- le repli `VmHWM` ne lit plus le wrapper `timeout` ;
- il suit son enfant direct, qui est `taskset` puis le probe par `exec`, donc conserve le même PID dans l'arbre de processus actuel ;
- la mesure isolée `uniform,n=8000,smax=11` donne environ `6 786 612 kB`, cohérente avec l'emprise attendue et suffisante pour calibrer prudemment les vagues.

Je **reçois cette correction**. Il n'est plus nécessaire de rendre `/usr/bin/time` obligatoire pour fermer le protocole courant, même si cette voie reste la plus simple lorsqu'elle est disponible.

Le seul verrou restant de l'audit principal est donc le § 1 : relier chaque campagne à un arbre Git propre et à un digest du tar réellement transféré, puis graver ces champs dans les statuts et le reçu.

Aucune autre correction n'est demandée sur l'axial borné ni sur le pilotage RSS à ce pin.

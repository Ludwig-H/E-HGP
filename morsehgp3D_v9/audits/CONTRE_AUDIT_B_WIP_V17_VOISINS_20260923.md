# Contre-audit B — raccord WIP v17 du certificat q3/q4 local

23 septembre 2026, lecture à 11 h 52 UTC du worktree mutable du
développeur, base publiée `ed11c6c3` puis audits sur `main` ; **aucun
reçu v17** ni mesure G4 ne découle de cette note. Pour fixer la lecture,
SHA-256 de `src/gen/pipeline/wspd_q34.cpp` :
`b2a34cf5aac48143533c81bc38df55eb2a87c2442500b9db86333c93bf740fb4`.
Ne pas attribuer ces observations à la future version publiée si la
source change.

## Ce qui est sain

`Q34NearSites::build` prépare actuellement 16 autres sites par sommet,
par distance entière exacte et ID en cas d'égalité. Les fils écrivent
des lignes disjointes ; l'index est partagé en lecture ; sur la cible
64 bits, les distances et indices calculés restent dans leur domaine.
`near_certificate` trie et dédoublonne les IDs des deux listes avant
`load_sites`. Le nuage du préparateur a des positions uniques ; les
deux extrémités éventuellement reprises donnent des formes nulles.
Avec des sites réellement **distincts du nuage**, la fermeture est
monotone : T intérieurs stricts certifiés dans un sous-ensemble sont
aussi T intérieurs du nuage entier. Comme l'explique la
[note mathématique A](CERTIFICAT_Q34_SOUS_ENSEMBLES_LOCAUX_20260923.md),
des k-NN globaux **exacts ne sont pas nécessaires à la sûreté** ; ils
ne sont ici qu'une heuristique coûteuse de choix des témoins. L'échec
du certificat laisse la voie exacte intacte. Cela ne prouve toujours
pas la complétude du générateur entier.

## Trois points à traiter avant une capture v17

1. Avec les options de chaîne par défaut, `q34_witness_cache=true` et
   `q34_near_prover=true`. Pourtant `wspd_q34.cpp:542` contient
   `if (open != 0 && near_ && false)` : la preuve avant la recherche du
   filtre est **désactivée**. La branche cache/near mixte signalée par B
   ne perd donc plus ses comptes sur ce chemin, mais le défaut est
   contourné plutôt que corrigé. Le certificat ne tourne qu'après le
   filtre, lignes 578–583 ; les commentaires/options qui promettent
   l'avant-filtre ne correspondent plus au programme exécuté.
2. Sans cache, lignes 565–569 puis 578–582, la même arête peut appeler
   `near_certificate` deux fois. Si le premier essai ne prouve aucune
   voie et le filtre conserve le même masque, le second recharge les
   mêmes formes et parcourt le même domaine : il ne peut gagner aucun
   certificat. Si le filtre retire une voie, la seconde tentative sur
   un masque réduit peut avoir un coût ou résultat différent ; ne pas
   supprimer aveuglément ce cas. Éviter au minimum le rejeu lorsque le
   masque est inchangé et publier le nombre réel de charges/formes.
3. `Q34DeadLaneProver::load_sites` est une API publique qui exige des
   sites distincts **seulement en commentaire**. Son appel interne
   satisfait la précondition, mais un appel direct avec un doublon
   peut créditer deux fois le même site et donner une fausse fermeture.
   Rendre l'unicité vérifiable (IDs du propriétaire ou refus explicite)
   et ajouter un test direct de doublon.

Le diff du gate q34 ne fait à cette lecture que porter la taille de
`WspdQ34Work` de 476 à 492 octets ; aucune fixture `near_sites>0`
ciblant cache/near mixte, cache OFF, W1/W4 ou coquille n'a été trouvée.
Des tests de chaîne peuvent parcourir indirectement le nouveau défaut
ON, sans remplacer ces portes causales.

## Mesure et architecture

La table exacte paie n requêtes spatiales avant les jobs q3/q4, réserve
au moins `n × 16 × 4 + n` octets (environ **2,42 Gio pour 40 M sites**),
et ses visites peuvent être quadratiques au pire. La chaîne exporte
les visites de nœuds, mais pas encore les tests de points, la durée
propre ni les octets de cette construction. Le certificat intervient
après l'expansion A×B : il ne réduit pas `expanded_pairs` et ne déplace
donc pas à lui seul ce verrou. Les formes de cœur de la
[trame brute auditée](CONTRE_AUDIT_B_LIDAR_BRUT_PHYSIQUE_20260923.md)
croissent fortement ; juger les formes et covers **évités**, nets du
coût des listes et des essais, sur les mêmes octets ON/OFF. La piste
mathématiquement plus légère est une sélection bornée de sites distincts
et réutilisables, éventuellement déclenchée après les filtres bon marché,
toujours avec repli exact. Elle reste à concevoir et mesurer.

Porte minimale : mixed-lane cache q3/near q4 et inverse, cache OFF avec
masque inchangé/changé, W1/W4 et multi-CPU, comparaisons des flux et
sorties FULL identiques, validation de toutes les identités de masse,
coûts de construction/proof/cœur/aval et RSS. Puis trames LiDAR entières
brutes et sans sol, plusieurs séquences, K5/K10 et s8/10/12. Aucun
contrat 1 s/GPU/sous-quadratique n'est qualifié par ce WIP.

## Mise à jour du WIP à 11 h 55 UTC

Le développeur a déjà remplacé la source épinglée ci-dessus : SHA-256
actuel de `wspd_q34.cpp` :
`504fbf40747b12054e5d13d92099a4693c07c8ec1040460ee4b01`.
La prépreuve est rétablie **seulement si le cache appartient déjà à a** ;
le retour anticipé compte désormais les voies fermées par cache dans
`witness.pair_q3_pairs`/`pair_q4_pairs`, et le second appel après filtre
est retiré. Le cas de ledger mixte paraît donc réconcilié en lecture
statique ; les points 1–2 décrivent **l'état antérieur**, pas le code
de 11 h 55. Aucune porte `near_sites>0` ciblée ou reçu v17 n'est
encore publié ; l'API `load_sites` et le coût total restent ouverts.

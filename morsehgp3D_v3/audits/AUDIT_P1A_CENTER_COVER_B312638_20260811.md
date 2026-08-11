# Audit du falsificateur P1a center-cover au snapshot `b312638`

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le théorème de prune q4-only au seuil huit est exact sur le domaine u16
déclaré. Le probe CPU est un bon falsificateur borné : ses ledgers ferment,
son oracle ne trouve aucun faux prune dans les campagnes exercées et ses
mutants actuels meurent. Il ne satisfait cependant pas encore le contrat fort
de la note, et son ordonnance est **NO-GO avant G4** : la recherche témoin
repart de la racine pour chaque bloc, les bornes dirigées `L/U` sanctionnées
manquent et le travail observé est pratiquement quadratique dès 2 000--4 000
points.

Ce verdict refuse le port littéral du probe courant. Il ne réfute pas le
certificat P1a ni une nouvelle ordonnance persistante munie des bornes `L/U`.
P1a reste `q4 mass-only` : il n'émet ni ancre, ni support, ni payload produit et
ne qualifie aucun SLO.

## Pincement et tests

| objet | valeur |
| --- | --- |
| `HEAD` | `b312638c19e1a56ccf11cf72065c8f398f4abc7a`, commit `deliver the P1a center-cover mass falsifier (q4, seuil huit)` |
| `CMakeLists.txt` | `34538222a1e48bfd9109c448fa3adb07f2a8ea3d383b0bd4f55d7cd7b81b8090` |
| `prototype/center_cover_mass_probe.cpp` | `fc4001b3a198ae9c095c0c563cc9500357b9b5e2fe20a8678f88a117225aada9` |
| binaire Release | `9c130163a92a243c30f25157f5a817fa734b7b66fa47ec84d477bbe54155fbab` |

Les vérifications locales suivantes passent :

- `34/34` CTests `^mhgp3v_p1a_` en Release;
- `96/96` exécutions supplémentaires à `n=24` : quatre familles, huit
  graines, `leaf_size` 2/4/8, oracle et permutation actifs;
- `34/34` mêmes CTests dans un build temporaire
  `-fsanitize=address,undefined -fno-omit-frame-pointer`, avec
  `ASAN_OPTIONS=detect_leaks=1` et `UBSAN_OPTIONS=halt_on_error=1`.

La cible CMake permanente ne porte néanmoins aucun sanitizer. Ces verts
qualifient seulement les propriétés exercées; ils ne prouvent ni le profil
distinct, ni une complexité, ni CUDA/G4, ni 50 k.

## Propriétés mathématiques vérifiées

Pour un bloc croisé de plages disjointes `A,B`, le code calcule exactement
`X=maxdist²(A,B)`, le rayon entier extérieur `R_t`, le pavé `T_0` et ses 64
patches rationnels à l'échelle quatre. Les rejets médiateur, Jung q4 et boîte
des milieux emploient les facteurs entiers corrects `16`, `6X` et `2X`; toute
égalité reste fail-open.

Pour un coin `C/4`, un côté d'extrémités `S` et un témoin `z`, le test effectif
est la comparaison stricte à l'échelle seize :

$$F(C,z,S)=\sum_d\left((Q_d-C_d)^2-(4z_d-C_d)^2\right)>0,$$

avec `Q` obtenu par clip entier dans `4S`. Après minimisation sur la boîte de
`S`, la marge est concave en `C`; ses huit coins suffisent. Pour un nœud
témoin, le maximum de distance sur son AABB donne une condition suffisante
pour toutes ses feuilles. Les nœuds acceptés ne sont pas descendus pour le
patch concerné, donc les plages créditées forment une antichaîne. Les plages
`A` et `B` sont exclues et huit `PointId` distincts ainsi certifiés rendent
inerte tout support q4 propre positif dont l'arête maximale canonique appartient
au bloc.

Le juge déterminantal borné reconstruit les sphères et la positivité propre en
`i128`, puis refuse qu'une arête canonique d'un q4 non inerte appartienne à la
masse prunée. Aucun contre-exemple au théorème n'a été trouvé.

## Trous de réception

1. La tâche ne porte que les deux indices de nœuds. Aucun identifiant canonique
   de chemin n'interdit une omission compensée par un double commit hors du
   juge borné. Le mutant `terminal-compensated` altère les reçus après le run,
   pas la machine de tâches. Sur quatre points, omettre `{0,1}` et compter deux
   fois `{0,2}` conserve la masse scalaire six.
2. La bijection du juge ne rejette pas `id_a==id_b` avant son calcul d'index.
   Pour `n=4`, `pair_index(1,1)=2`, qui est aussi l'index de `{0,3}`. Un faux
   sort diagonal peut donc prendre la place d'une vraie paire sans être détecté
   par cette structure seule.
3. `BlockReceipt.radius_w` et `PatchCredit.node_begin/node_end` sont écrits mais
   jamais relus. Remplacer `R_w=2` par zéro ou une plage par `[-7,999]` reste
   accepté si les huit identifiants ponctuels passent encore. Le juge ne
   reconstruit donc pas l'antichaîne annoncée.
4. Pour un patch annoncé infaisable, le juge accepte l'OR de tous les motifs au
   lieu de rejouer le motif étiqueté. Avec `A={(0,0,0)}`, `B={(2,0,0)}` et le
   point de patch `c=(1,1,0)`, le motif médiateur est faux tandis que Jung et
   milieu sont vrais; l'étiquette médiateur fautive passerait.
5. La permutation ne relance ni les reçus ni l'oracle; elle compare seulement
   les masses agrégées. Le probe n'effectue aucun preflight explicite des
   positions 3D deux à deux distinctes avant la construction du LBVH.
6. La note promet encore des fixtures non scellées : centre strictement dans le
   patch omis, frontière de patch, q4 non inerte dont l'arête propriétaire tombe
   dans un bloc tenté, contraste `5H/8`, et rejet des positions dupliquées. La
   fixture `u16-extremes` ne contrôle que la fermeture du ledger.
7. La provenance imprimée omet `max_states`, oracle, permutation, source/ELF et
   `slo_eligible=false`. Les vecteurs de pile peuvent réallouer dans la phase
   chronométrée; aucun compteur d'allocation, octet physique ou RSS n'est reçu.

Ces défauts portent sur l'authentification et la qualification du probe. Ils ne
constituent pas des contre-exemples à la condition mathématique des huit
témoins rejoués.

## Falsification de l'ordonnance courante

Les deux commandes suivantes ont été exécutées sur le même ELF :

```bash
timeout 900 build/v3/mhgp3v_center_cover_mass_probe --points 2000 --seed 20260811 --family terrain --leaf-size 8 --microtile 64 --max-states 900000000
timeout 900 build/v3/mhgp3v_center_cover_mass_probe --points 4000 --seed 20260811 --family terrain --leaf-size 8 --microtile 64 --max-states 900000000
```

Le préfixe brut de 1 692 octets contenant exactement ces deux sorties a le
SHA-256 `fbd28e66fa9f4b21f1554aba53cf25eaa14f2765a41ae23406891eb4a2e6088a`.
Il provenait d'un fichier temporaire Claude encore alimenté; ce hash et les
valeurs recopiées ci-dessous constituent la provenance de l'audit, pas un reçu
produit ni un benchmark sanctionné.

| compteur | 2 000 | 4 000 | pente `log2(C_4000/C_2000)` |
| --- | ---: | ---: | ---: |
| phase locale CPU, un thread | 24,862 s | 99,192 s | 1,996 |
| blocs tentés | 24 473 | 66 934 | 1,451 |
| visites nœud--témoin | 11 342 326 | 48 755 505 | 2,104 |
| tests point--patch | 371 871 550 | 1 454 747 634 | 1,968 |
| évaluations de coins | 1 499 943 648 | 5 880 386 017 | 1,971 |
| masse terminale | 639 273, soit 31,98 % | 1 618 860, soit 20,24 % | diagnostic |

Le signal le plus robuste est le coût par paire logique : 186,0 puis 181,9
tests point--patch, et 750,3 puis 735,2 évaluations de coins. L'extrapolation à
deux points des lois de puissance observées vers 50 k donne environ `2,10e11`
tests et `8,54e11` coins. C'est une estimation de diagnostic, ni un intervalle
de confiance ni un chrono G4.

La cause est directe : `cover_block` appelle le collecteur pour chaque bloc;
le collecteur vide sa pile puis pousse systématiquement le nœud racine zéro. Le
masque mutualise les 64 patches d'un seul bloc, jamais plusieurs blocs. Le cap
`max_states` ne compte que les tâches de partition : il ne borne ni les visites
témoins, ni les tests point--patch, ni les coins. Les bornes dirigées `L/U`
rendues obligatoires par la note pour le profil sanctionné sont absentes. Le
résultat est donc un NO-GO du probe courant, pas une réfutation de tout P1a.

## Conseil d'implémentation avant toute G4

1. Ajouter d'abord les bornes dirigées exactes `L/U` de la note, notamment le
   rejet sûr `U_W<=0`, et mesurer leur gain marginal sur les mêmes petits
   nuages. Une égalité descend toujours.
2. Remplacer le rescan par un ordonnanceur persistant d'états
   `(pair_block,witness_node,active_patch_mask)`. Au split d'un bloc, une
   frontière témoin complète peut servir de hint, mais crops, overlap
   d'extrémités et huit coins de la nouvelle grille doivent être recertifiés;
   aucun crédit parent ne devient une vérité enfant.
3. Préallouer une arène bornée pour les frontières et publier séparément les
   reprises racine, visites patch--nœud, tests de crop, candidats, hints,
   recertifications, allocations, octets et distributions p50/p95/p99/max.
4. Fermer les sept trous de juge ci-dessus, puis rejouer Release, sanitizer et
   le différentiel exhaustif `n<=32`. Une nouvelle ordonnance peut ensuite
   subir un diagnostic structurel CPU court, non sanctionné, sans remplacer le
   protocole P1a direct `n=32` vers 50 k. Elle n'accède à la session G4 que si
   le rescan racine a disparu et si ce diagnostic exclut déjà le régime
   quadratique.

Cette route continue d'éviter toute matrice de paires, tout catalogue global de
supports et toute mosaïque de Delaunay d'ordre supérieur. Cette économie de
mémoire ne compense pas à elle seule un travail implicite quadratique.

GCP non utilisé.

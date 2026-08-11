# Audit de réemploi — Yao48/LBVH et center-cover P1a de la ligne enregistrée

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Réponse à Claude

Le réemploi est substantiel, mais **aucun prune Yao48 ni classifieur terminal de
la ligne enregistrée ne peut être porté comme autorité v3**. Le bon partage est
le suivant :

| élément enregistré | décision v3 |
| --- | --- |
| ownership Morton, tuiles, epochs, lease move-only, reprise et ledger | reprendre le contrat structurel |
| `count--scan`, offsets 64 bits et consommation device-to-device | reprendre le motif transactionnel |
| ordre et disposition binary64, nœuds postorder et ABI de reçus | réécrire pour u16; aucune compatibilité binaire |
| fenêtre de rang fermé Yao48 | différentiel seulement; sémantique incompatible |
| filtre radial fermé | ne jamais employer comme certificat strict v3 |
| décision terminale `above_window` | réécrire selon le contrat v3 avant qualification |
| partition, patches et antichaînes P1a q4 | reprendre les lemmes après réécriture entière |
| juges historiques | squelettes différentiels, jamais autorités indépendantes v3 |

Les deux composants q2 enregistrés sont séparés : une frontière complète à
50 k ne constitue pas un catalogue terminal à 50 k. Le classifieur natif n'a
été reçu qu'à `n=257`, rangs fermés 2 et 3, au commit
`51102a022629988419696f199e20adf1aaad2f85`; ses deux artefacts enregistrés ont
les SHA-256 `5e22d80a23695cba6ffe39e5872acd9cf4f4b488807dc2a95c4ea1f26133b088`
et `049759c4acf61f3ce0bea6d040336525cebfbffe005e8f065c0eba9304992159`.
P1a n'a jamais été compilé ni exécuté nativement selon le registre. Le statut
logiciel du worktree v3 reste exclusivement dans
[l'audit live](AUDIT_ETAT_COURANT.md).

## 1. Incompatibilité sémantique q2

La frontière
[`morton_yao48_device_tiled_pair_frontier.hpp`](../../morsehgp3d/include/morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp)
offre deux modes différents :

- `closed_rank_window` demande `maximum_closed_rank-1` témoins non négatifs,
  soit dix au rang fermé 11;
- `strict_interior_threshold` emploie un seuil enregistré égal à deux.

Le contrat v3 courant demande dix intérieurs stricts pour une tombstone H0 et,
sinon, le census fermé nécessaire à sa décision terminale. Dix contacts de
coquille ne sont donc pas dix témoins stricts. Si la spécification v3 revenait
un jour à une censure purement par rang fermé, cette décision devrait être
modifiée explicitement dans les documents normatifs; elle ne peut pas être
inférée du code ancien.

Le filtre radial enregistré compare `minimum_squared_distance >= 3D`. Cette
égalité est sûre pour sa sémantique historique fermée, mais elle est fausse si
on la réemploie comme preuve de dix intérieurs stricts. La fixture u16 minimale
suivante doit accompagner tout port :

- ancre `p=(0,0,0)`;
- banque de la chambre `x>=y>=z>=0` : `(1,0,0)`, `(2,0,0)`, `(3,0,0)`,
  `(4,0,0)`, `(5,0,0)`, `(2,1,0)`, `(3,1,0)`, `(3,2,0)`, `(4,1,0)` et
  `(4,2,0)`;
- `D=25` et cible `q=(5,5,5)`, donc `||q-p||^2=75=3D`.

Le témoin `(5,0,0)` vérifie exactement
`q dot w=||w||^2=25`; il est sur la coquille. Il n'existe que neuf témoins
stricts dans cette banque. Un cutoff large produit donc une fausse tombstone
v3; l'égalité doit descendre.

Le classifieur enregistré s'arrête lorsque dix points non-support ont été vus,
qu'ils soient stricts ou de coquille, et rend `above_window`. Pour distinguer
les contrats, utiliser les extrémités `(5,10,10)` et `(15,10,10)`, puis les dix
contacts
`(10,15,10)`, `(10,5,10)`, `(10,10,15)`, `(10,10,5)`, `(7,14,10)`,
`(7,6,10)`, `(13,14,10)`, `(13,6,10)`, `(7,10,14)` et `(7,10,6)`.
Tous sont à distance cinq du centre `(10,10,10)` : la profondeur stricte vaut
zéro et le rang fermé vaut douze. Le classifieur historique rend
`above_window`; le contrat v3 déclaré doit soit produire le census complet,
soit être normativement changé avant le port. Copier seulement le
`count--scan` ne tranche pas cette question.

## 2. Ce que mesure réellement la frontière enregistrée

Le reçu G4 du 7 août au commit
`31b64409bb43470f6b7eecbe56121d8a9edba679` ferme comptablement
`1 249 975 000` paires en `7 962 604` candidates et `1 242 012 396` paires
prunées. À travail identique, le lanceur rang 11 vaut 2,434 s isolé et 7,480 s
comme dixième rang du balayage; la recertification hôte vaut respectivement
8,628 s et 8,756 s. Les JSON isolé et tous-rangs ont respectivement les
SHA-256 `091f0ce555f56816075bb207ebfd21fa3775143e4953ee4b804b7085f52c88b1`
et `22285853c2602f80704dc5c6fe34d7d1416925da30003c145112b4256ca8f32a`.
Ces contextes distincts sont détaillés dans
[`RESULTATS.md`](../../docs/validation/phase15_session_g4_20260807/RESULTATS.md).
Aucune plage « 2,4 à 3,0 s » ne résume honnêtement ces résultats.

Cette mesure porte sur la frontière seule : aucun rang exact terminal, support
q3/q4, fold, forêt ou payload officiel. La même source synchronise après chaque
subdivision et relit des contrôles d'ancres sur l'hôte. Les candidates et reçus
restent device-to-device, mais leur matérialisation, l'arène fixe, les contrôles
D2H et les synchronisations subsistent. La lease est un backpressure
host-mediated, pas un scheduler device persistant.

Le classifieur compacté ne corrige pas ce verrou : chaque candidate repart de
la racine du LBVH, puis chaque chunk paie scans, tris, reconstruction du
catalogue, contrôle D2H et synchronisation. `count--scan` dimensionne la sortie;
il ne mutualise pas la recherche de témoins.

Conclusion de performance : la ligne enregistrée prouve qu'une matrice de
paires n'est pas nécessaire, mais elle ne prouve ni un travail sous-quadratique
ni une latence sous la seconde. Un port littéral est interdit.

## 3. Banque Yao par antichaîne

La recherche des dix plus proches n'est pas requise. Pour une ancre `p` et une
chambre `c`, une banque peut être une antichaîne de nœuds LBVH `W_i` telle que :

- leurs plages de feuilles sont disjointes et excluent l'ancre;
- chaque feuille créditée vérifie `0<||w-p||^2<=D_c`;
- chaque boîte est entièrement certifiée dans la chambre;
- la somme des masses atteint dix.

Le preflight v3 de positions 3D distinctes fournit la stricte positivité. Une
extension qui autoriserait plusieurs `PointId` colocalisés devrait descendre et
exclure toutes les feuilles `w` telles que `||w-p||^2=0`.

Le majorant exact est :

$$D_c=\max_i\max_{x\in\mathrm{box}(W_i)}\left\Vert x-p\right\Vert^2.$$

Dix feuilles canoniques de l'union fournissent alors dix vrais témoins. Dans la
chambre canonique, l'échec d'au moins une coupe Yao implique
`||q-p||^2<=3D_c`. Une boîte cible est donc prunable par l'enveloppe radiale si
son ensemble conservateur non vide de chambres possibles `S` possède des
banques pleines et si :

$$\mathrm{dist}^2(p,\mathrm{box})>3\max_{c\in S}D_c.$$

Toute égalité descend. Le reçu engage `S`, la version et le digest de chaque
banque; omettre une chambre possible doit mourir sur une fixture dédiée.

Cette proposition ne justifie pas une table globale `n*48*10`. La banque
ponctuelle v3 corrigée conserve onze candidats afin d'exclure la cible avant
d'engager dix témoins; les documents d'architecture courants imposent donc
`O(B*48*(K+1))` pour une tuile de `B` ancres et rendent l'orientation inverse
facultative. Une banque de l'autre extrémité n'est essayée que si elle est déjà
disponible dans la même tuile ou un cache borné et authentifié. Les 96 MB
historiques supposent dix positions Morton `u32` par slot; onze candidats
occupent 105,6 MB en `u32` ou 211,2 MB avec les `PointId u64` enregistrés, hors
rayons, masques et offsets.

## 4. P1a : mathématique réutilisable et machine à remplacer

Le commit `95dd8036a2fcb36c8a7b6aeb7c44197d9c9f7e03` contient un vrai prior
art q4 : partition triangulaire, 64 patches, seuil huit, tests médiateur et
domaine des milieux, choix témoin côté A ou B, antichaînes de sous-arbres et
ledger de masse.

La cover historique `5H/8`, où `H` est le maximum des séparations axiales du
bloc (`maximum_separation`), est sûre en binary64, car `X<=3H^2` et
`sqrt(X/8)<5H/8`. Elle n'est pas le `T_0` entier de la note v3.
Après arrondi, aucune inclusion universelle n'existe : pour les boîtes
ponctuelles `A=(0,0,0)` et `B=(1,0,0)`, l'ancienne cover réelle vaut
`[-1/8,9/8]`, tandis que le nouveau `T_0` entier vaut `[-1,2]` sur l'axe actif.

Le filtre v3 doit employer sans ambiguïté
`X=maxdist^2(A,B)` et le déplacement quadratique `||t||^2<=X/8`. Pour un
nœud témoin, c'est une **borne inférieure** strictement positive de la marge
sur chaque patch qui prouve tous ses points, jamais un majorant positif.

Les défauts de machine enregistrés ne sont pas à porter :

- génération des shards sérielle et rescans donnant un coût quadratique en
  nombre de shards;
- 64 parcours indépendants repartant de la racine avant même le test
  microtuile;
- allocations et libérations à chaque appel;
- antichaînes développées seulement dans les reçus bornés, pas comme lease aval
  du profil 50 k;
- tentative du prune sur les blocs diagonaux sans preuve v3 dédiée.

Le port v3 reste q4-only. Il teste la microtuile avant les patches et utilise un
unique parcours collectif `(witness_node, active_patch_mask)`. Une antichaîne
parent peut guider l'ordre de l'enfant, mais chaque crédit doit être recertifié
contre son overlap, son crop et les huit coins de l'enfant avant emploi. Le
profil 50 k ne développe ni candidats ni reçus point-par-point par bloc. Un
self-bloc est partagé tant qu'une preuve et une fixture diagonales ne sont pas
ajoutées.

## 5. Juges et layouts

Le rejeu historique P1a recalcule la même borne `power_lower` sur une AABB; il
ne développe pas chaque vrai `PointId` témoin contre chaque vrai endpoint et
n'énumère pas les supports q4 propres positifs non inertes. Il fournit un
décodage utile, pas une autorité indépendante.

Le juge v3 doit reconstruire lui-même `X`, les racines entières, `T_0`, les 64
indices et exactement un statut par patch; vérifier chaque preuve
d'infaisabilité, chaque antichaîne et chaque point réellement crédité; puis
énumérer indépendamment les q4 propres positifs et la bijection des paires. Les
oracles historiques restent des différentiels supplémentaires.

Les layouts ne sont pas compatibles. La ligne enregistrée emploie des
coordonnées binary64 SoA, des `PointId` et plages `u64` et des nœuds postorder
de 80 octets. La v3 emploie des coordonnées u16/i64, une clé Morton 48 bits,
des AABB entières et un ordre de nœuds différent. Sont réutilisables les
**invariants** d'ownership, d'epoch, de lease, de capacité et de ledger, pas les
vues brutes ni l'ABI.

## 6. Ordre d'implémentation conseillé

1. Graver les deux fixtures q2 ci-dessus et décider explicitement le contrat
   terminal v3 avant tout port CUDA.
2. Conserver le prototype CPU comme différentiel; remplacer les banques par
   antichaînes tuilées et mesurer la masse par reçu, sans table globale.
3. Construire un parcours collectif des régions et une classification groupée;
   conserver un reçu logique compact `(banque,plage,digest)` pour le rejeu, sans
   arène persistante de reçus développés ou par-feuille, et sans redémarrage de
   racine par candidate.
4. Porter seulement les schémas transactionnels de la ligne enregistrée vers
   des layouts u16 neufs, puis rejouer la gate de compteurs device avant toute
   campagne de latence.
5. Auditer puis requalifier le prototype P1a q4 séparément avec l'arithmétique
   entière à l'échelle seize et fermer le différentiel hôte `n=32`; dans la
   même session G4 gardée, fermer
   ensuite la parité native, le rejeu `n=32` et Compute Sanitizer, puis lancer
   les profils 50 k sans palier de performance ni retry. Cette campagne mass-only
   ne qualifie ni P1 ni le pipeline.
6. Installer le producteur de payload complet, les dix forêts et les verticales
   avant de prononcer un résultat `warm_e2e` sous une seconde.

GCP non utilisé pour cet audit.

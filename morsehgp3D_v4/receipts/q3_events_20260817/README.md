# Reçu — instruction q3 : ancres survivantes → événements

Date : 17 août 2026 UTC. CPU 4 cœurs single-thread, non contractuel.

Juge PAR IDENTITÉS contre l'oracle brut (tous les C(n,3) triangles) :

| famille | n | événements | ev/point | shell refusés | manquants | en trop |
|---|---:|---:|---:|---:|---:|---:|
| uniform | 400 | 48 965 | 122,4 | 15 | **0** | **0** |
| eight_clusters | 400 (porte) | — | — | — | **0** | **0** |

Échelle (sans juge) :

| famille | n | ancres vues | porteurs testés | événements | ev/point | t_instruction |
|---|---:|---:|---:|---:|---:|---:|
| uniform | 2000 | 232 006 | 7 307 995 | 313 537 | 156,8 | 35,7 s |
| eight_clusters | 2000 | 479 929 | 115 512 175 | 249 093 | 124,5 | **474,8 s** |

Constat : l'exactitude est acquise (0/0 au juge, refus transactionnel des
coquilles), mais le coût d'instruction est dominé par les ancres
inter-amas à grande lentille — 240 porteurs testés par ancre en moyenne sur
eight_clusters, chacun payant une descente de profondeur. C'est le poste
« census q3 » déclaré non résolu par la v3, reproduit et chiffré sur la v4.
Limite déclarée : le juge partage le prédicat de profondeur du sujet (il
valide énumération, owner, complétude — pas l'arithmétique ; l'oracle à
arithmétique indépendante viendra avec oracle/).

## Addendum — filtre h_a/h_b branché avant l'instruction (consensus des audits)

Mêmes nuages, mêmes événements (exactitude préservée, 0 doublon) :

| famille | n | ancres tuées par h_a/h_b | porteurs testés | t_instruction |
|---|---:|---:|---:|---:|
| eight_clusters | 2000 | 81 390 / 479 929 (17,0 %) | 77,5 M (−32,9 %) | **253,3 s (−46,7 %)** |
| uniform | 2000 | 7 741 / 232 006 (3,3 %) | 6,7 M (−8,3 %) | 26,0 s (−27,3 %) |

Confirmation de la prédiction d'audit : les ancres tuées sont
préférentiellement les LONGUES (17 % des ancres portent 47 % du coût sur la
famille adversariale). Le poste restant est le census partagé par ancre
(plan médiateur, réponse Q9) — prochaine primitive.

## Addendum 2 — cover partagé trié par ancre (borne √3·D/2 de l'audit)

Une seule requête `B(m, √3·D/2)` par ancre (elle couvre lentille ET témoins,
inclusion prouvée par l'audit § 6.2), liste TRIÉE par distance croissante au
milieu, scan plat par porteur avec early-exit à h_3 (les points proches du
milieu sont intérieurs à la plupart des circum-boules : les boules profondes
meurent en ~h_3 tests). Mesure appariée `--census=tree|cover`, événements
identiques, juges 0/0 sur les deux chemins :

| famille | n | t_instruction tree | t_instruction cover | gain |
|---|---:|---:|---:|---:|
| uniform | 400 | 2 652 ms | 290 ms | 9,1× |
| uniform | 2000 | 25 965 ms | 2 995 ms | 8,7× |
| eight_clusters | 2000 | 253 272 ms | **24 214 ms** | **10,5×** |

Cumul sur eight_clusters n=2000 : 475 s (origine) → 24,2 s (filtre h_a/h_b
puis cover partagé), soit ≈ 20×. Poste dominant restant : la collecte du
cover lui-même (237,6 M de points collectés/triés, ~594 par ancre sur la
famille adversariale) — c'est ce que la version A de l'audit (arbre 2D des
porteurs + range-add au niveau des blocs) doit amortir, avec l'arithmétique
i192 des T_x prouvée avant le kernel.

## Addendum 3 — paquets de témoins certifiés + scan site-major (audits du 17 août)

Le préfiltre ne jette plus ses certificats : les IDs du cœur, de h_a(a) et de
h_b(b) (disjoints par théorème, < h_3 au total sur une ancre survivante)
initialisent la profondeur de chaque porteur (`depth = base`) et sont exclus
du scan — et deviennent le préfixe des `InteriorIds` de chaque événement.
Le census passe en site-major (chaque site du cover défile devant tous les
porteurs actifs, masques saturés, SoA — la structure « un CTA par ancre,
lanes = porteurs » de la cible CUDA).

- Juge d'identités : 0/0 inchangé (packet on ET off) ; mutant
  `packet-no-exclude` (double compte) tué par le juge (code 4).
- Tests de puissance : 3,91 M (packet on) contre 7,63 M (off) à n=400 —
  le paquet retire ~49 % des prédicats.
- eight_clusters n=2000 : 26,7 s site-major contre 24,2 s carrier-major —
  léger surcoût de boucle, payé par le payload (1 292 638 identités
  intérieures collectées, ~5,2/événement) et la structure GPU ; l'arbre de
  centres (version A) viendra comme accélérateur jugé contre cette baseline.

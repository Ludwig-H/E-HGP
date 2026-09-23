# Contre-audit B — MEB proposé pour la tour FULL (WIP)

23 septembre 2026. Lecture du worktree produit à base `6200bb5a` :
`anchor_meb.hpp`, `full_ball_tower.hpp`, porte différentielle, probe et
protocole G4 sont **non commités** à cette date. Aucun reçu local figé,
GPU/G4 ni gain de tour ne découle de cette lecture.

## Exactitude de l'objet

Le nouveau `anchor_meb_proposed` cherche une miniballe par Welzl en
`double`, mais cette proposition ne décide rien. `attempt` reconstruit
un support positif exact et teste la puissance de **tous** les sites ;
une proposition non certifiée repasse par l'énumération de référence.
Une boule englobante à support positif est la MEB, unique ; si sa
coquille contient plus de sites que le support, le code réénumère les
triples puis quadruples de **ce bord exact** en ordre d'IDs croissants.
Tout support accepté par la référence doit être sur le même bord ; le
premier support ainsi retenu est donc celui de l'énumération complète.
Si bord et support ont la même taille, ce support est unique. Cette
preuve est conditionnée à la certification exacte et à la réénumération
du bord ; la proposition flottante seule n'est jamais un certificat.

À la lecture, pas de perte de support ni de course manifeste : les
workers FULL possèdent leurs compteurs privés, et le raccord change
le résolveur local, pas le recensement du catalogue. Le gate nouveau
compare résultat exact, clé, niveau, support et taille de coquille sur
aléatoire u18, points co-sphériques, cubes/octaèdres et permutations ;
il prévoit un mutant de canonisation et un repli sur proposition
corrompue. Il manque encore des fixtures de débordement pour chacun
des **quatre compteurs ajoutés**, des modes d'arrondi/FENV et un contrôle
du coût complet de la tour, pas seulement du nombre d'essais exacts.

## Qualification et protocole

Sur R5 K10/08-000000, l'ancien chemin effectue environ **11,3 M**
appels MEB et **342 M** essais de triples : réduire ce poste est
pertinent. Le gate exige moins d'essais exacts, mais ne mesure pas le
temps CPU de Welzl, les replis, le RSS et l'effet sur toute la chaîne.
La proposition peut même être plus chère sur de petits ensembles ; le
choix produit doit venir d'une ablation FULL appariée, mêmes entrées et
sorties, avec CPU/mur et nouveaux compteurs par K et taille de support.

Le premier diff du probe annonçait `meb_accounting=...v3` quand le
lecteur G4 exigeait `...v2` : un lancement aurait été refusé. Le WIP a
ensuite aligné les littéraux `mhgp9_tower_probe_v10` et MEB v3 entre
probe, worker et faux sondeur, **sans encore exporter**
`proposals`, `verified_proposals`, `boundary_canonicalizations` et
`proposal_fallbacks` dans le JSON, sa liste de clés exactes et le
selftest. La marque v3 seule ne prouve ni l'activation ni le coût de la
voie proposée. Ajouter ces quatre compteurs avec invariants
`verified≤proposals`, `fallbacks≤proposals`, une mutation causale et
des planchers du préflight sur une fixture pertinente. Garder le reçu
R6 et son ancien lecteur figés ; jamais requalifier R6 avec le schéma
v10.

L'utilisateur a choisi l'**arrêt immédiat** de la session G4 SPOT
envisagée sous le schéma incompatible. Le contrôle ciblé à 05 h 33 UTC
trouve la cible épinglée déjà `TERMINATED` (dernier arrêt
04 h 50 min 54 s UTC) ; aucun stop supplémentaire n'a été lancé.
Une nouvelle campagne payante n'est utile qu'après protocole, gate et
identité d'objet clos localement. Statut public inchangé :
`not_claimed`.

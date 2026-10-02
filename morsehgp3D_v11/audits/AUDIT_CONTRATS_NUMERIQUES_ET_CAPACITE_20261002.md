# Audit indépendant v11 — état des fondations

2026-10-02 12:28 UTC. Publication `6a22a9118`, source exécutée sur G4
`a97180667` : aucun code produit ne change entre ces deux commits.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Lecture de sources figées, recoupe d'archives et contrôles autonomes Fraction ;
aucun nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Fondations qualifiées dans leur périmètre ; aucun défaut nouveau établi.**
L'oracle séparé, les entiers exacts et le propriétaire immuable permettent
le raccord au catalogue. La nouvelle réserve mathématique porte sur une
hiérarchie commune aux ordres : des groupes core K1/K2 peuvent se croiser.
Aucun contrat catalogue/FULL, LiDAR, GPU ou 100 ms acquis.

## Qualification recoupée

[Archives, sources et inventaires G4](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md) :
les trois paquets correspondent exactement aux archives Git annoncées ;
les 91 entrées des manifestes de résultats sont vérifiées. Les deux premières
campagnes échouées restent conservées ; seule la troisième est conforme.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 205/205 | 130 portes de base +75 références Python |
| GCC ASan/UBSan, TSan, B21, B24 | 130/130 chacune | Même base, références Python exclues |
| Poison | 131/131 | Base +porte poison |
| Style ; mutants | 2/2 ; 9/9 | Style inclus dans la base ; manifestes et campagnes séparés |

Les 103 mutants correspondent aux manifestes : 101 juges exécutés
(98 causes code, trois ligne), deux refus de compilation attendus,
aucun signal/délai. Clang absent, aucune qualification Clang.
La sentinelle LiDAR ne traite aucune donnée. CPU sur VM G4 ne signifie pas GPU.

Les anciens défauts de minuteur/lancement restent fermés. **Interruption
globale corrigée** : le signal reçu impose désormais l'échec du résumé final.
**Isolation après sortie normale encore ouverte**, explicitement
`isolation=not_certified` : attendre le parent ne certifie pas la quiescence
de sa descendance avant la configuration suivante. Fermeture finale du groupe
et arrêt ciblé de ces sessions recoupés ; aucune fuite de VM déduite.
Pour la prochaine capture, conserver aussi les hashes des exécutables,
CMakeCache et flags par configuration : `default_build=false` laisse cette
provenance binaire vide, malgré des sources et journaux bien attribués.

## Numérique et géométrie

[Lecture des prédicats et bornes](../receipts/audit_independant_20261002/numeric_geometry_review_3/README.md) :
196 contrôles autonomes identiques normal/−O ; aucun défaut trouvé jusqu'à
B24. Au point serré du test de milieu, chaque membre est strictement inférieur
à 96·2^(5B), donc à 2^127 en B24 : i128 signé suffit. Les centres des
circumsphères peuvent sortir de la boîte ; les preuves n'utilisent pas leur
convexité. Le q4 strict à préfixe q3 obtus et le poids q4 nul ont leurs portes.
Les 16 requêtes G4 préparées par l'audit ne sont pas exécutées.

[Entiers et niveaux](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md) :
narrow protège les conversions ; les opérations conservent la sortie sur
refus, alias compris ; produits et comparaisons Level ont la largeur requise.
`Sphere::through` calcule une circumsphère, pas une MEB ni une clé canonique.
Le futur u32 exige de reprendre Vec/dot/cross/centres natifs, pas seulement Level.
Les portes des futures expressions filtrées restent à établir sur leur domaine
réel ; la doctrine F3/F4/F6 ne les qualifie pas par anticipation.

## Propriétaire et capacité

[Cloud immuable et vingt portes dans six configurations](../receipts/audit_independant_20261002/cloud_immutable_review_3/README.md) :
les 19 fichiers relus sont identiques aux sources G4. Stockage privé, vues
const, entrée stable pendant l'appel, restitution du delta réservé et pic
absolu documenté ferment les réserves précédentes.

Au raccord, fixer la durée de vie et l'adresse du propriétaire emprunté :
déplacer Cloud conserve ses buffers, mais vide l'objet initial. Un index
empruntant cet objet doit avoir un contrat compatible. Ajouter une porte de
pic avec les quatre buffers d'entrée déjà réservés et un ancien pic supérieur
au nouveau. Aucun index actuel n'est déclaré fautif : il n'est pas livré.

Pic propre de préparation : max(2Rn+H, Rn+4n+24s+8), R=16 en B18/21,
R=32 en B24. Avec s=n et quatre entrées Buffer 16n :
B18/21=max(48n+H,60n+8), B24=80n+81920.
À 30 M retours uniques : environ 1,8/2,4 Go décimaux, **préparation seulement**,
hors index/catalogue/FULL ; formules, aucun benchmark massif/RSS.
La réserve SHA sur l'ancien IO privé concerne un port non livré ; elle reste
dans [son reçu historique](../receipts/audit_independant_20261002/io_contract_review_2/README.md),
pas dans la qualification actuelle.

## Prochain raccord utile

Le plan catalogue séquentiel T=0 distingue correctement circumsphère,
support critique, hull fermé et census I/U complet. Ne pas rejeter q4 sur
l'obtusité d'un préfixe q3. Deux passes count/fill doivent conserver le même
propriétaire et la même politique, vérifier leurs fins et réserver les états
simultanés ; un juge catalogue Gram/Fraction reste distinct du seul FULL.

[Oracle séparé](../receipts/audit_independant_20261002/reference_separation_review_2/README.md),
[stabilité frontière](../receipts/audit_independant_20261002/boundary_stability_review_2/README.md)
et [croisement statique inter-K](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md)
fixent les objets à préserver. Livrer core/cover à K fixé reste cohérent ;
un choix commun aux K doit déclarer sa perte. Le [massif](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md)
reste secondaire après le jalon trame. Les modifications privées ultérieures
du développeur ne reçoivent aucune qualification de ces copies figées.

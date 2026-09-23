# Contre-audit B — G4 R7b, MEB proposé sur tour FULL CPU

23 septembre 2026. [Reçu R7b](../receipts/g4_tower_r7b_20260923/README.md)
publié avec `ec6d1b74`, paquet moteur **`8e8b83a3`**, sonde v11,
plan v6, grille u18 1 mm, trois trames sans sol de la seule séquence
08, `s=8`, W48/static48. La G4 SPOT a démarré à 06:08:58 UTC et
s'est arrêtée à 06:16:11 UTC ; instance et autres SPOT relues
`TERMINATED`. **CPU seul** (`GPU_executed=false`). Ni Welzl
move-to-front `8fa03046`, ni le nouveau tri/libération d'index du
commit publiant le reçu ne font partie du paquet mesuré.

## Réception et identité effectivement vérifiées

Les **388/388** SHA du reçu sont valides. Le lecteur extrait du commit
exact `8e8b83a3`, avec worker et contrôleur égaux au manifeste, rend
`completed` en Python normal **et** sous `-O` : 24/24 cas
`complete_relative`, préflight 1 500 sites non vide, arrêt ciblé
certifié. Dans les paires ON/OFF, seul `tower_meb_proposal` varie ;
les propositions sont toutes vérifiées et `fallbacks=0`.

Les 18 comparaisons archivées jugent **une projection** : identité
d'entrée, effectifs de catalogue, résumés des ordres et digest FNV64.
Elles ne publient pas les flux intégraux de clés, supports, coquilles
et parents. Dire « même objet » au sens bit-à-bit intégral serait
plus fort que la preuve du reçu. La correction des petits oracles
et la vérification exacte du proposer sont distinctes de cette
comparaison LiDAR à grande taille ; `complete_relative` ne prouve
pas l'absence de clés entièrement omises.

## Temps et travail

Sur 08/000100, premières répétitions ON contre OFF :

| Tour | Chaîne ON / OFF hors digest | Digest ON / OFF | Mur externe ON / OFF | q3/q4 ON | FULL ON |
| --- | ---: | ---: | ---: | ---: | ---: |
| K=1..5 | 3,669 / 3,686 s | 0,162 / 0,163 s | 3,925 / 3,924 s | 2,523 s | 0,734 s |
| K=1..10 | 9,579 / 10,053 s | 0,798 / 0,809 s | 10,555 / 11,098 s | 5,226 s | 3,199 s |

Les deux répétitions K10 montrent un gain de chaîne d'environ 2 à
5 % selon la trame, et de tour d'environ 5 à 8 %. À K5, la tour
ON est neutre ou plus lente ; les moyennes de chaîne ON sont même
plus lentes sur les trois trames, à l'échelle du bruit entre essais.
Ne pas conclure à un meilleur défaut K5 avec deux répétitions.

Sur le meilleur K10, **17,49 M** paires q3/q4 sont encore développées,
**4,38 M** boules entrent au catalogue, et la chaîne garde 5,226 s
de q3/q4 plus 3,199 s de FULL. Rendre q3/q4 gratuit dans la chaîne
actuelle laisserait 4,353 s ; rendre q3/q4 et FULL gratuits laisserait
encore 1,154 s. Ce ne sont pas des planchers matériels pour un
nouvel algorithme, mais ils empêchent d'attribuer le contrat 1 s à
un seul port de noyau. À K5, q3/q4 gratuit laisserait 1,147 s.

Le digest est **hors** `chain_total` dans R7b. Pour une comparaison de
périmètre avec R6, l'ajouter, sans oublier que plusieurs autres
optimisations et la frontière temporelle ont changé entre sessions :
R6→R7b n'est pas une ablation causale. Le sans-sol utilise un masque
déjà préparé ; ni sa segmentation ni la trame brute avec sol ne sont
qualifiées ici. Aucune coupe LiDAR appariée 8k/16k/32k, aucun `s=10/12`,
aucune autre séquence, aucun p95 ni GPU. Contrat 1 s/100 ms et pente
sous-quadratique globale toujours **non acquis**.

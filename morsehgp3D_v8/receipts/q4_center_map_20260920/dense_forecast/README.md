# Filtre VC puis carte partagée : prévision dense auxiliaire

Cette expérience adapte explicitement les deux sources auxiliaires de la tranche 26, sans modifier les anciennes preuves. Elle est hors de l'inventaire produit de 170 sources ; ses deux sources s'ajoutent aux hashes de capture, soit 172 fichiers.

Pour une arête fixée et ses n−2 triangles aigus possédés, la sonde exécute réellement le filtre Variance/Collective sur le pool spatial C64, puis interroge **une seule carte lazy partagée**, uniquement pour les familles q4 encore vivantes. La carte est créée à la première requête utile. Le workspace collectif reste partagé entre tous les triangles ; les domaines Disk/Positive, profondeurs 5/7 et K5/K10 sont comparés à n8000/16000/32000 : 48 configurations, plus une compilation.

Les coûts payés comprennent nuage/index/cover/pool, préparation du domaine positif par blocs, projections/hull/formes, prédicats, copies des listes héritées, capacités du stockage de nœuds et toutes les requêtes. Le pic publié additionne les capacités de carte et de workspace effectivement simultanées. Les durées internes du filtre, de création de carte et de requêtes sont séparées ; elles ne s'ajoutent pas une seconde fois au temps englobant.

La carte ne produit pas de candidats. **Ni le census ni le tri des familles restantes ne sont exécutés.** Le champ `future_scan_lower_bound = cover_sites × q4_survivors` est le minimum de lectures imposé par le fallback actuel, avant son tri, ses coquilles et les sorties. Il ne prédit pas un temps et ne qualifie ni la tour FULL, ni G4, ni un coût global sous-quadratique.

## Deux recettes distinctes

- `dense_prefix` reproduit exactement la grille et l'ordre de la tranche 26.
- `dense_permuted` construit les clés SplitMix64 de **tous les 32 718 indices** de la grille 41×21×38, les trie avec départage par indice, puis seulement prend les n−2 premières complétions. Les endpoints restent les IDs0/1. La génération, le tri complet et sa capacité transitoire sont payés. Ce n'est pas l'ordre SHA256 de l'auditeur et ses résultats ne sont pas hérités.

Un calcul Python séparé reconstitue les six entrées et vérifie leurs hashes. Les propriétés « cover complet, tous triangles aigus et arête strictement la plus longue » sont vérifiées par des inégalités scalaires. La préparation produit valide indépendamment l'unicité.

## Préflight avant gel — pas la qualification

À 32k, Positive/profondeur7 n'ajoute que 0/24 rejets aux K5/K10 du préfixe, et 109/0 sur la permutation. Le domaine positif se prépare en sept visites de nœuds et zéro test scalaire pour ces cas. Cette carte emploie seulement les 64 témoins du même pool, contrairement au prototype d'audit utilisant tous les témoins : ses grands gains ne doivent pas lui être attribués.

Le développement auxiliaire a rencontré une erreur de compilation locale lors de l'extraction de ses tables JSON, corrigée avant tout préflight exécuté. L'ajout ultérieur des trois compteurs produit de copies/capacités a nécessité l'adaptation de ces tables et du validateur ; le premier essai du validateur avec l'ancien binaire ne pouvait pas fournir ces champs. Ces essais antérieurs au gel ne constituent pas une capture close ni un échec de la géométrie produit.

## Capture close

`forecast_ysxpufca/` : **49 commandes / 48 configurations PASS**, fermeture inchangée des 172 sources (170 produit + 2 auxiliaires), de l'archive, du cache, du compilateur et de l'exécutable temporaire. Les quatre lecteurs historique/live normal/−O concordent ; les deux selftests détectent chacun **24 corruptions de reçus**, pas 24 fautes géométriques. Commandes et sorties : `READBACK.json`.

Dans la table, les triplets sont toujours à n8k / 16k / 32k. Les lectures futures sont un **minimum non exécuté**, exprimé en millions, et excluent le tri des familles. « Ajouts » compte seulement les rejets de carte après Variance/Collective. Tous les essais emploient C64 et un budget de 4096 nœuds.

| Recette | K | Domaine / profondeur | Ajouts de rejets | Familles restantes | Lectures futures (M) | Ratios aux doublements |
|:---|---:|:---|:---|:---|:---|:---|
| Préfixe | 5 | disk / 5 | 0 / 0 / 0 | 961 / 1195 / 5792 | 7.688 / 19.120 / 185.344 | ×2.487 / ×9.694 |
| Préfixe | 5 | disk / 7 | 0 / 0 / 0 | 961 / 1195 / 5792 | 7.688 / 19.120 / 185.344 | ×2.487 / ×9.694 |
| Préfixe | 5 | positive / 5 | 0 / 0 / 0 | 961 / 1195 / 5792 | 7.688 / 19.120 / 185.344 | ×2.487 / ×9.694 |
| Préfixe | 5 | positive / 7 | 0 / 0 / 0 | 961 / 1195 / 5792 | 7.688 / 19.120 / 185.344 | ×2.487 / ×9.694 |
| Préfixe | 10 | disk / 5 | 0 / 0 / 0 | 1771 / 3598 / 11195 | 14.168 / 57.568 / 358.240 | ×4.063 / ×6.223 |
| Préfixe | 10 | disk / 7 | 0 / 0 / 0 | 1771 / 3598 / 11195 | 14.168 / 57.568 / 358.240 | ×4.063 / ×6.223 |
| Préfixe | 10 | positive / 5 | 0 / 0 / 0 | 1771 / 3598 / 11195 | 14.168 / 57.568 / 358.240 | ×4.063 / ×6.223 |
| Préfixe | 10 | positive / 7 | 0 / 0 / 24 | 1771 / 3598 / 11171 | 14.168 / 57.568 / 357.472 | ×4.063 / ×6.210 |
| Permuté | 5 | disk / 5 | 0 / 0 / 0 | 1256 / 2433 / 3977 | 10.048 / 38.928 / 127.264 | ×3.874 / ×3.269 |
| Permuté | 5 | disk / 7 | 0 / 0 / 0 | 1256 / 2433 / 3977 | 10.048 / 38.928 / 127.264 | ×3.874 / ×3.269 |
| Permuté | 5 | positive / 5 | 0 / 0 / 0 | 1256 / 2433 / 3977 | 10.048 / 38.928 / 127.264 | ×3.874 / ×3.269 |
| Permuté | 5 | positive / 7 | 5 / 114 / 109 | 1251 / 2319 / 3868 | 10.008 / 37.104 / 123.776 | ×3.707 / ×3.336 |
| Permuté | 10 | disk / 5 | 0 / 0 / 0 | 2656 / 5101 / 9394 | 21.248 / 81.616 / 300.608 | ×3.841 / ×3.683 |
| Permuté | 10 | disk / 7 | 0 / 0 / 0 | 2656 / 5101 / 9394 | 21.248 / 81.616 / 300.608 | ×3.841 / ×3.683 |
| Permuté | 10 | positive / 5 | 0 / 0 / 0 | 2656 / 5101 / 9394 | 21.248 / 81.616 / 300.608 | ×3.841 / ×3.683 |
| Permuté | 10 | positive / 7 | 0 / 45 / 0 | 2656 / 5056 / 9394 | 21.248 / 80.896 / 300.608 | ×3.807 / ×3.716 |

Le disque seul n'ajoute aucun rejet dans ces 24 configurations. Le domaine positif à profondeur 5 n'en ajoute pas non plus. Positive/profondeur7 ajoute seulement 24 rejets sur le préfixe K10/32k et 5/114/109 sur le permuté K5 ; à K10 permuté, ses 45 rejets à 16k ne se retrouvent pas à 32k. Le pool spatial dépend du nuage : ces ensembles de témoins ne sont pas emboîtés.

Cette absence de gain important ne contredit pas l'audit : celui-ci utilisait tous les témoins du cover, tandis que cette composition produit réemploie seulement le pool C64. La carte prépare exactement 64 formes, une fois par arête. Les mesures ne justifient pas d'augmenter aveuglément profondeur ou budget.

### Travail réellement payé par Positive/profondeur7

| Recette | K | Visites de requête, 8k / 16k / 32k | Tests témoins | IDs copiés dans les listes héritées | Pic carte+workspace (octets) |
|:---|---:|:---|:---|:---|:---|
| Préfixe | 5 | 26695 / 33216 / 177125 | 1395 / 1113 / 1526 | 3720 / 3024 / 3672 | 27152 / 22480 / 24536 |
| Préfixe | 10 | 62652 / 107143 / 331518 | 1560 / 1545 / 1929 | 3720 / 3864 / 4128 | 27888 / 25632 / 25016 |
| Permuté | 5 | 35298 / 70570 / 124053 | 1422 / 1443 / 1519 | 3400 / 3432 / 3664 | 23440 / 23552 / 24096 |
| Permuté | 10 | 77066 / 164875 / 287848 | 1700 / 1846 / 1626 | 3936 / 4100 / 3788 | 25360 / 24960 / 24344 |

Pour tous les essais Positive, le domaine exact se prépare en **7 visites de nœuds, 0 test scalaire**, avec deux blocs admis couvrant n−2 complétions. Les endpoints sont exclus séparément. Il n'y a donc pas de scan scalaire systématique du nuage par arête dans cette préparation. Le propriétaire et l'index globaux sont, eux, réellement construits et leur travail est publié séparément.

Sur les 48 configurations, aucun budget de nœuds n'est épuisé. Le nombre de nœuds est faible, mais les frères jamais interrogés ont bien leurs copies et listes comptabilisées. Les tris de VC restent également payés : par exemple 3 729 870 comparaisons au préfixe32k/K10, en amont de la carte. Le pic mémoire exclut les métadonnées d'allocateur, chevauchements transitoires de réallocation, objets fixes et le stockage global partagé ; il ne s'agit pas du RSS.

**Conclusion :** la composition exacte fonctionne, mais ne ferme pas le coût résiduel. Sur le préfixe, les dernières croissances des lectures futures restent ×9,694 à K5 et ×6,210 à K10. La permutation donne ×3,336 et ×3,716 pour Positive/profondeur7 : trois tailles finies ne constituent pas une borne sous-quadratique. Le budget4096 borne l'effort du certificat et renvoie UNKNOWN en cas d'épuisement ; il ne retranche jamais une famille. Aucun census dense complet n'a été lancé.

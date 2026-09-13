# P0 — addition et intersection q2, troisième tranche mono

13 septembre 2026. `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`.
GCP non utilisé. Ces reçus qualifient des composants sur un rectangle
séparé : aucune WSPD complète, expansion des grandes candidates, census
global, tour FULL ou mesure GPU. Aucun résultat v7 n'est hérité.

## Ce qui est livré

Le mode `Additive` cumule les témoins des colonnes exactes disjointes
hors de l'ancre. Sa restriction facultative intersecte le résidu avec
un plan local q2, sans addition de témoins qui pourraient se recouvrir.
Les crédits sont copiés depuis le même propriétaire immuable. Les
rejets locaux et par boîte précèdent les recherches de rang ; les plages
adjacentes sont fusionnées. Le mode `Independent` par défaut est conservé.
Les plans locaux actifs n'allouent plus deux tableaux nuls aussitôt
remplacés ; aucun gain de temps isolé n'est attribué à ce dernier delta.
Voir le [contrat pédagogique](../../docs/P0_ADDITION_ET_INTERSECTION.md).

## Protocole et preuves

26 CTests passent en Release GCC 13.3 et Debug Clang 18.1.3 ASan/UBSan.
La nouvelle gate géométrique confronte 650 plans sur 81 petits cas,
avec 29 946 census ponctuels multiprécision indépendants, 7 contre-modèles,
15 rejets d'API et 2 cas de propriété. Elle contrôle également les grandes
grilles par formule et descripteurs, sans expansion exhaustive. Les anciennes
gates de crédits, partage et affectation après panne mémoire repassent.
La CLI nouvelle couvre 809 scénarios et 198 contrôles de la référence ;
la gate de campagnes réfute 7 mutants du runner et 14 du lecteur, en
Python normal et −O. Ces tests de reçus ne prouvent pas la géométrie.

648 mesures, 576 configurations distinctes en comptant l'ordre :

| Campagne | Contenu | Mesures |
| --- | --- | ---: |
| additive_matrix | 4 familles × 3 tailles × K5/10 × s8/10/12 × 2 ordres, addition seule | 144 |
| intersection_matrix | même matrice, intersections Pool/Dual/Tubes | 432 |
| focus | grille et nappe complète, 3 tailles, K10/s8, Pool, 2 variantes × 2 ordres × 3 répétitions | 72 |

Toutes les invocations sont mono et séquentielles, après la fin des builds
et gates. Même propriétaire pour les deux bras. Les configurations du focus
ont donc quatre mesures par ordre en incluant la matrice initiale ; les
autres en ont une. Pas de chauffe déclarée, pas d'intervalle de confiance.
Les métadonnées de l'hôte partagé sont conservées, sans prétendre l'isoler.
Les sources et le binaire sont vérifiés avant/après ; sorties brutes,
commandes, identités d'entrée et tous les compteurs restent dans les JSONL.
Les trois campagnes ferment avec 648 succès et aucun échec/essai invalide.

Les deux lecteurs normal/−O passent. Ils séparent ordres et variantes,
refusent les mélanges de builds/métadonnées machine et contrôlent les
références indépendantes entre stratégies. Le paramètre s vérifie ici
la précondition des mêmes rectangles : **aucune WSPD s8/10/12 n'est encore
comparée**. RAM/VRAM de pointe et destruction des objets ne sont pas mesurées.

## Résidu et coût : ne pas choisir seulement la comparaison favorable

n=32 000, Kmax=10, s=8. Les plages de temps ci-dessous sont les deux
médianes par ordre, pas un intervalle statistique. Grille et nappe complète :
quatre mesures par ordre ; cas dissymétrique : une mesure par ordre.
Temps des plans seuls, hors génération/propriétaire et inspection.

| Famille | Pool local seul : candidates / temps | Intersection Pool + addition : candidates / temps |
| --- | --- | --- |
| grille 3D | 378 840 / 2,42–2,61 ms | 114 716 / 33,82–35,39 ms |
| nappe complète | 256 000 000 / 2,27–2,45 ms | 3 928 390 / 238,64–243,23 ms |
| facteurs dissymétriques | 144 298 / 2,41–2,56 ms | 28 534 / 20,64–21,16 ms |

Le coût de l'intersection **inclut le plan Pool**, les copies et la
sélection. Le résultat est plus sélectif, mais Pool seul prépare plus vite.
Il reste à mesurer si le census économisé rembourse ce surcoût.

L'ancien filtre axial, dans le même binaire, garde 6 483 670 candidates
sur la nappe complète. L'addition seule les réduit à 3 928 390 (−39,4 %),
mais coûte 237,29–238,14 ms contre 56,45–58,16 ms pour sa référence
appariée. Le nombre de plages passe de 766 418 à 438 362, mais l'addition
paie 38 195 381 comparaisons de rang. **L'addition seule n'est donc pas
une accélération de la sélection sur cette famille.** Aucun gain aval
n'est inventé pour la promouvoir.

Sur la grille, l'intersection réduit simultanément le travail de sélection
par rapport à l'addition seule : 308 362 classifications contre 10 052 730,
et 2 271 251 comparaisons de rang contre 119 725 550. Mais elle reste plus
coûteuse que Pool seul. Dual donne le même résidu final que Pool sur cette
grille et coûte 73,86–79,18 ms. Tubes donne 445 422 candidates et coûte
43,01–44,31 ms. Sur le cas dissymétrique, Dual est un peu plus sélectif
(26 594 candidates) mais coûte 72,70–76,65 ms. Aucun gagnant universel
n'est choisi silencieusement.

## Est-on sous-quadratique ?

Voici les comptes déterministes à Kmax=10, identiques pour s8/10/12.
J inclut les racines classifiées sans requête d'index ; D est le nombre
de plages après fusion. Ces compteurs ne sont pas additionnables entre eux.

| Famille / méthode | n | Candidates M | Classifications J | Plages D |
| --- | ---: | ---: | ---: | ---: |
| nappe complète / addition | 8 000 | 918 160 | 797 168 | 100 864 |
| nappe complète / addition | 16 000 | 1 912 660 | 1 724 728 | 214 748 |
| nappe complète / addition | 32 000 | 3 928 390 | 3 567 584 | 438 362 |
| grille / intersection Pool | 8 000 | 36 960 | 65 108 | 10 918 |
| grille / intersection Pool | 16 000 | 67 660 | 175 726 | 27 112 |
| grille / intersection Pool | 32 000 | 114 716 | 308 362 | 51 344 |

Pour la nappe complète alignée, le résidu a une borne linéaire démontrée
à h fixé. Les autres comptes ci-dessus montrent une croissance nettement
inférieure à ×4 par doublement de n sur cette plage, pas un théorème
global. La nappe tronquée est testée séparément : M=923 110, 1 914 420,
3 928 600 ; sa recette n'hérite pas de la formule de la grille complète.
Le cas dissymétrique avec intersection Pool donne M=9 434, 16 915, 28 534.

La préparation des colonnes est O(|A| log |A|), mais visites, fragments
et résidu ne sont pas généralement bornés sous-quadratiquement. Une
rotation peut encore faire conserver tout A×B. Et ces mesures n'incluent
pas le census ni la reconstruction : **la complexité globale de Morse HGP
3D v8 n'est pas encore qualifiée sous-quadratique**. P0 reste ouverte.

## Suite et rejeu

Le prochain jalon est le census q2 partagé par blocs sur tous les sites,
avec sorties et coquilles exactes, contre une référence de requêtes
individuelles. Le [prototype de l'auditeur](../../../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md)
consomme effectivement les candidates de `8e406f9b` et expose son coût ;
ses données ne sont pas un nouveau benchmark des présentes sources.
Les queues/fenêtres A/B restent une alternative à comparer. Ne pas
prolonger l'optimisation du seul cas axial en la présentant comme générique.

Les répertoires `build/v8_additive_20260913` et
`build/v8_additive_sanitize_20260913` sont épinglés. Les captures antérieures
restent immuables : r3 sur `3589a2c9`, partage/axe sur `8e406f9b`.
Rejouer leur lecteur sur les sources correspondantes, pas les sources courantes.
Le manifeste de chaque campagne contient la commande exacte de reproduction.

```bash
python3 -B morsehgp3D_v8/bench/check_paired_campaign.py morsehgp3D_v8/receipts/additive_q2_20260913 --summary
python3 -B -O morsehgp3D_v8/bench/check_paired_campaign.py morsehgp3D_v8/receipts/additive_q2_20260913
```

Les XML et [QUALIFICATION.json](QUALIFICATION.json) épinglent cette tranche.
Un [export neuf d'index](PUBLICATION_CHECKS.json) repasse les 26 CTests,
avec les mêmes sorties de gates que le Release principal, les 36 pins
source identiques, les deux lecteurs PASS648 et le contrôle documentaire.
Contrats 50k/1s, 100 ms, GPU et dizaines de millions : tous ouverts.

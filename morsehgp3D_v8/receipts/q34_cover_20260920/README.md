# Tranche24 : covers partagés et producteur pour une arête

20 septembre2026, après785d0589, hors registre, `public_status=not_claimed`.
Les151 hashes de sources dans chaque manifeste identifient le delta réel ;
le seul commit de départ n'est pas l'identité des binaires mesurés.
Anciennes captures et builds conservés, GCP non utilisé.

## Captures

| Capture | Périmètre |
| --- | --- |
| `smoke_palw_bei` | Release : trois portes et six sondes32, K5/10 |
| `smoke_exzlsfbc` | Clang ASan/UBSan : les mêmes trois portes et six sondes32 |
| `scale_7ganupv_` | Release sur CPU0 : trois portes et vingt mesures appariées |
| `regression_qbnh7tdm` | 85/85 CTests Release PASS,133,65s |
| `readers_zet2njey` | Dix commandes de lecture normal/−O concordantes,25 mutants lecteur |
| [mutants](mutants/README.md) | Trois perturbations compilées détectées, dix commandes |

La porte nouvelle effectue11 702 contrôles :186 arêtes/covers,184 appels
par face contre le raccord global et l'oracle rationnel,198 candidats,
dont152 q3 et46 q4. Douze petits nuages sont explorés sur toutes leurs
arêtes. Sont exercés : tangence fermée, coordonnées u16 extrêmes,
coquille30, q3 rejeté/q4 conservé, compte partiel différent du global à une
racine non positive, conservation des propriétaires, exceptions de callback,
quatre appels concurrents et tri physique des vues avant normalisation.
Les25 mutations du **lecteur** sont distinctes des trois mutations produit.

Les deux builds sont `build/v8_q34_cover_20260920` et
`build/v8_q34_cover_sanitize_20260920`. Le second couvre trois portes et
six sondes, **pas une nouvelle exécution des85 CTests sous sanitizers**.
Le [préflight](PREFLIGHT.md) est séparé des captures finales.

## Mesures : préparations et sorties réellement payées

Une arête fournie, pas toute la tour ni le générateur WSPD. L'ancien bras
reçoit les seeds connus ; le nouveau les découvre. Chaque bras paie son
propriétaire et sa validation de sorties ; le nouveau paie en plus l'index
global et le cover. Les callbacks collectent réellement supports/coquilles.
Génération du nuage, contrôle indépendant des fixtures et libération commune
sont séparés des temps de bras et inclus dans `total_ms`. L'ancienne sortie
reste en mémoire pendant le nouveau bras pour la comparaison physique.

Temps de bras en ms, K10, **une observation par configuration** :

| Régime / bras | 8k | 16k | 32k |
| --- | ---: | ---: | ---: |
| Fond lointain, ancien | 8,310 | 17,224 | 37,024 |
| Fond lointain, nouveau | 2,984 | 6,077 | 12,910 |
| Fond dans le cover mais hors lentille, ancien | 8,157 | 21,487 | 34,054 |
| Même régime, nouveau | 13,272 | 19,524 | 41,615 |

Avec le fond lointain, le cover contient6 sites et les deux faces ne font
que12 lectures initiales, contre2n visites familiales de l'ancien chemin
(plus son census q3). Le nouveau temps est surtout la construction de
l'index :2,374 /4,861 /10,384ms. Le temps du producteur déjà préparé vaut
environ0,007–0,008ms ; **ce n'est pas un temps de tour**.

Quand m=n mais S=2, les lectures font16 000 /32 000 /64 000. La préparation
de l'index n'est pas amortie sur cette unique arête ; aucun gain stable
n'est établi. Pour K5, nouveau/ancien vaut1,224 /1,226 /1,334 ; pour K10,
1,627 /0,909 /1,222. Les fluctuations sont conservées, pas transformées en
gain général. Les tris nouveaux font252 949 /534 944 /1 148 634 comparaisons.
Les objets étant communs à tout le pipeline, leur amortissement réel sera
à mesurer au raccord global, sans soustraire fictivement leur préparation.

## Contre-régime : le carré reste avéré

| n | Faces S | Sites m | Lectures S·m | Comparaisons de tri |
| --- | ---: | ---: | ---: | ---: |
| 32 | 30 | 32 | 960 | 2 550 |
| 64 | 62 | 64 | 3 968 | 28 438 |
| 128 | 126 | 128 | 16 128 | 134 035 |
| 256 | 254 | 256 | 65 024 | 620 216 |

Rapports des lectures :×4,133 /×4,065 /×4,032. Les tris croissent également
au-dessus du quadruplement. Pourtant les sorties restent **14 à K5 et54
à K10**, pour les quatre tailles. Ce coût n'est donc pas justifié par la
taille de la sortie de cette fixture. L'ancien et le nouveau chemins
produisent exactement les mêmes enregistrements, profondeurs et coquilles.

Le petit adversaire est une fixture bornée, pas un plafond de l'algorithme.
Sa validation à n256 est différentielle contre le raccord global qualifié ;
l'oracle rationnel indépendant reste limité aux petits nuages de la porte.
Les grands fonds ont trois boules attendues explicites et un contrôle
scalaire sur chaque point. Aucun oracle exhaustif de production caché.

**Conclusion :** gain local et accès complet pour une arête fournie ; pas
de borne globale sous-quadratique. Ne pas généraliser S scans de m témoins
sur toutes les arêtes. Le travail collectif entre familles est la prochaine
priorité. GPU, FULL, contrats50k/G4 et dizaines de millions restent ouverts.

## Rejouer les lecteurs

```sh
python -B morsehgp3D_v8/bench/run_q34_cover_checks.py read morsehgp3D_v8/receipts/q34_cover_20260920/scale_7ganupv_ --check-live
python -B -O morsehgp3D_v8/bench/run_q34_cover_checks.py selftest morsehgp3D_v8/receipts/q34_cover_20260920/scale_7ganupv_
```

Le lecteur contrôle plan/commande/reçu, sorties brutes, sources, artefacts,
clôture et identités de travail. L'analyse de croissance publie tous les
compteurs géométriques, préparations, tris, sorties, capacités et temps,
y compris les rapports supérieurs à quatre. La capture des lectures
`readers_zet2njey` ferme151 sources,59 artefacts et52 entrées de vérification.
Les deux builds sont désormais épinglés ; ne pas les réutiliser pour
compiler la tranche suivante.

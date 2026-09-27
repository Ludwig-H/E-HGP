# Contrelecture de la capture synthétique

27 septembre2026. Les lecteurs normal et `-O` passent et produisent des
JSON **bit-identiques**, SHA256
`c91166d738aac45e17df2850419863daec6843e2927337f464edfa608db62e48`.

Reçus :

- `/tmp/mhgp9-synthetic-post-audit-20260927-r1-normal/audit.json`
- `/tmp/mhgp9-synthetic-post-audit-20260927-r1-optimized/audit.json`

La capture scientifique fermée est
`/tmp/mhgp9-synthetic-quality-20260927-r1/receipt.json`, SHA256
`2a7e9fb49404a553e14abd520852c779671cb2506ae92993e33e909854d5a7e0`.
Les empreintes couvrent les sources, entrées, preuves antérieures et sorties
consultées :2 148pins LIVE. Toutes les34unités et leurs groupes de processus
sont clos ;612lignes, sans omission ni doublon. Le moteur n'a pas été modifié.

| Contrôle par passage | Nombre |
|---|---:|
| ARI rationnels et scores de labels relus | 612lignes |
| Arbres ponctuels explicites | 68 |
| Sélections ponctuelles exclusives et leurs condensations | 136 |
| Condensations première couverture et HDB commun | 272 |
| Fits HDBSCAN liés aux paramètres et sorties | 68 |
| Commandes natives et workers clos | 34chacun |

Les ARI sont recalculés avec une arithmétique rationnelle indépendante des
métriques de production. Couverture, bruit et F1 sont vérifiés depuis les
contingences entières. L'optimiseur hongrois reste partagé ; NMI n'est pas
recalculé indépendamment. Le vote pondéré conserve m comme masse de facettes,
sans fausse garantie de cardinalité après vote. Les autres voies contrôlent
effectivement cette cardinalité.

Les structures ponctuelles, les dates et les sélections condensées sont
relues. Pour les coupes échantillonnées, seules les dates et nombres de blocs
étaient archivés ; aucun hash de partition absent n'est inventé. La comparaison
aux coupes du routage a été effectuée dans le worker scientifique. Le lecteur
ne relance ni géométrie, ni routage, ni masses/vote, ni condensation/EOM ou fit.
Ce n'est donc pas une deuxième preuve indépendante de tous les algorithmes.

Le lecteur passe six tests normal/−O :120contingences aléatoires, cas de bruit,
mutations de métriques et de journal de processus, seuils distincts et quatre
sélections ponctuelles. Le formateur passe sept tests/17refus normal/−O.
Ces tests de lecture/publication sont séparés des40gates préalables de
la [qualification](QUALIFICATION.md). Aucun nouveau calcul natif dans ces tests.

La publication compacte conserve612scores,306agrégats,136comparaisons
appariées et12diagnostics de taille. Les données volumineuses restent sous
`/tmp` ; les reçus publiés ne sont pas une archive autonome. Aucune qualification
GPU,100ms ou sous-quadratique globale n'est revendiquée. GCP non utilisé.

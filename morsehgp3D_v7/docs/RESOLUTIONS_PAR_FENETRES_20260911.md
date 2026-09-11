# Résolutions par fenêtres et certificats de connexion

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype mono-thread, distinct du moteur actif. GCP non utilisé.

## Ce que change le calcul en flux

Le [raccord par graphes](../receipts/atlas_graph_full_20260911/README.md)
reconstruisait déjà toute la tour FULL, mais conservait les terminales de
tous les représentants et les graphes complets. Le nouveau producteur
développe seulement une fenêtre de demandes, résout ses facettes distinctes
et réduit immédiatement les arêtes en un certificat de connexion daté.

Il conserve un seul pivot par bloc. Après résolution de ces pivots, il
projette le certificat sur les naissances et reconstruit les histoires,
les contributions datées et les images verticales. Les dates d'émission
des arêtes et les admissions des marques ne changent pas.

Sont retirés : le tableau global des terminales, les demandes développées
de tout un ordre et les graphes complets. Restent : le catalogue, les
masques compacts des représentants, les métadonnées des blocs, les semis
complets, les certificats intermédiaires et la sortie FULL. La réduction
des buffers de demandes ne garantit donc pas, à elle seule, une diminution
du pic mémoire du processus.

La fenêtre est un choix de résidence, pas un plafond d'itérations. Elle
n'interrompt aucune descente géométrique et ne change ni K ni le facteur
s de séparation WSPD.

## Pourquoi le résultat reste le même

La résolution utilise le même index immuable, le même catalogue complet
et les mêmes semis complets à chaque fenêtre. Sa clé contient toute la
facette, pas seulement le support de sa boule minimale. Le consommateur
le plus ancien est choisi pour une clé répétée dans la fenêtre ; chaque
autre occurrence vérifie séparément l'antériorité de la terminale.

Le certificat minimal conserve les connexions à toutes les coupes, pas
seulement la composante finale. Les marques et contributions restent
séparées des arêtes éliminées. La preuve de
[composition des certificats](../audits/receipts_composable_msf_20260911/README.md)
s'applique avant leur projection ; celle-ci conserve les dates originales.

La gate compare directement chaque terminale au témoin global, puis les
certificats à leurs graphes et la tour au Builder et à l'oracle T2 borné.
L'égalité physique est exigée entre les voies du nouvel encodage et les
consultations CPU1/4. La comparaison au Builder historique reste une
bijection explicite, sans prétendre reproduire ses indices bruts.

## Travail répété et mémoire à mesurer

Une facette présente dans plusieurs fenêtres peut être résolue plusieurs
fois. Le nombre d'uniques locaux cumulé ne doit pas être présenté comme un
nombre d'uniques global. Pour K≥2, avec G démarrages hors semis initial,
Q échanges et H succès du semis après échange, les compteurs vérifient
`MEB = G + Q − H` et `H = terminales après échange`.

Les quatre vecteurs de la fenêtre ont des tailles et capacités mesurées
séparément. La pile de certificats, les domaines et la sortie ne rentrent
pas dans ce budget. Le RSS est mesuré par processus ; le mode de comparaison
garde deux tours et ne sert pas de mesure mémoire par voie.

Le benchmark inclut index, génération, census, validation commune et
reconstruction jusqu'à la tour retenue. La synthèse du nuage, les digests
et la comparaison sont chronométrés séparément. Aucun oracle exhaustif ni
ensemble des descendants n'appartient à ce benchmark.

### Résultat mono n8000 : pas de gain de performance

Le [paquet de qualification et de mesures](../receipts/streaming_graph_20260911/README.md)
ferme O2/SAN (114 census, 456 essais et 253 224 terminales par build), puis
les sondes n200/400/800 et deux processus séparés à n8000, s8, K1..10.
À n8000, les digests et compteurs FULL sont identiques ; le RSS comprend
un seul bras par processus, validation incluse.

| Voie | Tour complète (s) | MEB | RSS (KiB) |
| --- | ---: | ---: | ---: |
| Référence matérialisée | 188,638 | 3 947 627 | 2 731 664 |
| Flux W=65 536 | 250,408 | 4 359 540 | 2 770 676 |

Le flux retire bien les gros tableaux visés, mais il n'accélère pas cette
paire et ne réduit pas son pic RSS. Ses quatre buffers de fenêtre font
6 553 600 octets ; la pile retraite 48 390 815 arêtes pour 10 456 312
arêtes source, avec 318 compactions. Ce résultat négatif motive une
correction du réducteur mono avant de prolonger cette variante à 16k/32k
et s10/s12 à grande taille. Aucun temps historique n'est transféré.

## Suite du raccord

L'[encodage historique validé par l'auditeur](../audits/receipts_historical_export_20260911/README.md)
peut se reconstruire après les histoires : minimum BallKey par groupe
fermé pour leur ordre, minimum distinct par lot K pour les fractions
brutes. Tous les blocs participent, silencieux compris. Cette passe n'est
pas encore intégrée à la voie mesurée.

Le calcul géométrique, la composition des certificats et la reconstruction
des histoires restent séquentiels dans ce prototype. Leur distribution
massive et le coût final de l'export sont les étapes suivantes. Aucune
promesse universelle sous-quadratique en points ne se déduit d'un stockage
réduit : la taille des naissances et de la sortie explicite doit elle-même
être prise en compte. Les contrats 50k/1 s, 100 ms et plusieurs dizaines de
millions de points sur G4 restent ouverts.

## Correctif ordonné qualifié séparément

Le [nouveau paquet](../receipts/ordered_streaming_20260911/README.md)
qualifie maintenant Kruskal incrémental sur l'ordre déjà croissant des
arêtes. Il initialise un seul DSU **pour les hubs de chaque K**, conserve
un certificat et ne retrie plus les arêtes à chaque fenêtre. La compaction
native finale reste exécutée et comptée séparément : ce n'est pas un
retrait de tous les tris ou DSU de la tour.

Les 114 census O2/SAN conservent FULL, contributions et verticales. Les
82 368 pivots sont identifiés et retenus ; leur projection ne supprime que
ces pivots en boucles et reste forestière. Les 54 612 comparaisons d'arêtes
entre fenêtres vérifient le déterminisme du nouveau certificat. Le juge
abstrait distingue les coupes ouvertes/fermées, les égalités de naissance,
les sommets futurs/isolés et les refus d'ordre, puis l'empoisonnement de
l'objet après erreur. Aucun ancien verdict n'est simplement transféré.

À n8000/s8/K1..10/W65536 : 10 456 312 visites au lieu des 48 390 815
de la pile, zéro tri d'arêtes de hubs, 2 404 636 arêtes présentées au
compact natif. Même digest FULL et 4 359 540 MEB. Le total observé vaut
207,867 s, le RSS 2 769 680 KiB ; le retrait des retris ne suffit donc
toujours pas à revendiquer le contrat 50k ou une baisse de mémoire.

Le triplet complet est maintenant enregistré dans le paquet : à 16k,
595,244 s, 21 948 186 occurrences et 9 364 101 MEB ; à 32k,
1 076,969 s, 45 453 599 occurrences et 19 784 213 MEB, pic 11,07 Gio.
Les volumes croissent près du linéaire sur ce nuage uniforme/s8 ; aucune
preuve sous-quadratique tous régimes ni extrapolation de latence depuis
ces mesures sur hôte partagé. Les comparaisons n800/s8/10/12 donnent
le même digest dense, les mêmes comptes par K et le même travail géométrique.

La [contraction proposée avec l'auditeur](CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md)
est maintenant qualifiée séparément : les pivots sont contractés **pendant**
leur consommation et le DSU ne porte plus que les naissances. Les identités
restent stables après union ; leur conversion en BlockId attend la fin de K.
Les hubs, recherches binaires d'extrémités, certificat intermédiaire et
projection/compaction finale disparaissent de ce nouveau chemin, pas des
témoins historiques ci-dessus. O2/SAN et mono8k sont clos : même sortie,
186,354 s et 2 752 852 KiB, sans gain statistique ou RSS majeur revendiqué.
Le raccord suivant distribue les groupes géométriques à des workers CPU
persistants, puis restitue l'ordre source. Il est qualifié O2/SAN, mais
préparation et reconstruction restent séquentielles. Le moteur actif et
les contrats GPU ne sont pas promus par ces qualifications privées.

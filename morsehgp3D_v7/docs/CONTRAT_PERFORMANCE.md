# Contrat de performance v7

Consigne utilisateur du 4 septembre 2026 : l'exactitude et le gain
d'optimisation sont deux obligations. Priorité **mono-thread, multi-CPU,
puis GPU**. Le premier objectif porte sur **50 000 points, toute la tour
K=1 à 10, moins d'une seconde**. **Toute la tour K=1 à 5** est le repli
si la tour jusqu'à 10 ne satisfait pas ce délai, avec la même exactitude.
Une fois le jalon d'une seconde validé, **la cible suivante est 100 ms**
pour le même périmètre de tour déclaré, sans relâcher l'exactitude.
Ce document ne rapporte aucun objectif atteint.

Dernières tours 50k complètes, le 10 septembre : [G4 SPOT CPU/hybride](RESULTATS_TOUR_CACHE_G4_20260910.md),
418,873 / 418,921 s pour K1..10 et 33,853 / 33,569 s pour K1..5.
La route hybride n'accélère que prefilter/census ; FULL reste CPU dans ces
captures. Aucun contrat 1 s/100 ms ou plusieurs dizaines de millions acquis.
Les deux générations de VM utilisées sont closes, arrêts ciblés certifiés.
Les [refus du 6 septembre](RESULTATS_G4_FULL_20260906.md) restent historiques ;
leurs 21,372 / 5,646 s ne sont pas des temps de tour. Le nouvel observateur
des quatre blocs nommés 50k reste un contrôle distinct des digests globaux.

Le 11 septembre, la [résolution statique CPU](RESOLUTION_STATIQUE_CPU_20260911.md)
est intégrée en option, avec MEB/supports réduits sur des cas appariés.
Ses tests locaux et sa compilation CUDA ne réattribuent pas les temps 50k
précédents et ne qualifient pas un backend de résolutions GPU. GCP non utilisé
pour cette étape ; le chemin nominal reste sélectionné par défaut.

Le statut demeure `public_status=not_claimed`, profil d'entrée u16.
La cible de 100 ms du plan transverse est ainsi conservée comme jalon
suivant, et non comme premier délai à valider. Les rattachements certifiés,
la verticale et le supplément pondéré déclaré ne sont pas supprimés pour
gagner une mesure de temps ; conserver leur effet ne signifie pas
matérialiser leurs cofaces silencieuses ou Gamma exhaustif.

## Périmètre et preuve

L'utilisateur a confirmé explicitement le périmètre de tour 1..K :
`smax=11` pour K=10 et `smax=6` pour le repli K=5.
Ne pas assimiler silencieusement une hiérarchie d'ordre isolé à cette
tour, ni une sortie Gabriel `verified_events_only` au résultat HGP complet.

L'[audit du certificat FULL](AUDIT_NIVEAUX_GABRIEL_20260905.md),
[contrelu indépendamment](../audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md),
fixe la cible topologique régulière : minima Gabriel de cardinal K avec
leurs points et niveaux, vraies multifusions aux niveaux Gabriel de cardinal
K+1 et parents certifiés. Leurs unions descendantes restituent les couvertures,
isolés inclus. Le [lecteur structurel livré](CONTRAT_CERTIFICAT_FULL.md)
`src/forest/full_certificate.hpp` n'est pas, à lui seul, un constructeur
complet ni une porte de performance.
Le [producteur FULL horizontal historique](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
calcule les parents relativement aux catalogues Gabriel fournis. Ses mesures
par ordre n'incluaient pas une tour conservée. Le [raccord par boules actuel](TOUR_FULL_PAR_BOULES.md)
retient maintenant forêts datées, terminal K=n et verticales adjacentes,
y compris les plateaux ; son autorité reste relative au census exact complet.
L'archive industrielle et le supplément pondéré restent des obligations
distinctes, non certifiées par une mesure de cette sonde.
Le produit F et ses mesures réduites ou `verified_events_only` conservent
leur portée historique ; aucune de ces mesures ne qualifie ce nouveau payload.

Le profil pondéré doit préciser l'univers de facettes contributrices,
les scores, la date d'affectation de leur masse et la convention de
condensation. Les minima FULL ne remplacent pas automatiquement ces feuilles.
Comparer des timings de profils pondérés différents n'est pas un gain
algorithmique à objet constant. Ce supplément n'exige pas par principe
l'univers de toutes les facettes Gamma.

Chaque résultat nomme la sémantique du payload, les ordres réellement
publiés, le backend et le nombre de threads. Un refus de dégénérescence,
un cap, une censure ou une sortie partielle n'est pas une réussite du
contrat. Le temps complet inclut les étapes nécessaires au payload ;
les temps de génération, census, tri, fold et export sont également
rapportés pour guider les optimisations. Le coût du digest diagnostique
est isolé, jamais retranché sans déclarer la commande mesurée.

La porte de répétition du contrat précédent est conservée : deux
échauffements puis dix nuages frais par famille, p95, mémoire RAM/VRAM,
et preuves d'exactitude rattachées aux sources et binaires. Aucun speedup
n'est déduit de mesures sur un hôte concurrent non contrôlé. Les runs
appariés déjà engagés à huit threads restent des diagnostics historiques,
pas une qualification mono-thread.

## Ordre d'optimisation

La [sonde historique v5](CONTRAT_SONDE_FULL_MEB.md) a retiré les quotas
arbitraires d'opérations ; la sonde de tour par boules conserve cette absence
de plafond de travail. Les admissions RAM et limites
de représentation restent distincts ; augmenter l'admission ne constitue
pas une accélération. Les triplets 8k/16k/32k doivent comparer des runs
complets de même profil, ainsi que le volume des minima et le travail
intermédiaire. Une croissance locale sous-quadratique n'est pas une preuve
universelle : [la sortie FULL peut elle-même être quadratique](CROISSANCE_ET_BORNE_DE_SORTIE.md).

La [réduction aux minima](SQUELETTE_MINIMA_GABRIEL.md) conserve la cible,
à condition de transférer les bonnes connexions. Elle n'autorise ni un
graphe d'intersection naïf sur ces minima ni la perte des naissances d'un
ordre supérieur sous prétexte qu'elles sont des no-op dans l'ordre inférieur.

1. Établir le chemin mono-thread réel, y compris le fold ; `threads=1`
   ne suffit pas si un autre étage tourne en parallèle. Conserver un
   témoin v6 et un témoin v7 avant chaque optimisation, sur les mêmes
   nuages et avec des objets canoniques identiques lorsque leur
   sémantique est identique.
   Comparer explicitement la séparation WSPD **s=8, s=10 et s=12**,
   sur les mêmes nuages et graines : temps, RAM, volumes intermédiaires
   et égalité des tours. Distinguer ce paramètre de `smax=K+1` ; aucun
   changement d'ordre ou de payload ne doit se cacher dans le réglage s.
2. Mesurer les étapes dominantes et les volumes intermédiaires. Réduire
   d'abord le travail, les copies et les parcours, avec une fixture ou
   un mutant pour chaque changement sémantique. Ne pas remplacer le
   constructeur par son oracle exhaustif.
3. Mesurer ensuite la montée en charge CPU sur le même travail, en
   distinguant gain algorithmique et parallélisme.
4. Qualifier enfin les primitives puis la route GPU sur G4 protégée.
   Un stub ou un calcul CPU exécuté dans une VM GPU n'est pas un résultat
   GPU. Aucun démarrage ne précède les deux coupe-circuits.

## Échelle massive

La cible massive explicitement demandée est **GCP G4**, pour des nuages
de plusieurs dizaines de millions de points. Les paliers 10 000 001, puis
30, 50 et 100 millions de points demeurent distincts du contrat 50k.
Ils exigent des budgets RAM/VRAM, des index et une
résidence compatibles, ainsi qu'interruption/reprise du moteur. Ni une
archive atomique ni une extrapolation linéaire depuis 50k ne les valide.

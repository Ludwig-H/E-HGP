# Contrat de performance v7

Consigne utilisateur du 4 septembre 2026 : l'exactitude et le gain
d'optimisation sont deux obligations. Priorité **mono-thread, multi-CPU,
puis GPU**. Le premier objectif porte sur **50 000 points, toute la tour
K=1 à 10, moins d'une seconde**. **Toute la tour K=1 à 5** est le repli
si la tour jusqu'à 10 ne satisfait pas ce délai, avec la même exactitude.
Une fois le jalon d'une seconde validé, **la cible suivante est 100 ms**
pour le même périmètre de tour déclaré, sans relâcher l'exactitude.
Ce document ne rapporte aucun objectif atteint.

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
Le [producteur FULL horizontal](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
calcule ensuite les parents relativement aux catalogues Gabriel fournis.
Même une exécution des ordres 1..K de ce composant ne qualifie pas la tour
intégrée : la verticale, l'autorité terminale, le profil pondéré déclaré
et la publication ne sont pas encore raccordés.
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

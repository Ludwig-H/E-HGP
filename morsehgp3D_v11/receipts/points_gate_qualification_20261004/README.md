# Porte stricte FULL → points — contrelecture close du 4 octobre 2026

Publication auditée : ab1a739d17f801823a66d74609209696152c8705.
Cadre : exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed. Lecture de sources et reçus existants uniquement : aucun moteur,
build, fit, workflow ou GCP lancé. Les archives originales copiées ne contiennent
pas de coordonnées, masques ou labels bruts KITTI.

**Qualification acquise, F/claudepts6.** Commit poussé exécuté f02f91c7e,
worker 0, toutes les étapes OK, arrêt ciblé certifié et TERMINATED dans la même
génération. Porte conforme : 2 854 nuages, 194 520 comparaisons, 215 974
comparaisons de sites en rayon, 12 fixtures et quatre mutants. Les dates et
propriétaires sont comparés exactement avec l'oracle. Le plateau de propriétaire,
les deux triangles, cinq et huit sites, égalités de radicaux et annulation du
filtre font partie des fixtures. La porte est bornée : au plus neuf sites,
k≤4, m∈{1,k+1,k+2} avec m≤n ; les sites sont comparés plusieurs fois.

**Échec conservé, E/claudepts5.** Commit 6c88fe0e ; porte 1 après 300,203 s,
FileNotFoundError dans gate/mutants/qualification_decalee lors de l'écriture de
l'entrée. Les trois campagnes refusent ensuite avec code 2 et ne produisent
aucun cas mesuré. Ce n'est pas un défaut arithmétique natif. f02f91c7e ajoute
uniquement mkdir à run_export ; F joue et valide cette correction. E est aussi
arrêtée avec certificat ciblé.

**Provenance.** Les 5 385 fichiers réguliers de chacun des paquets E/F ont été
comparés au contenu de leurs blobs Git respectifs. Les paquets complets ne sont
pas dupliqués ici ; hashes et inventaires comprimés sont conservés. Les sources
sélectionnées réellement jouées par F sont identiques à la publication ab1a.
L'ELF exportateur déclaré cfdc5450… est identique D/E/F, GCC 11.4 Release.
Tous les payloads des résultats sont rehachés et inventoriés exactement :
D338, E77, F283. Sources/reçus/plans/archives locaux sont restés inchangés entre
les lectures ; les pièces publiées et locales E/F sont identiques.

**Résultats.** F persiste 205 noms de cas OK : 128 synthétiques, cinq démos et
72 voisines, mesurés à k=2,3,5,10. Les 201 noms communs D/F ont exactement les
mêmes JSON après retrait exclusif de seconds, wall_seconds, full_ns, export_ns.
L'égalité concerne les observables persistés, dont les IoU arrondis à six
décimales, pas toutes les décisions internes ou un dump FULL canonique.
c08_000882 et démo02 ont le même hash XYZ et les mêmes IDs d'objets retenus :
compté comme démo, il reste 71 voisines/859 observations d'instances. Ces
observations sont corrélées entre trames. Le hash de tout le tableau de labels
généré des voisines n'est pas retenu dans leurs métadonnées : aucune égalité
complète de ces tableaux n'est prétendue ici. Moyennes et sauvetages/pertes
recalculés dans [result_normal.json](result_normal.json).

**Précision utile à la documentation.** Les « quatre mutants tués nativement »
de REPONSE_CLAUDE_POINTS sont quatre patches Python du consommateur avec un
exporteur C++ inchangé. Trois sont tués par fixtures et désaccords avec l'oracle ;
coupe_ouverte est tué par l'invariant propriétaire_non_vivant:margin_r. Aucun
mutant C++ ou port natif PointRadiusDate n'est qualifié par ce lot.

La qualification reste celle du prototype C++ exporteur + Python consommateur,
sur CPU G4 compilé u21. Aucun GPU, domaine entier u21/u24, condensation,
sélection, supériorité générale ou contrat de 100 ms n'en découle.

[review.py](review.py) est autonome : python3 -B review.py ou python3 -B -O
review.py, sans import produit. Deux replays : 4 343 contrôles, sorties identiques.
Les deux premières erreurs de schéma du lecteur d'audit sont préservées sous
history/ ; elles ne sont pas des erreurs du produit ou de la campagne.
SHA256SUMS racine inventorie tous les fichiers sauf lui-même, LEDGER inclus.

# Réponse CUDA et mesures locales de l’étage G

7 octobre 2026. Lecture seule après la publication de G ; aucun build, banc,
appel CUDA/cloud ou accès aux données LiDAR. Sources, hashes, tailles et extraits :
[capture.json](capture.json).

**CUDA : correction locale des deux sorties mémoire identifiées.** Le prototype passe
pour `device_cuda.cu` de `620c840d…` à `5e215fe2…` : `grow` libère désormais `fresh`
avant de rendre une erreur de copie/synchronisation ; `stage` réinitialise la réservation
épinglée avant son retour sur échec d’allocation. Le [diff minimal](cleanup.diff) reconstruit
exactement l’ancien hash complet à partir des extraits audités. Le développeur a retenu
un nettoyage explicite, différent de la [proposition RAII](../cuda_proposition/README.md).
Les branches identifiées sont corrigées **par lecture statique** ; aucune validation
runtime ni clôture générale de mémoire. Les échecs de libération restent ignorés :
nettoyage best effort si le pilote est déjà défaillant.

**Compteur toujours absent.** `device_pipeline.hpp` (`318fe91d…`), `device_driver.hpp`
(`dd04fcbf…`) et `device_leaves.hpp` (`bb47089f…`) sont inchangés. `leaves_rewritten`
n’est toujours pas alimenté sur la voie appareil, malgré les réexécutions GPU au-delà
de 64 émissions et celles des reprises CPU. Leur temps appartient au chrono global.

**Des temps locaux G existent désormais ; aucun nouveau temps G4 intégré et qualifié
comparable.** Le [rapport développeur](../../developpement_20261007/tour_G_RAPPORT.md)
annonce sur ng00 : K5, un fil, G **2,83 s** dont résolution **2,67 s** ; K10, trois fils,
G **10,1 s** dont résolution **9,56 s**, tables **385 ms**, RSS **1,74 Go**, catalogue
compris pour la mémoire. Ce sont des observations locales historiques sur machine
partagée ; cet audit ne les a pas rejouées. Les portes de correction annoncées restent
bornées, sans transfert à FULL ni au contrat de latence.

`bench/tower_probe.cpp:187–208` construit index, Pool et catalogue avant le chrono ;
celui-ci encadre `resolve_tower`, puis digest/export et destruction du résultat sont
hors chrono. Il inclut la reconstruction des cellules/tables, la résolution et les
contrôles, sans les étages T/M/V/R. **Ne pas additionner ce G local au catalogue G4.**

La construction des tables est déjà parallèle **entre ordres** (`stage.cpp:240–256`),
chaque table restant séquentielle dans son ordre. Les 37/385 ms du rapport sont des
mesures historiques : ni nouvelle mesure après cette intégration ni gain n’en découle.
La somme MES-M7 locale de 2,344 s porte sur les ordres 2–5, tandis que la résolution
produit inclut l’ordre 1. À l’ordre 5, les valeurs rapportées sont 1,58 s contre
1,255 s (environ +26 %), mais sans prises appariées ; les diminutions de comptes ne
prouvent pas un gain de temps. Le profil du produit reste à mesurer.

```sh
sha256sum -c SHA256SUMS
```

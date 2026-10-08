# T2-d-C : préciser le pic mémoire publié

8 octobre 2026, reçu développeur `27eca166b`, campagne `02b735d6b` contre `902041f66`.
L’en-tête « pic de l’hôte » du tableau K5 désigne en fait **le pic du budget commun hôte + appareil**.
La sonde construit un seul `MemoryBudget`, puis appelle `CatalogueDevice::open(budget)` : cette surcharge
appelle `open(budget, budget)` dans `device_cuda.cu:285`. Les réservations appareil et celles de l’hôte débitent
donc le même objet. Les deux versions de la sonde ont cette propriété.

Les valeurs publiées **994→907 / 828→754 / 1 011→922 Mo** restent inchangées. Elles ne permettent pas de
conclure que le seul pic physique hôte ou le RSS a baissé de 9 %. La mémoire épinglée demeure un compteur
distinct, inclus dans ce budget. Cette précision ne change ni le temps C, ni le verdict d’adoption, ni les
[mesures admises](../session_t2dc_admission/README.md). Le RSS n’est pas ce budget compté.

`erratum.patch` propose uniquement un fichier d’erratum à côté du reçu développeur, sans réécrire ses
primaires ni son tableau historique. Le patch n’est pas appliqué au produit. Les quatre blobs Git sont
épinglés dans `capture.json` ; lecture source et contrôle d’application seulement, aucun moteur exécuté.

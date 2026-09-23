# Erratum du reçu G4 R6

23 septembre 2026, relevé par le contre-audit B (`bc76a597`). Le `README.md`
de ce reçu écrit « Objets identiques ON/OFF dans chaque paire (générateur,
catalogue, ordres, condensé) ». « Générateur » est trop large : le compteur
`q34_cover_builds` de la section `generator` change par construction avec le
noyau (08/000000/K5 : 900 377 covers ON contre 2 043 612 OFF). Sont identiques
dans chaque paire : les émissions q2/q3/q4 et les masses de candidats du
générateur, le catalogue, les ordres, le travail FULL et le condensé. Les
trois compteurs du cache témoin varient légèrement entre répétitions, sans
changer l'objet.

Ce fichier est ajouté après publication ; il ne figure pas dans `SHA256SUMS`,
qui reste celui du reçu tel que publié.

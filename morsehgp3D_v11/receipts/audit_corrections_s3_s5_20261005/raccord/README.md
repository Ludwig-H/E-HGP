# Raccord de la porte S3 Fraction et temoin K10

WIP L1b `19b2fb218`,5octobre2026 ; six empreintes de gate/sonde/temoin/docs dans `source_manifest.json`, portee dans `review.json`. Les37 fichiers `src/tower` restent identiques a la capture precedente : aucun delta moteur, count/fill ou budget a recapturer.

Le raccord CMake appelle le vrai `attach_probe` a W1/W3, avec bits, ligne et planchers explicites. La sonde prepare Cat_K puis construit uniquement l’ordre K. Les refus, arrêts, sorties incompletes et divergences alimentent les echecs du juge ; la seule couverture imprimee ne peut produire un succes. Le temoin de12 sites s’execute dans le pilote Fraction a K1..12 et dans `square_k10` a K9..12, serie/lots, avec compteur final des boules effectivement jugees.

Aucun nouveau faux succes ni defaut de capacite/budget significatif etabli par cette lecture. Les nouvelles docs gardent la qualification G4 en attente. Logique Fraction contre-lue par l’autre auditeur. Aucun build, test natif, GPU/GCP ou benchmark execute ici ; aucune qualification transferee.

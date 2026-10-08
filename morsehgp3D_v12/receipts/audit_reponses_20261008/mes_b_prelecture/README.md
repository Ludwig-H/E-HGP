# MES-B : contrelecture avant campagne massive

8 octobre 2026, 03:43 UTC. Copie du développeur encore non commise, base `bec107f7d` ;
pilote `6255e2cafe4210e7caaadd522c40992c70f287a6ecc4334c946a8324dee98f81`.
Les cinq sources sont épinglées dans `capture.json`. Audit Python/source uniquement :
aucun moteur compilé ou exécuté, aucune donnée de scène lue, aucun appel GCP.
Complément de CST-0018 ; aucun nouveau temps ni qualification de produit.

## À corriger avant lancement

1. **Boucle d'étiquette.** `label_of` tronque à droite après avoir préfixé un compteur.
   Pour un nom d'au moins 23 caractères déjà rencontré, tous les préfixes disparaissent :
   la boucle ne peut finir. `main` l'appelle pour chaque cas, donc la même scène à K5 puis
   K10, ou sur GPU puis CPU, suffit. Une collision de suffixes entre scènes suffit aussi.
   Le témoin est un nom inventé ; seul le sous-processus d'audit est arrêté après 0,3 s.
   Correction : réserver d'abord la place du préfixe dans l'étiquette de 23 caractères.
2. **Refus oublié par le verdict.** Une scène K5 réussie à 1 s/million puis une scène K10
   refusée avant sa première passe donnent B1/B2 tenus, B4 non évalué et bilan **tenu**.
   Même défaut pour une scène K5 de 12 millions refusée : B1 l'omet et B2 l'exclut par taille.
   B1/B4 doivent compter les cas lancés en refus/échec ; seul `non_joue` est hors mesure.
   Cela suit leurs quantificateurs « chaque scène jouée » ; le point de rupture reste publié.
3. **Admission JSON encore incomplète.** Quatre corruptions sont acceptées : clé `cpu_ns`
   répétée, `pass=false` dans la libération 0, ouverture `ok/device_fault`, entier 2⁶⁴.
   Le patch refuse doublons/constants non JSON, borne les entiers u64 et vérifie ces deux
   lignes de contrôle. Il ne prétend pas qualifier toutes les combinaisons statut/raison,
   l'intégrité des données d'entrée ou toute la future campagne.

`corrections.patch` est une proposition applicable uniquement au pilote épinglé, sans
changement du moteur. `source_excerpt.py` conserve littéralement les sept fonctions et dix
constantes concernées ; `fixed_excerpt.py` porte les corrections. `check.py` rejoue les
contre-exemples, puis vérifie facultativement l'application du patch à la source entière :

```sh
python3 -B check.py --pilot /chemin/morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py
python3 -B -O check.py --pilot /chemin/morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py
```

## CPU et mémoire : portée des nouveaux champs

`full_probe.cpp` ajoute CPU utilisateur+système de tous les fils autour de `run_wall`.
Le RSS est un **maximum depuis le lancement du processus**, pas le pic isolé de cette passe :
entrées et validation/empreinte des passes antérieures peuvent le fixer. Les appels système
encadrants ajoutent un faible coût au delta CPU ; celui-ci n'est pas exactement la fenêtre
horloge interne. Aucun CPU·s de K n'est reconstitué à partir de ces champs futurs.

`process_cpu_ns()` rend actuellement zéro si `getrusage` échoue : un échec au second appel
produit un sous-flux u64 (exemple : 0−10⁹ devient 18 446 744 072 709 551 616 ns), un échec au
premier publie le CPU absolu. Le RSS indisponible est également présenté comme zéro. Rendre
la disponibilité explicite, ou refuser la mesure, avant la soustraction ; vérifier cpu1≥cpu0.
Aucun échec de cet appel ni faux chiffre réel n'a été observé. Ce point est séparé du patch.

La nouvelle forme `CatalogueDevice::open(hote, appareil)` réserve les tableaux CUDA dans
le budget appareil et le staging épinglé dans le budget hôte ; `open(b)` conserve `open(b,b)`.
La durée de vie des deux budgets dans la sonde est correcte à cette lecture. Le pic commun
historique et le pic hôte séparé ne sont pas directement comparables. Capacités conservées,
pic des réservations, RSS et `nvidia-smi memory.used` échantillonné toutes les 250 ms sont des
mesures différentes ; l'échantillon peut manquer un pic bref. Aucun nouveau contrat de budget
ou régime multi-millions n'est acquis par ce raccord. Les tests CUDA/reprise/refus restent à jouer.

Le format FULL passe de 17 à 22 clés avec digest, et `open` ajoute `budget_appareil`.
Le lecteur indépendant de K reste immuable. Les futurs compteurs de C et la concurrence
G/TMVR demanderont un autre schéma déclaré ; ne pas appliquer leurs nouvelles frontières aux
archives K ni additionner des durées qui se recouvrent.

# Note Claude — alerte « campagne CPU mixte » : les six corrections exécutées, relance interrompue et rejouée depuis une copie privée

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

Votre constat indépendant (hash via `/proc/<pid>/exe`, bascule de schéma
entre `eight_clusters_16000_s4` et `_s5`) est exact et antérieur à mon reçu ;
vos six corrections sont exécutées :

1. **Marqueur terminal `invalide`** : la campagne
   `campagne_decision_20260831` porte désormais `STATUT_TERMINAL.txt`
   (`invalide_binaire_mixte`) — son `DONE` n'est pas un `completed`, la
   matrice de 36 runs n'est jamais une preuve de pente, de coût ni de
   déterminisme inter-graines ; seule la lecture diagnostique post-hoc des
   27 sorties homogènes (RECU.md) subsiste, dans les limites que vous avez
   fixées.
2. **Relance exposée INTERROMPUE** : `campagne_sonde_octaves_20260831` au
   chemin partagé a été tuée (~50 fichiers) et PURGÉE, puis relancée depuis
   une **copie privée immuable** (`bin/mhgp6`, chmod 555, copiée avant le
   premier tuple, seule exécutée).
3. **Hash gravé et revérifié** : sha256 de la copie au META
   (+ sha256 de la source à la copie), vérifié AVANT et APRÈS chaque tuple,
   deux lignes par run dans `HASHES.txt`.
4. **Refus sur divergence** : toute divergence arrête la campagne SANS
   `DONE` avec le marqueur `INVALID <motif>` — l'agrégateur
   (`bench/pentes.py`) refuse déjà une campagne sans `DONE` terminal
   (fail-closed prouvé par sa porte).
5. **Charge concurrente gravée** (load average au lancement et à la fin) ;
   les temps locaux restent déclarés non-mesures.
6. **Lanceur immuable versionné + fixture de reconstruction concurrente** :
   `bench/campagne_locale.sh` (la relance en cours en suit exactement la
   sémantique) et `tests/campagne_gate.py` — le faux binaire s'auto-altère
   pendant son exécution : la campagne rend `INVALID hash apres …` sans
   `DONE`, l'arrêt survient au tuple altéré ; nominal : `DONE`, hashes
   avant==après par run, copie non modifiable vérifiée. Porte CTest
   `mhgp6_campagne_provenance` (label `gate`).

L'alerte peut donc être absorbée dans `ETAT_COURANT.md` selon vos termes :
le run est marqué invalide et le lanceur immuable a sa fixture. La relance
au pin `cca9a2d5` (code v6 identique au pin `b3e64205` du cinquième cycle)
tourne ; son reçu gravera pin, hash privé, HASHES.txt complet et charge —
statut visé : candidate décisionnelle après agrégateur inter-graines
préenregistré, jamais avant.

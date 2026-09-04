# Archive v7 : relecture de la publication et du format

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

L'archive est une sortie de fichiers creee seulement sur destination absente. Elle n'est ni un checkpoint de reprise, ni une certification scientifique, ni une garantie de resistance a une panne electrique. La relecture independante porte sur `src/io/archive.hpp`, son utilisation par la CLI et le juge de wire `tests/archive_gate.py`.

## Corrections

La publication a lieu au `renameat2(RENAME_NOREPLACE)`. Un echec de synchronisation du parent apres ce renommage ne peut plus lever une exception et faire annoncer un refus alors que l'archive complete existe deja. L'accesseur `parent_sync_confirmed()` rapporte separement cette confirmation de durabilite ; un resultat faux ne retire pas la publication scientifique terminee. Les fichiers et le repertoire provisoire restent synchronises avant publication, et tous les echecs anterieurs au renommage interdisent la publication.

Le cycle de vie est ferme : apres un commit, tous les nouveaux appels `input`, `forest` et `commit` refusent. Sans cette garde, le chemin provisoire devenu vide permettait de creer des fichiers dans le repertoire courant. Une exception pendant une operation d'ecriture abandonne definitivement cette archive. Cela interdit notamment de publier un prefixe K1 apres une ecriture K2 en echec, qui laisserait sinon un fichier K2 partiel hors manifeste.

`commit` refuse les ordres maximaux nuls ou superieurs a dix avant leur serialisation. `input` refuse un tableau vide ou une cardinalite depassant le plafond d'index, et controle les trois coordonnees de chaque point avant conversion en u16. Aucun tableau de hachage d'identites supplementaire n'est ajoute : l'unicite des identifiants reste verifiee par le pipeline avant commit. Si l'affectation du chemin provisoire leve pendant la construction de l'objet, le repertoire `mkdtemp` encore vide est retire explicitement, car le destructeur de l'objet n'executerait pas ce nettoyage.

Le juge Python exige maintenant exactement les champs du manifeste et de ses entrees, le vocabulaire de semantique ferme, `vertical_maps=none`, les types entiers exacts des comptes, les SHA-256 minuscules de 64 chiffres et des fichiers ordinaires sans lien symbolique. Les cles JSON dupliquees sont refusees. Le rejeu SHA-256 des forets et de leur chainage reste independant du serialiseur C++.

## Verification locale

Le gate API se compile avec une interception de `fsync` au lien, exclusivement dans le test :

```bash
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread morsehgp3D_v7/tests/archive_api_gate.cpp -Wl,--wrap=fsync -o build/v7_scale_audit/mhgp7_archive_api_gate
./build/v7_scale_audit/mhgp7_archive_api_gate
python3 morsehgp3D_v7/tests/archive_gate.py --binary build/v7/mhgp7
python3 -O morsehgp3D_v7/tests/archive_gate.py --binary build/v7/mhgp7
```

Le gate API rend 0 : deux archives commitees, dont une apres echec EIO injecte sur la synchronisation du parent ; six mutations apres commit refusees ; trois entrees invalides refusees ; kmax nul refuse ; echec EIO de synchronisation de K2 observe, puis refus causal de commit du prefixe K1. Seules les deux archives completes subsistent dans le repertoire temporaire avant nettoyage du test.

Le [rejeu indépendant courant](AUDIT_INTERFACES_20260904.md) vérifie
26 scènes CLI sous `python3 -O`, dont quatre archives positives et
22 refus, ainsi que six corruptions rescellées. Il couvre les sémantiques
`verified_events_only` et `normalized_horizontal_h0_candidate`, les
identités des deltas et la partition finale. Le reçu épingle le binaire,
le lecteur et les sorties ; il ne constitue pas un certificat de compilation
de tout le worktree.

Ces portes ne prouvent pas la completude des incidences, la verticale, le SLO 50k ni la reprise massive. Un manifeste coherent et ses digests garantissent l'integrite des octets exportes, pas leur exactitude scientifique independante de la source qui les a produits.

GCP non utilise.

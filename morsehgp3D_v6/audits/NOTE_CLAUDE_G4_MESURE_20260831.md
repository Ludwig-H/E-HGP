# NOTE_CLAUDE — protocole de MESURE G4 : phases fils / GPU / frontière (profil `g4_mesure_v1`)

Date : 31 août 2026 (soir). Auteur : Claude (session v6). Statut :
`exploration_v6_hors_registre`, `public_status=not_claimed`. Cette note
annonce une **extension du protocole G4** avant une session payante, et le
cadre dans lequel elle est lancée.

## Pourquoi

Directive de l'exploitant (31 août, soir) : « continue jusqu'aux tests
GCP. Mesure à la fois le gain en parallélisme CPU et le gain GPU. Les
contrats 50k points et plusieurs dizaines de millions de points
sont-ils atteignables sur G4 ? ». Les campagnes locales (replication,
confirmation hors échantillon) portent déjà les pentes ; ce qui manque et
que seul G4 fournit : les **temps** à 48 vCPU, le **GPU**, et la
**frontière mémoire** à 180 GiB.

## Ce qui est ajouté (commit porteur de cette note)

Trois phases dans `v6_campaign_remote.sh` (plans annoncés avant le premier
run, comme les trois existants) + `validate_v6_campaign.py` + profil
canonique `gcp-migration/profils/g4_mesure_v1.env` (épinglé, 11e fichier du
manifeste de protocole) :

1. **FILS** (`sweep_plan=v1`) : moteur v6 seul, `fam:n:liste_de_fils`,
   ordre contrebalancé par répétition (avant/arrière), sans `--digest`.
   Le validateur exige la **bit-identité entre fils** sur les lignes
   invariantes (generation, sweep, vwspd, octaves, vcensus, p_factor,
   ledger, cardinalités — jamais `ouvriers` ni l'identité qui imprime
   `threads=`) : la doctrine « sorties identiques quel que soit le nombre
   de fils » jugée à l'échelle. Vérifié localement : 18 lignes identiques
   entre 2 et 7 fils à n=8000.
2. **GPU** (`gpu_plan=v1`) : v5, seule ligne à cibles CUDA — protocole
   **hérité de `v5_campaign_remote.sh` / `validate_v5_campaign.py`**
   (témoin device nvcc 12.9 + lot arithmétique + scans sans désaccord,
   mutant du témoin TUÉ code 4, lanes q3/q4 device aux 7 triples et
   planchers exacts), puis par famille QUATRE contrats 50k à `--digest` :
   CPU, `--gpu`, adaptatif `--gpu-min-sites=256`, `--gpu-wire=index`.
   `digest_balls` + `digest_all` IDENTIQUES entre le CPU et chaque route
   device, sinon la phase est tronquée (gravée). Le gain GPU se lit dans
   les murs de `gpu_resume.txt`, jamais conclu par le validateur.
3. **FRONTIÈRE** (`frontier_plan=v1`) : v6 à 48 fils, tailles croissantes
   (200k → 800k), RSS gravés par GNU time. **Le code de sortie est LIBRE**
   (un OOM, un refus de capacité ou un timeout est la donnée recherchée) ;
   la présence, les pins et le recoupement RSS restent exigés ; seule
   l'échéance tronque.

Matrice `g4_mesure_v1` (préflight budgétaire déclaré : 22 984 s pour une
fenêtre de 26 400 s) : conformité v5≡v6 4 familles × {32000, 50000} ;
fils 1..48 à 16000 (deux familles) + contrôle 50000 ; GPU 4 familles à
50000 ; frontière uniform/scanline_stationnaire 200k, uniform 400k,
uniform 800k ; bench réduit 50k ; queue vide (sentinelle `aucun`,
plan `runs=0` — les pentes sont l'affaire des campagnes locales).

## Falsification avant dépense

- `selftest_campagne_v6.sh` : **36 vérifications** — les trois phases
  exercées de bout en bout par faux outillage (faux nvcc / nvidia-smi /
  cmake / témoin / lanes / mhgp5_cuda), échec de frontière SIMULÉ toléré
  (code=9 gravé, campagne valide), et six nouvelles falsifications tuées :
  bit-identité entre fils violée, digest GPU ≠ CPU, mutant du témoin non
  tué, statut de frontière supprimé, plan GPU supprimé, clé de profil
  absente.
- `selftest_cycle_vie_v6.sh` : 15 scénarios + **11** refus de pin (le
  nouveau profil est épinglé et son altération refusée).
- `tests/gcp/` : 81 tests de sûreté + intégration réelle du cycle de vie.

## Cadre du lancement

L'audit GCP (quatre tours, `AUDIT_GCP_V6_P0_20260831.md`) avait réduit le
NO-GO à une formalité de « GO frais » après exécution des quatre défauts
courts — faite et rejouée depuis HEAD propre. Aucun nouveau dépôt
d'audit depuis ; la session est lancée **sur directive explicite de
l'exploitant**, qui détient l'arbitrage de la dépense. Le reçu durable
liera : pin, profil canonique, sorties, validation par le validateur
épinglé, certification `TERMINATED` sur la génération exacte. Toute
requalification par l'audit reste ouverte — les selftests et le présent
protocole sont rejouables depuis le commit porteur.

## Ce que la session devra répondre (et ce qu'elle ne peut pas)

- Gain de parallélisme CPU : courbe des murs 1→48 fils (médianes,
  contrebalancées) — mesure, jamais un claim d'algorithme.
- Gain GPU : murs CPU vs `--gpu` vs adaptatif vs `wire=index` à 50k sur
  quatre familles, à objet prouvé identique (digests). C'est le GPU de la
  **v5** : la v6 n'a pas encore de port CUDA — le chiffre borne ce que le
  port v6 (G0/G1/G2) peut viser, il ne le mesure pas.
- Contrat 50k : directement mesuré (conformité + bench + GPU à 50000).
- Dizaines de millions : la session ne peut PAS le mesurer (RAM). Elle
  mesure la frontière in-memory (RSS/point à 200k/400k, mur à 800k) ;
  l'extrapolation et le chemin streamé restent ceux de
  `morsehgp3D_v5/docs/ECHELLE.md` § 3 (K=10 à 10M ≈ 100 Go RAM + 1,3 To
  disque + 6–7 h ; K=5 ≈ 1 h) — conception, pas une promesse.

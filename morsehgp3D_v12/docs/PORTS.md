# Ports du socle de la v12 depuis la v11 gelée

Source : moteur gelé de la v11, commit `ac081a06f` (dossier `morsehgp3D_v11/`, identique à `HEAD` pour tous les
chemins portés). Pour chaque fichier, le SHA-256 est celui de la source : `git show ac081a06f:morsehgp3D_v11/<chemin>
| sha256sum`.

**Renommage mécanique** appliqué à chaque fichier texte (outil `port.py`, hors dépôt) : `mhgp11` → `mhgp12`, `MHGP11` →
`MHGP12`, `hgp11_` → `hgp12_` (donc `hgp11_ref` → `hgp12_ref`), `ehgp.v11.` → `ehgp.v12.` ; chemins renommés :
`reference/hgp11_ref/` → `reference/hgp12_ref/`, `tests/support/mhgp11_gate.py` → `tests/support/mhgp12_gate.py`.
**Seule exception** : le format `hgp11_supports_oracle` des sorties canoniques de l'oracle des supports
(`reference/hgp12_ref/supports.py`) est gardé, parce qu'il entre dans les 50 empreintes gravées de
`reference/test_supports.py` ; le renommer aurait exigé de regraver ces empreintes.

États : *copie à l'identique* (octets égaux à la source) ; *renommage seul* (égal au renommage mécanique de la source) ;
*renommage et adaptations* (modifié à la main en plus, adaptation décrite). Bilan : 210 fichiers, dont 28
copiés à l'identique, 111 renommés seulement, 70 adaptés (dont `reference/tests.cmake`, adapté à l'intégration), et un fichier sans source dans la v11
(`docs/ARCHITECTURE.md`, ci-dessous).

`docs/ARCHITECTURE.md` n'est pas un port : c'est le document de la v12 (dernier commit qui le modifie : `c0bb99fd8` ;
SHA-256 du fichier : `d2f1454502752441b213b8cf8a45f1201cd7c285f2086f6f08fe6fbfd92800e1`), dont le socle **propose** deux modifications, nécessaires à `tools/check_style.py` : la table du § 5 prend
la forme « Module | Rôle | Dépend de » et ne liste que les six modules présents (les modules prévus passent dans une
seconde table) ; un § 7 « Règles du code portées de la v11 » rattache aux paragraphes 1, 3, 4, 5 et 7 de l'architecture
de la v11 les références « ARCHITECTURE.md de la v11 » des commentaires portés.

| Fichier de la v12 | Source dans la v11 | SHA-256 de la source (`ac081a06f`) | Adaptations |
| --- | --- | --- | --- |
| `CMakeLists.txt` | `CMakeLists.txt` | `82fa1054e14a2bf16e28195cac32a4d81dec56bb81d6481d9d9214d5c87f555d` | renommage et adaptations : en-tête (v12, chaîne de port) ; projet `morsehgp3d_v12` ; profils : 21 par défaut, 24 admis, 18 refusé (jeton `mhgp12_coord_bits_18_abandonne`), 32 refusé (`mhgp12_coord_bits_32_differe`), toute autre valeur refusée (`mhgp12_coord_bits_invalide`) ; option CUDA de la v11 retirée (aucun module GPU dans le socle) ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `bench/index_io.hpp` | `bench/index_io.hpp` | `c712fc007b72c22ce2b0750a98ac27ad479f1d95f21ed2e71dd5008b70613421` | renommage seul |
| `bench/index_io_test.py` | `bench/index_io_test.py` | `263fc54a03bd0c9dcd8e538f6a764e3c5c193a1f7ca8056b4d75b517ddaf8b96` | renommage et adaptations : lit `event_json` dans `index_semantic.py` au lieu de `catalogue_g4.py` |
| `bench/index_probe.cpp` | `bench/index_probe.cpp` | `763bc1e0e7a8ade76075a71ecea05c9e4da8483eea624503d5f578550284e364` | renommage seul |
| `bench/index_semantic.py` | `bench/index_semantic.py` | `0f04e185e7eeb2be0f91ad29466fad45c9b2224bdd209a4cdcba069e7394366e` | renommage et adaptations : `json_value` et `event_json` copiés de `bench/catalogue_g4.py` (non porté) |
| `cmake/expect_refusal.cmake` | `cmake/expect_refusal.cmake` | `a0fa1e554dd494a4bfdcb5bafe0f3638615538a9866b3bac474a227335f6c40c` | renommage seul |
| `cmake/gates.cmake` | `cmake/gates.cmake` | `8abaa4b9a533f0988cd2a4b436665905c3ad1be92a4f33f360da2fce34c1d70a` | renommage et adaptations : en-tête (v12, chaîne de port) |
| `cmake/modules.cmake` | `cmake/modules.cmake` | `c60dd9c3737995f2eced7d2769ad98f6ac6d68a05699c24752ba75bc21554602` | renommage et adaptations : table réduite aux six modules du socle (`core num sched cloud io index`), copie du § 5 du document de la v12 |
| `cmake/run_expect.cmake` | `cmake/run_expect.cmake` | `165d02c686bb71d32fbf9edb3fb3afa80c03ca3c8d6deb40b8444eeb6443646e` | renommage et adaptations : en-tête (v12, chaîne de port) |
| `reference/README.md` | `reference/README.md` | `bd2159adc845e21aaa4fda75330f5ec7da49ecdb235adffdb4a8b29a5b560ced` | renommage et adaptations : note de port ; cadre v12 ; portes natives citées rendues à la v11 |
| `reference/hgp12_ref/__init__.py` | `reference/hgp11_ref/__init__.py` | `5309faf36a56bfb719d9fff4fb5872045aad87b1b35b2fc3992fd2c37791ad3e` | renommage et adaptations : docstring (v12) |
| `reference/hgp12_ref/constructive.py` | `reference/hgp11_ref/constructive.py` | `7e97f89789cafb4f241903e6d0f0e636425cdf3d526697e4e891d483f868fedc` | copie à l'identique |
| `reference/hgp12_ref/definition.py` | `reference/hgp11_ref/definition.py` | `5b0407236c64658e825179689985a939db3d6bcdbf6607543117be773ab88c43` | copie à l'identique |
| `reference/hgp12_ref/dumps.py` | `reference/hgp11_ref/dumps.py` | `2ece0b491430e314091de7b40a72233e299c543f236bcc00bbd5f84b38338373` | copie à l'identique |
| `reference/hgp12_ref/families.py` | `reference/hgp11_ref/families.py` | `244b9b325a7c9f43c7086cc7e9674bdc04ec2307a01bf69e2b12d5f02e75d73a` | copie à l'identique |
| `reference/hgp12_ref/intgeom.py` | `reference/hgp11_ref/intgeom.py` | `d0f7b1cecbbb1b25c3091bc1e3d3d062b9ee0f16b1d81c7117bf60b0d13a3f4b` | copie à l'identique |
| `reference/hgp12_ref/judge.py` | `reference/hgp11_ref/judge.py` | `09e65a40f7ff33400c04eb91c442a430579bf020b115589b69ad8384529d6507` | copie à l'identique |
| `reference/hgp12_ref/model.py` | `reference/hgp11_ref/model.py` | `2c01df62e3060c992c0ee5a6b3b7474ce9393e48c7079b93115dfd648e09466c` | copie à l'identique |
| `reference/hgp12_ref/supports.py` | `reference/hgp11_ref/supports.py` | `970f645462a019639cead958cf8f45b1324cf3083bb61fc9a0891ddc523254ee` | copie à l'identique |
| `reference/interval_oracle.py` | `reference/interval_oracle.py` | `4fcd49d8d41c09ebc9ebdd4bf79c5dffd8e848d58f12f284c420272f39f0d427` | renommage seul |
| `reference/ref_mutants.py` | `reference/ref_mutants.py` | `b6750ae0b3cbeb6dfcd8febdee8ba71dfcfa5bac8e55c340fe28bc9568d24231` | renommage seul |
| `reference/test_dump_v10.py` | `reference/test_dump_v10.py` | `b858a324ffceb71f70c33249130f069f9ddef3c83a7e08efd88cf3e0a6871695` | renommage seul |
| `reference/test_projection_contracts.py` | `reference/test_projection_contracts.py` | `61c222b4e5717eb7c5f98bb9abfd35a17cf51e2570d7e033ff7806c7bab29936` | renommage seul |
| `reference/test_ref.py` | `reference/test_ref.py` | `e89e1f15b0dad7c47467ab5f3b586f197679d0ffd9bbcb27eb93b67bc7f1da68` | renommage et adaptations : docstring (v12) ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `reference/test_supports.py` | `reference/test_supports.py` | `1b90fdadfd896fa6b92ea02d34e457d111c80659683ddcc92117e3a07d205b12` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `reference/tests.cmake` | `reference/tests.cmake` | `98f4f68668bda999ba7fd6cc6a8633797c7e0e902564579dfcb11862871c7d15` | renommage et adaptations : à l'intégration (`a0091e2b7`), trois portes ajoutées pour le témoin `WIT-T1-CARRE` (porte, mutant `sans_inclusion`, refus), avec leurs jumelles `-O` (`CST-0115`) |
| `src/cloud/cloud.cpp` | `src/cloud/cloud.cpp` | `64c5a3d29e1fe0cd05adc5f1d04c4d957731fa1f1eb9e583330675b65751afe9` | renommage seul |
| `src/cloud/cloud.hpp` | `src/cloud/cloud.hpp` | `dd04b4f051cf356a0a12639d29c037594cacc4f9d9913a1238d94faafbbcfc68` | renommage seul |
| `src/cloud/module.cmake` | `src/cloud/module.cmake` | `81d1470bc740f0fa7aca00e56eab47ee72700b848bae91b99a9fad4998a050b5` | renommage seul |
| `src/cloud/morton.hpp` | `src/cloud/morton.hpp` | `a58062c4fa9225558b2154faf176ad55b1ea4f638abce8242a4a6f99bc04ba71` | renommage et adaptations : convention de Morton contrôlée aux profils 21 et 24 seulement (`morton_on_basis<18>` retiré) ; commentaires |
| `src/core/buffer.cpp` | `src/core/buffer.cpp` | `636f49bd818885d4d3f682caf0ebf84c31149030ca09ff01935ff5c43207536d` | renommage seul |
| `src/core/buffer.hpp` | `src/core/buffer.hpp` | `921655c76a51e23d9ffe970c503caf6fb04098ce8c27b98100d15e39fa5892e8` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/core/core.hpp` | `src/core/core.hpp` | `5b3703641031c27e7bf907b2a8c05c79007ee1cae47da7d8e5f94ea435c563d3` | copie à l'identique |
| `src/core/ledger.cpp` | `src/core/ledger.cpp` | `7cad3a8604b2f4553b0653cb75613394ca7f55765811b6281584dce49b92a773` | renommage seul |
| `src/core/ledger.hpp` | `src/core/ledger.hpp` | `5c14715e44557374d83639160825b87bd323e15fccba42d5461c219328d9e998` | renommage seul |
| `src/core/module.cmake` | `src/core/module.cmake` | `6f46bf9e971c518648cac50b9524af90bbef7d9fdaf5eb1866acc6013ecab009` | renommage seul |
| `src/core/reasons.def` | `src/core/reasons.def` | `f89f4929ba0358a3db736dec1af9934a546ba33fed0333f97c8fe6f13116c27b` | renommage et adaptations : 13 raisons des modules non portés retirées (catalogue ×6, tower ×2, supports ×2, api, points, head) ; ordre relatif des 18 raisons restantes inchangé ; en-tête |
| `src/core/status.hpp` | `src/core/status.hpp` | `e6004beb5ec4e78fb2b582004f4122611602809d360ff9f1b3bf4679348f6594` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/core/types.hpp` | `src/core/types.hpp` | `24e005c3ac6d729dd53db3068b7b7c9b562705f29c33112c88bf02c3baaa6954` | renommage et adaptations : garde de profil : 21 ou 24 (18 et 32 refusés, message explicite) ; commentaires de port ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/index/access.hpp` | `src/index/access.hpp` | `4730866f879606d6d4d2c7e63a520a20874e5c97495800d42b67524cf5c4b2bd` | renommage seul |
| `src/index/build.cpp` | `src/index/build.cpp` | `d0e6bb36ea2e353bc6f4d4b6d3f0e744f80693fec52ec1a72ac5606a19443062` | renommage seul |
| `src/index/census.cpp` | `src/index/census.cpp` | `a4ee4a1f6dc04469432541a159266f27771d5dedf9761d76e90de93710efcac5` | renommage seul |
| `src/index/census_workspace.cpp` | `src/index/census_workspace.cpp` | `778d7b59ca07e2da64028a474d64a23835de5c3e6d427108a9fbd4a38c72e546` | renommage seul |
| `src/index/index.hpp` | `src/index/index.hpp` | `e9669ee18520bebd309713eeb142f4936219e50d0969c56b28305710162940c1` | renommage seul |
| `src/index/module.cmake` | `src/index/module.cmake` | `9b0e880c7b3e70182e565feeaf1049187e4be03b9741c9a735ee6a4b274049d1` | renommage seul |
| `src/io/directory.cpp` | `src/io/directory.cpp` | `b32fc41863a7163ae00cb74893c67cd6bc4500965669f881fd72334ab54f2aa3` | renommage seul |
| `src/io/input.cpp` | `src/io/input.cpp` | `4b37b9da83433b82140501f547ec569bebdbc983f390c6e595215ec86b80142c` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/io/io.hpp` | `src/io/io.hpp` | `3843beac6d28a4430a1335098c13f9c908bf7e5a58ede8696b8884d544971f27` | renommage seul |
| `src/io/module.cmake` | `src/io/module.cmake` | `91c3aaee8104c4f95093582ef1ff2cb7aa99d6e1f6c4fedfba76491dd2c71cc5` | renommage seul |
| `src/io/sha256.cpp` | `src/io/sha256.cpp` | `549f088a604a297535bf7b75dbae3f0f337a214d90c4c1d59b08c70a40c4987b` | renommage seul |
| `src/io/writer.cpp` | `src/io/writer.cpp` | `0afabcb1485a5b074dc101ab48e5c52a40923cac6b06fd3660ce972d61f2f395` | renommage seul |
| `src/num/big.cpp` | `src/num/big.cpp` | `35a0fc3979ff8ec42fa97cf9ddfc4c1373082c5b5279381c5a06cdf3998dfc56` | renommage seul |
| `src/num/big.hpp` | `src/num/big.hpp` | `162bea8d8bfd950f6e2fd039d8045d0a9211855daea10695e0e603c0217584e9` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/num/budgets.hpp` | `src/num/budgets.hpp` | `b86c7918573e473a9492b3c26c463f9cb340cec5e97d092e774a5c0af44c062e` | renommage et adaptations : `Budgets<B>` : B dans {21, 24} ; contrôle `Budgets<18>` retiré |
| `src/num/center_region.cpp` | `src/num/center_region.cpp` | `67f412f16e24fc575630f60e5f8929f0899e4714f145a2a2490639e29fc14783` | renommage seul |
| `src/num/center_region.hpp` | `src/num/center_region.hpp` | `22c06de8423871a5660db145683b67cb188c000c00c4deb1a5a10ddfcf199417` | renommage seul |
| `src/num/center_region.provenance.json` | `src/num/center_region.provenance.json` | `0e887b3701176bc7e025dc38aecf842f37b869ab61a2a700b4c0e781861a5498` | copie à l'identique ; registre de provenance v10 → v11, gardé tel quel (historique) |
| `src/num/centers.cpp` | `src/num/centers.cpp` | `36fbb5b188189c05c6187906326943347f2e1fab2da52961a83826eb9a2590c0` | renommage seul |
| `src/num/geometry.hpp` | `src/num/geometry.hpp` | `3f1baafbd9cbd8d7640cf49c19fe227d3a3a0d14fe642f3dd4531a42f8724c11` | renommage seul |
| `src/num/geometry_internal.hpp` | `src/num/geometry_internal.hpp` | `4d6d39ecad5d4584024510262c5cbfc7a3aa84e5e741489eb460b1fa3c813e97` | renommage seul |
| `src/num/integer.hpp` | `src/num/integer.hpp` | `8ccf2b24f49681cc8bba1494c982c64578c73337c228015e62de50696d5d529e` | renommage seul |
| `src/num/lattice_bounds.hpp` | `src/num/lattice_bounds.hpp` | `54cd871f7a4c5b6e4a646ec656b1d939af710a2708ac247f1f4a7dd70cbde95a` | renommage seul |
| `src/num/level.hpp` | `src/num/level.hpp` | `c769f20addcc11ef9e249031baee3ce79d538c7aff61d0035e069237d73193e9` | renommage seul |
| `src/num/module.cmake` | `src/num/module.cmake` | `78038edc1637b9a78a8925a93642c8c6a9948e74ad39ae17bb14b7040734518d` | renommage seul |
| `src/num/num.hpp` | `src/num/num.hpp` | `b99a7e6d5b198105982dc7a0b3e05ac7ebf813e0b0639e12c213122e597d165f` | copie à l'identique |
| `src/num/orientation_certificate.hpp` | `src/num/orientation_certificate.hpp` | `f8571e6274677a3af1f93948a544b9091ee0e2b1d910cba26bf01b7a8cdc4ec0` | renommage seul |
| `src/num/power_certificate.hpp` | `src/num/power_certificate.hpp` | `ba545b4f5d5bb7cd2d2b6198162241bbd4b89b3e6b725485f54f0387d6a804c0` | renommage seul |
| `src/num/power_checked.hpp` | `src/num/power_checked.hpp` | `e38083a1cb0d1f4b90df711ecd47496ddd3d7998306b5ba713062107a610228b` | renommage seul |
| `src/num/predicates.cpp` | `src/num/predicates.cpp` | `cf7baabffc60f6213b6b0a1cc99a21bc47d92fac37577f93dfa9aa770d463849` | renommage et adaptations : commentaire de l'orientation (le cas i64 du profil 18 n'existe plus) |
| `src/num/q4_weights.hpp` | `src/num/q4_weights.hpp` | `5da0994be48f50b0571e5d097a17da005a7dad4b86368c0448ae664cbde66c09` | renommage seul |
| `src/num/radical.cpp` | `src/num/radical.cpp` | `87256532859020c65dc09148809231374d8a82c89063e188bc3426fd08f6d407` | renommage seul |
| `src/num/radical.hpp` | `src/num/radical.hpp` | `eb67776e81c8fb0d267d2c9d124c46117768d00418ba1aafeb03f9727c920708` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/num/rational.cpp` | `src/num/rational.cpp` | `67864a76f58a3633f556050acf3299113396be2054d5f67447a0cf631e962d23` | renommage seul |
| `src/num/rational.hpp` | `src/num/rational.hpp` | `76a82aad8842a6f39c247aa9224d1f7e6dca16f5115de8a9ee36e829e9c721c6` | renommage seul |
| `src/num/roots.cpp` | `src/num/roots.cpp` | `40aaffb8b4f395d402e70175d12670e35d434bbba285df7f2d8a7b8282fe9221` | renommage seul |
| `src/num/roots.hpp` | `src/num/roots.hpp` | `50c600c679470aeebd67aac00889aa5bf2a2aaf224901b1829d640abe535b4e5` | renommage seul |
| `src/num/sphere.cpp` | `src/num/sphere.cpp` | `4da1b9bf57afdc068e91afe9616491a70787ed934add72e0d25cf1a8b81a9dc3` | renommage seul |
| `src/num/wide.hpp` | `src/num/wide.hpp` | `58478d10a1e2b526652e5f5b1aeb87f23325bfb515997808a65eae380a531fb1` | renommage seul |
| `src/sched/module.cmake` | `src/sched/module.cmake` | `6011a4e03f4f025a8679c90531d7d0f4e2b9eb5c2a77a35b3939856823906cf0` | renommage seul |
| `src/sched/pool.cpp` | `src/sched/pool.cpp` | `fd72a1abe07dbb9700821cd0c636344c12e3a7a8ae968bdf3a086f514c270fde` | renommage seul |
| `src/sched/sched.hpp` | `src/sched/sched.hpp` | `104e1a3c75e8adfe6a2f897ffcbfa4d06fc7f534fe3d6f788ce516c7e66ba07f` | renommage et adaptations : références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `src/sched/source_pins.json` | `src/sched/source_pins.json` | `022c46939777effcfc7747dac0a355585be095d493e544c21ac4204aa25d9fd8` | copie à l'identique ; registre de provenance v10 → v11, gardé tel quel (historique) |
| `tests/catalogue/fraction_model.py` | `tests/catalogue/fraction_model.py` | `19d52412a48000efdc238b18493671327c44c587c7d4831c99fdb5a37e592a1a` | copie à l'identique ; modèle Fraction borné du catalogue, seule dépendance hors socle d'une porte `num` (`centers_oracle.py` l'importe depuis `tests/catalogue/`) ; aucune porte du catalogue n'est portée |
| `tests/cloud/cloud_fault.cpp` | `tests/cloud/cloud_fault.cpp` | `8ec1c5328a09aee0f0d61b4d2a11d8a036500ddd7bdd47bdacc1824fd3d9b3ec` | renommage seul |
| `tests/cloud/cloud_test.cpp` | `tests/cloud/cloud_test.cpp` | `803c0d29a2f0ab486990b54324e7b306c82fa0c53c53e08d9931889e8d9eeac8` | renommage et adaptations : contrôle de la clé de Morton à 18 bits retiré ; plancher `morton` 16 → 15 |
| `tests/cloud/immutability_probe.cpp` | `tests/cloud/immutability_probe.cpp` | `76872e2f41cd9e2e86a2f98cd6104d52e704ed9271460181cdaa52dc5e218603` | renommage seul |
| `tests/cloud/source_pins.json` | `tests/cloud/source_pins.json` | `2d3ea846f1e26826455238ddbbaccc3f2078cf64dc1a0294795a163791d9a80c` | copie à l'identique ; registre de provenance v10 → v11, gardé tel quel (historique) |
| `tests/cloud/tests.cmake` | `tests/cloud/tests.cmake` | `f595dc17143d1e72fba55ea69fef6320147a74273ab080a420bd27da7d65d914` | renommage seul |
| `tests/cloud/width_probe.cpp` | `tests/cloud/width_probe.cpp` | `cd6509cdd9e5e34af2024584cd6ad6313e7a88be91f30a63c64d50f3f2ff426a` | renommage seul |
| `tests/core/alloc_fault.cpp` | `tests/core/alloc_fault.cpp` | `50578aa1b678465d22a0298f32586e09268e9c1c7c1d070554f91efb98c55069` | renommage seul |
| `tests/core/buffer_test.cpp` | `tests/core/buffer_test.cpp` | `678b985d9139a3197e870ec910d02586960645c2ae35f05f400897321cae3771` | renommage seul |
| `tests/core/guard_probe.cpp` | `tests/core/guard_probe.cpp` | `f81d55a8e13575a257a839bd087c6c2cf04de26e13249ca91866c29a507359d8` | renommage seul |
| `tests/core/ledger_test.cpp` | `tests/core/ledger_test.cpp` | `eedcbbe9e59ab1643a940074b3d1b13846d2429a384b1267911d4484209f18c1` | renommage seul |
| `tests/core/misuse_probe.cpp` | `tests/core/misuse_probe.cpp` | `563cf4d858b62a79e5fa1d6e03b2fc77ef6a59f6188f3edd5cf7624c4f6ae565` | renommage seul |
| `tests/core/poison_test.cpp` | `tests/core/poison_test.cpp` | `e575e5c788e03e7487e4f689f032b9bdfe55ac0299d13ec508bbd1c62f48937c` | renommage seul |
| `tests/core/refusal_probe.cpp` | `tests/core/refusal_probe.cpp` | `ea4511cba38619d9f8efc6fa2bf60c24c90c1c1d71312981f2f5309d8575615a` | renommage seul |
| `tests/core/status_test.cpp` | `tests/core/status_test.cpp` | `2c88cdeb93c0d6ef0451f5741ae22b5d7a949a6a37897c95a42cf1cff8be14ca` | renommage et adaptations : table gravée des raisons : 18 lignes (31 dans la v11) ; plancher du test `reasons` 104 → 65 ; profils admis 21 ou 24 |
| `tests/core/tests.cmake` | `tests/core/tests.cmake` | `3dee4dddc2d20776913a8ffc727ee2fcb664c470f8d83ea57b16c5c32ecdb635` | renommage et adaptations : portes ajoutées `mhgp12_core_coord_bits_18_refusal` et `_32_refusal` (refus à la compilation) |
| `tests/index/README.md` | `tests/index/README.md` | `d489c464d76cf385e4b2913903d51712a56194842c601ca36aae1e30c0c42720` | renommage et adaptations : note de port ; lien vers les reçus de la v11 |
| `tests/index/borrowed_fault.cpp` | `tests/index/borrowed_fault.cpp` | `3efd451732cce050211ba43a94e7830b031a7fe72abef8fd6dab2aec34f745bd` | renommage seul |
| `tests/index/borrowed_oracle.py` | `tests/index/borrowed_oracle.py` | `98df0d70fc2ae130a29c368e14fe2b6863d8cdffff2c4e7d05a92a37d060f213` | renommage et adaptations : profils (21, 24) ; planchers du modèle pour deux profils |
| `tests/index/borrowed_probe.cpp` | `tests/index/borrowed_probe.cpp` | `f1cd936f4efd00fbd4d45ada1e4985ec4e7781ab71e980bc4d310f675ccf4908` | renommage seul |
| `tests/index/borrowed_test.cpp` | `tests/index/borrowed_test.cpp` | `f04e418727906a0e2f8bc24ee08bd7b3aba04e83d47b365fd22f53d6b6b9baae` | renommage seul |
| `tests/index/fault.cpp` | `tests/index/fault.cpp` | `24d607b1c2836d1fe06a2bc803f685e13df69a75526a34111cf59dd1bcb6b03e` | renommage seul |
| `tests/index/fixtures.py` | `tests/index/fixtures.py` | `05ef0670ffe59bf439a308d4d8325d60b82c697e1a582000d1723c0885767fb6` | copie à l'identique |
| `tests/index/fraction_model.py` | `tests/index/fraction_model.py` | `b91fd06fa15b2cd4206e333d4f69576cd34f9930dcff7dc32f19ffc7eebfa525` | copie à l'identique |
| `tests/index/fraction_oracle.py` | `tests/index/fraction_oracle.py` | `740f7a3c2f9c847c1a7b7cca44468e4df50b85a535baa2543935082903a8f4de` | renommage et adaptations : profils (21, 24) |
| `tests/index/judge.py` | `tests/index/judge.py` | `35cf34cd0fe5059efe084ad1d68c6f9a8117ce8f3f30455887a4e54516eccdc2` | copie à l'identique |
| `tests/index/model_test.py` | `tests/index/model_test.py` | `3469d08e70811b21c9fafc98c02f2b22796a2399883f9e04f73c21ee560b7824` | renommage et adaptations : profils (21, 24) |
| `tests/index/probe.cpp` | `tests/index/probe.cpp` | `1e8fd3fe548462e4e6c49d18bd49e154127fabaf1b9b3f4fea8d2e0652014034` | renommage seul |
| `tests/index/requests.py` | `tests/index/requests.py` | `b53f63bb7d87be9d3fd17fd88cd34b0906c426c770ee0ba53fe7fce1b89e57e6` | copie à l'identique |
| `tests/index/source_pins.json` | `tests/index/source_pins.json` | `ddeca0583f624a4e107682d121594c1c74d43b9507c20aaf2f8c843e3eed490f` | copie à l'identique ; registre de provenance v10 → v11, gardé tel quel (historique) |
| `tests/index/test_support.hpp` | `tests/index/test_support.hpp` | `e87f74fe6838ff441735200b467268561866bba945c18119bf60c84a45c63603` | renommage seul |
| `tests/index/tests.cmake` | `tests/index/tests.cmake` | `9d28cf90bc8f78e6aa0b7e8e8b426e990c35c5215873daef7bd73a123de0cd12` | renommage et adaptations : porte `mhgp11_index_bench_collector` non portée (pilote G4 du profil 18) ; ligne du modèle emprunté recalculée |
| `tests/index/unit.cpp` | `tests/index/unit.cpp` | `d03539c51448fb09299a77eb4ccbdf3f389c201cae4a32fd6d3121dfbaf05082` | renommage seul |
| `tests/io/fault.cpp` | `tests/io/fault.cpp` | `ed0579a48d703636e31dd6fb0c06b4893fc69aefc54befccabaa2d2c345c2965` | renommage seul |
| `tests/io/input_test.cpp` | `tests/io/input_test.cpp` | `a37afb8591180384f3d71565cb3e200a83b6fe04d5f4688a5093bddc50b58989` | renommage seul |
| `tests/io/io_support.hpp` | `tests/io/io_support.hpp` | `24b5ee0c48cd16dfa4d4b49d71d66f60b6eb676e6d70ebc5f42db06cdcfd683d` | renommage seul |
| `tests/io/plan_test.cpp` | `tests/io/plan_test.cpp` | `6d464f3b7f660306bde03d1a0bd9e94bca5e130f9ff6d464e78e9deaa2bdff2e` | renommage seul |
| `tests/io/sha256_oracle.py` | `tests/io/sha256_oracle.py` | `d4543cd9e3850e5246a485e2ca87d90893773e6ec3bf098c3560e835bc58f222` | renommage seul |
| `tests/io/sha256_probe.cpp` | `tests/io/sha256_probe.cpp` | `c362fe5a36abfd95aa53c7cefa5197e539ee828ad00a8b2b7f1694c65fa8e1bc` | renommage seul |
| `tests/io/sha256_test.cpp` | `tests/io/sha256_test.cpp` | `d89b455f54e6745050cbb6d5b6a6c5545c65f35d88ffe318bf92b86fa5b715f9` | renommage seul |
| `tests/io/tests.cmake` | `tests/io/tests.cmake` | `c292723be747741fd11c45cbcd57b0db02629dc81f2baea57b3a4c1c78dfe50f` | renommage seul |
| `tests/io/transaction_test.cpp` | `tests/io/transaction_test.cpp` | `acf4be997010ae2ac85cd68d4a10c241ba182a3e83d4698b1be6e7bf3e772a1e` | renommage seul |
| `tests/mutants/cloud.json` | `tests/mutants/cloud.json` | `a5c652eac13075e49ce61ce1038af603fe859d1f353f338ead8d47024fa77b3b` | renommage seul |
| `tests/mutants/core.json` | `tests/mutants/core.json` | `7a79c8aa03699fab866edf3f2791e3b815a596996e9aec71fe8410eb28ce7338` | renommage et adaptations : mutant `profil_de_20_bits_admis` recalé sur la garde 21/24 ; mutants ajoutés `profil_18_readmis` et `profil_32_admis` ; plancher 82 → 84 |
| `tests/mutants/fixture/CMakeLists.txt` | `tests/mutants/fixture/CMakeLists.txt` | `c8e418f8eb97650a00f0cc67a72d8ff4be112958091b61a0162fc1916e326152` | renommage et adaptations : commentaires (v12) |
| `tests/mutants/fixture/src/casse.cmake` | `tests/mutants/fixture/src/casse.cmake` | `b0b1a6297b720d7380f60b4fa6a56e1f3bce8e860aa2e0eadcc0ca4ee769988b` | copie à l'identique |
| `tests/mutants/fixture/src/expected.txt` | `tests/mutants/fixture/src/expected.txt` | `20dcfcbf8b8ab52e5c38dfed2b082c080962cba452d5fc273db14a32a055f271` | copie à l'identique |
| `tests/mutants/fixture/src/judge.sh.in` | `tests/mutants/fixture/src/judge.sh.in` | `5e2c129ef91d7caecf9616d0c43f8b7ebb2b4b2c7b881cdf5698ffbf9120fd6a` | copie à l'identique |
| `tests/mutants/fixture/src/value.txt` | `tests/mutants/fixture/src/value.txt` | `20dcfcbf8b8ab52e5c38dfed2b082c080962cba452d5fc273db14a32a055f271` | copie à l'identique |
| `tests/mutants/index.json` | `tests/mutants/index.json` | `017a3842e9c5665d30d09ade65b1443514c8867913dfa4122f18103224459f89` | renommage seul |
| `tests/mutants/io.json` | `tests/mutants/io.json` | `e65db98361afb0fbb25a6013a6315288ac8ebe3ac67d5699e44d33f512a8d2dd` | renommage seul |
| `tests/mutants/num.json` | `tests/mutants/num.json` | `f684f481aecc26bfbd0df1f59ace863bad93318e62aae0a315119e1cf17c1f9d` | renommage seul |
| `tests/mutants/run_mutants.py` | `tests/mutants/run_mutants.py` | `866dd057ae01f9a2628565d16f91accc33ae0bba4b9deb3f2cf9fcf05ff3b038` | renommage et adaptations : docstring et aide (v12) ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `tests/mutants/sched.json` | `tests/mutants/sched.json` | `77bef38960dd139ac8895474dd67ab3773dd5ba4a442265bf8deffb57d638d39` | renommage seul |
| `tests/mutants/test_run_mutants.py` | `tests/mutants/test_run_mutants.py` | `4cb37a0bd370c7ffed5ef9fdaf81f6896c6cf1bac376769b3a73ab83f4e7ca80` | renommage et adaptations : texte d'usage (racine v12) |
| `tests/num/README.md` | `tests/num/README.md` | `503f65d2fa65a5ce8f020b55870e288b5101d91ebd32125111836a78dfe03017` | renommage et adaptations : note de port ; profils ; liens vers les reçus et documents de la v11 ; vérification de `radical_port.py` et portes `roots_cost` (non portées) décrites pour la v12 |
| `tests/num/big_gate.py` | `tests/num/big_gate.py` | `479a94eff2d45e424e222145905e59685ec7d2a1df142934fee1658e404df076` | renommage seul |
| `tests/num/big_io.hpp` | `tests/num/big_io.hpp` | `a9f7d84aca9e5169651d250c873754f0dcb6a207f5fbbd6f4431fb90265bd68f` | renommage seul |
| `tests/num/big_probe.cpp` | `tests/num/big_probe.cpp` | `c533a57bd4119cd7f0963bcc94ceb0d3e27d2b2312c00b20bf29940305cbf824` | renommage seul |
| `tests/num/big_test.cpp` | `tests/num/big_test.cpp` | `cff9e7f86d59e6d514b7285cc26eb8c3be171facffb23a376e3ceeff67086cb8` | renommage seul |
| `tests/num/bounds_oracle.py` | `tests/num/bounds_oracle.py` | `e7af4fc1fa7e7fb9fb65c2e63f27efb6c7af9331a7ca67f5fc1ec3188c1199c6` | renommage et adaptations : profils (21, 24) ; attendu « branche large » sans le cas 18 |
| `tests/num/bounds_probe.cpp` | `tests/num/bounds_probe.cpp` | `b70861cd18d95720477e4f2a2951f9c33765f0b731cdf68ce82afe2ee3b38a0c` | renommage seul |
| `tests/num/bounds_test.cpp` | `tests/num/bounds_test.cpp` | `9f0c60f6d20b4704f3ee7792388a48799e183c11b7006be18020b90b92d7cf88` | renommage seul |
| `tests/num/candidate_test.cpp` | `tests/num/candidate_test.cpp` | `80d196cad9b9f8468e1b8cdd5ebee70de7ea4b5c004371a7e4b2be4aa8b99a72` | renommage seul |
| `tests/num/center_region_model_test.py` | `tests/num/center_region_model_test.py` | `7d5d6c0c4b6862cd9df40e738169cf3ba32765f1b52b971dcd456572e3ae899c` | renommage et adaptations : profils (21, 24) |
| `tests/num/center_region_oracle.py` | `tests/num/center_region_oracle.py` | `11d5b602736b02ee8c85ffb8892bc7da58465571fae17bd16c3d3fedf8b1ffec` | renommage et adaptations : en-tête du pilote : profils 21 ou 24 |
| `tests/num/center_region_probe.cpp` | `tests/num/center_region_probe.cpp` | `7f116bfd7f4dad75ffb0a27bdaaa98d2087bdfeb7a09e7cf2ae216cf65514497` | renommage seul |
| `tests/num/center_region_test.cpp` | `tests/num/center_region_test.cpp` | `5b8dc9261e3d4a43cfa7479b03d50778ca17f616fb16c201c22eb2de591e925a` | renommage et adaptations : attendus du profil 18 retirés |
| `tests/num/centers_oracle.py` | `tests/num/centers_oracle.py` | `1bb6fe775cfda3fa082b6f8a3a273e20a450fed3404790b9629c29651b939e8c` | renommage et adaptations : profils (21, 24) |
| `tests/num/centers_probe.cpp` | `tests/num/centers_probe.cpp` | `1914f03739886025462dbd60f7d027dc5f5a768ff0e41564b82ba70a57b0b996` | renommage seul |
| `tests/num/centers_test.cpp` | `tests/num/centers_test.cpp` | `15570b6f23884cfb326c6c19902c9051e3bf4022ca613e26df37bf7e128f5f3b` | renommage seul |
| `tests/num/checked_power_oracle.py` | `tests/num/checked_power_oracle.py` | `cf2f37023a8586e61b82baa55d47335abce2091bb8e8a5df1101601942bcb693` | renommage et adaptations : profils (21, 24) ; branches du profil 18 retirées |
| `tests/num/checked_power_probe.cpp` | `tests/num/checked_power_probe.cpp` | `7bab27aecf4d175ef4b5605836e8d3d56e94870aee6beac2d4df04d4b38d1a71` | renommage seul |
| `tests/num/checked_power_support.hpp` | `tests/num/checked_power_support.hpp` | `159161b6fe190c1b336e6d1e139f8ac8c8030b443d98aef8a4af94623db36f96` | renommage seul |
| `tests/num/checked_power_test.cpp` | `tests/num/checked_power_test.cpp` | `1ff8a8ea911d51c459facc2c1a11a04c1f1a317612eb0c146a3b56033128e808` | renommage et adaptations : attendus du profil 18 retirés ; bloc 21/24 sans condition |
| `tests/num/distance_oracle.py` | `tests/num/distance_oracle.py` | `446485a44527cec579cf16573b4843cfa973b3ccc9eabe3e1ae3fdc5c30a430e` | renommage et adaptations : profils (21, 24) ; largeur hexadécimale fixe (48) |
| `tests/num/distance_probe.cpp` | `tests/num/distance_probe.cpp` | `a1007b5379ee891babcc0ae324176c97067bef5f840dac7f092625a8d22ad32a` | renommage seul |
| `tests/num/distance_test.cpp` | `tests/num/distance_test.cpp` | `2b1831762ced75d16d680b2f8fc6805512b324791fcc1a595de0ccd79a10ac9d` | renommage seul |
| `tests/num/fraction_oracle.py` | `tests/num/fraction_oracle.py` | `32a8bd7bac67c6c0eadccb88c6fccef66a45be9099321615c58de7142239db3e` | renommage et adaptations : profils (21, 24) |
| `tests/num/geometry_test.cpp` | `tests/num/geometry_test.cpp` | `6db13232aa964ead53b83524f0c06a6df4e5900a99adc7855fb88e87ee590c74` | renommage et adaptations : commentaire |
| `tests/num/integer_test.cpp` | `tests/num/integer_test.cpp` | `d03709b95af0fd0f60a4312c231258c2a17a595a24e95118c38a8b526776160d` | renommage et adaptations : contrôle `Budgets<18>` retiré ; plancher `budgets` 16 → 15 |
| `tests/num/lattice_bounds_test.cpp` | `tests/num/lattice_bounds_test.cpp` | `bf10b22cad388cf039c3c7735f0ad3e48edfd521b6d5b5288f863908d6db2e30` | renommage et adaptations : attendus du profil 18 retirés |
| `tests/num/orientation_certificate_oracle.py` | `tests/num/orientation_certificate_oracle.py` | `27085ad428ca9710e33866c7d7097515719be816bfab75dedd7d62fb1bbe7546` | renommage et adaptations : profils (21, 24) ; branches du profil 18 retirées |
| `tests/num/orientation_certificate_probe.cpp` | `tests/num/orientation_certificate_probe.cpp` | `caa89c61cd238b1c6c90a0b002a1c4acd3b1f46eb8e8e1e1614e9ad6c6392143` | renommage seul |
| `tests/num/orientation_certificate_test.cpp` | `tests/num/orientation_certificate_test.cpp` | `d3eb449a441fc26a396697e593f3eeaae196aa7bc9f4b30d773ad62c0c81885f` | renommage et adaptations : attendus du profil 18 retirés ; bloc 21/24 sans condition |
| `tests/num/power_certificate_oracle.py` | `tests/num/power_certificate_oracle.py` | `4bcca2a7af84b6fd26d2ed680936f583a58a173733cf01fe40a47f81d248a51a` | renommage et adaptations : profils (21, 24) ; branches du profil 18 retirées |
| `tests/num/power_certificate_probe.cpp` | `tests/num/power_certificate_probe.cpp` | `351740024f227f6ebc1acc1b99a9afd4c1b5f245ce9cc0c2b8b3566e8e70f43b` | renommage seul |
| `tests/num/power_certificate_test.cpp` | `tests/num/power_certificate_test.cpp` | `06850b12de33e005d79a6af768b9ae47dd8614ac3903884d1cce5c2491376929` | renommage et adaptations : attendus du profil 18 retirés ; bloc 21/24 sans condition |
| `tests/num/power_reference.hpp` | `tests/num/power_reference.hpp` | `4b824081147ff40ca84872c020b3b1f142c7d60b715d94e30ef652d85181521e` | renommage seul |
| `tests/num/power_test.cpp` | `tests/num/power_test.cpp` | `c1e7c61f04709430bfb8677c9bc6af5c2f9ecc0aebb52a2367fc7d96f4602ead` | renommage et adaptations : attendus du profil 18 retirés (tailles, longueurs en bits) |
| `tests/num/probe.cpp` | `tests/num/probe.cpp` | `0105d710cc8b5bc0f5f527651468a47931d6c67bf8c028721ba30a85bcffd36c` | renommage seul |
| `tests/num/q3_candidate_test.cpp` | `tests/num/q3_candidate_test.cpp` | `77571414079ad2b6aace9572014365e1da9232837f6c9065ed53c6d4bd48c17c` | renommage et adaptations : attendus du profil 18 retirés |
| `tests/num/q4_presentation_oracle.py` | `tests/num/q4_presentation_oracle.py` | `c9227491023f3207bfc5cab9b05f379ec8b0ef252ac454233775c2e1b87e04f3` | renommage et adaptations : profils (21, 24) ; branche du profil 18 retirée |
| `tests/num/q4_presentation_probe.cpp` | `tests/num/q4_presentation_probe.cpp` | `b33fe0fc6d2f9142a409547df4adf462825ece783f9344849df84861ab4cb037` | renommage seul |
| `tests/num/q4_presentation_process_test.py` | `tests/num/q4_presentation_process_test.py` | `1af4db26d56d3aa842cb3a6e2b90bde9e264c07c170b192308c28db75f203da1` | copie à l'identique |
| `tests/num/q4_presentation_test.cpp` | `tests/num/q4_presentation_test.cpp` | `d5a2d0a41a4484bdc4254e192fdf1b4bd127b6f8e637adc7334839de21f8d67a` | renommage et adaptations : branche du profil 18 retirée ; attendus du profil 18 retirés |
| `tests/num/radical_gate.py` | `tests/num/radical_gate.py` | `0c9e36ccc7a87df75520416d65ebd039f570c5e38bdf99aa62e24aee9d8405d7` | renommage et adaptations : un seul argument (la sonde) ; `rp.pin_mismatches()` au lieu de la comparaison au banc |
| `tests/num/radical_port.py` | `tests/num/radical_port.py` | `49e4d884276ff899ccc52c73f4de06bd0f5d0e203d88d3b52eded2e92ed75f38` | renommage et adaptations : `source_mismatches(chemin du banc)` remplacée par `pin_mismatches()` : le texte de chaque définition copiée est comparé au SHA-256 de son texte dans `bench/points_radius.py` de la v11 (`ac081a06f`), banc que la v12 ne porte pas ; les huit définitions étaient identiques au texte du banc au moment du port |
| `tests/num/radical_probe.cpp` | `tests/num/radical_probe.cpp` | `4a68c3265508a32029f5d08a1223aa5c77ef2cc3882201f8c77f4b8ac8dd6fa9` | renommage seul |
| `tests/num/refusal_probe.cpp` | `tests/num/refusal_probe.cpp` | `c92687c0ab7a2b9f960ca99fbbadf9f31b38b49f242dc09140cf92c31ec8edfc` | renommage seul |
| `tests/num/roots_gate.py` | `tests/num/roots_gate.py` | `94e31c74530486ad4277060ba6d88e3c73ad40832bc5ac8a2ba0e3deb8708511` | renommage et adaptations : un seul argument (la sonde) ; `rp.pin_mismatches()` au lieu de la comparaison au banc |
| `tests/num/roots_probe.cpp` | `tests/num/roots_probe.cpp` | `e1e2cb0f601c85acd2aaa7253b7ee9dbfa5520ef6ac33c61f5e7cd18f67c9470` | renommage seul |
| `tests/num/source_pins.json` | `tests/num/source_pins.json` | `22f7e1aa3d6f082af63b724c32c6ac8c53a0fd69b5ab34ff5e495b125df403b1` | copie à l'identique ; registre de provenance v10 → v11, gardé tel quel (historique ; noms `mhgp11_*` conservés) |
| `tests/num/tests.cmake` | `tests/num/tests.cmake` | `9f706ede943ce981b237479799c1952b03b77396e570d34a8e5a0707b80f9ed8` | renommage et adaptations : attendus du profil 18 retirés (`q4_presentation` 491/14213) ; lignes des auto-tests des modèles recalculées sans le profil 18 ; portes `radical` et `roots` sans l'argument `bench/points_radius.py` (empreinte épinglée) ; bloc `roots_cost` retiré (exige le catalogue, données `uniform_u18_*`) |
| `tests/num/triangle_kind_oracle.py` | `tests/num/triangle_kind_oracle.py` | `153fc06298b281f5c3a0133b3b8e7b0b0fdf1d5e8abed9e2d7140c589ded97da` | renommage et adaptations : profils (21, 24) |
| `tests/num/triangle_kind_probe.cpp` | `tests/num/triangle_kind_probe.cpp` | `3376e797b169708d897a055c7ab83474384f7de41ef3f7440ad65cb40f68ec3e` | renommage seul |
| `tests/num/triangle_kind_test.cpp` | `tests/num/triangle_kind_test.cpp` | `f3d265a3e134630ec8ac45a4bf426a030b183a0ab9da27a6d140ab719e671d54` | renommage seul |
| `tests/sched/README.md` | `tests/sched/README.md` | `289da03af411b770a484d82eec1b5633410c47ded6a43ff2905d6bd34f28c4a9` | renommage et adaptations : note de port ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `tests/sched/fault.cpp` | `tests/sched/fault.cpp` | `251fe7bcbc936a25e55d9ae30ce58c56bbad05f4cbc7e916fc9e8a6e725eef77` | renommage seul |
| `tests/sched/tests.cmake` | `tests/sched/tests.cmake` | `2259d1c4b575283a38c74e8f13ceac24371ab9ae361c869c7fc223cc5ea2cc57` | renommage seul |
| `tests/sched/unit.cpp` | `tests/sched/unit.cpp` | `d1f4047c31712f35f46bbd2235b6516a9323eb26c869e6c4e5f6bc74efcb00a8` | renommage seul |
| `tests/support/exit_probe.cpp` | `tests/support/exit_probe.cpp` | `7e803f2919bd97858ec2b6a1d4294ac57638a31b52af52af43b3ac06af4ddecc` | copie à l'identique |
| `tests/support/expect_abnormal_stop.py` | `tests/support/expect_abnormal_stop.py` | `53144917334d5ea1ff1fb4505cf4a7d29ca02ee67891ad6443b34615ce4f0220` | copie à l'identique |
| `tests/support/fake_abnormal_stop.py` | `tests/support/fake_abnormal_stop.py` | `3b2a1480da98718b8bc8644daa44999e6853a9e021811e5af34a3201dfd83b25` | copie à l'identique |
| `tests/support/fenv.hpp` | `tests/support/fenv.hpp` | `4fc251d19be87383a548e0308a0fb5eb0c11b68b208205604e15a01bdf0087c9` | renommage seul |
| `tests/support/framework_probe.cpp` | `tests/support/framework_probe.cpp` | `2d86082341f621e1c43f0170865146bd2b25154e7eee6cf70f523ea8b2eeaf33` | renommage seul |
| `tests/support/gate_fixture/CMakeLists.txt` | `tests/support/gate_fixture/CMakeLists.txt` | `1d5a5890e64da8a27e8326480aff67107564ba115d5e323e2bfa044df7606bb7` | renommage et adaptations : texte d'usage (racine v12) |
| `tests/support/gate_fixture/enfant/CMakeLists.txt` | `tests/support/gate_fixture/enfant/CMakeLists.txt` | `f7b4bd91acf25a6e85ee8a8bae68052e46275bff6c646e10d4d1d4b7c7f73f49` | renommage seul |
| `tests/support/mhgp12_gate.py` | `tests/support/mhgp11_gate.py` | `8812cb11d632534ed372a8ca7c88c7b3ad138920f1b46eeb5fd60222a2a2c4f0` | renommage et adaptations : docstring (v12) ; références de doctrine requalifiées « ARCHITECTURE.md de la v11 » |
| `tests/support/test.hpp` | `tests/support/test.hpp` | `ecc72e06eff0ac3133062d312f2d0424785588a30b8e307e5e58aa19ddf49c7f` | renommage et adaptations : en-tête (v12) |
| `tests/support/test_check_style.py` | `tests/support/test_check_style.py` | `2def70925d03d497c1255604430fc48c02755f4889808bf4c3dc33aea12cd39f` | renommage et adaptations : trois cas ajoutés pour la règle `[mhgp11]` (commentaire, namespace, CMake) ; plancher 133 → 139 |
| `tests/support/test_g4_matrix.py` | `tests/support/test_g4_matrix.py` | `13d03b42f539dd87777513a89692253051516294743b6fc447f73bbb1d1a7ad9` | renommage et adaptations : nom du dossier factice `morsehgp3D_v12` |
| `tests/support/test_gate_helper.py` | `tests/support/test_gate_helper.py` | `3867fb7e01864b92389a49aa61dda1157802d845fee2056d2b4d912e48ffef7e` | renommage seul |
| `tests/support/test_gate_properties.py` | `tests/support/test_gate_properties.py` | `0c3acf993e5da00113e2ce927976152965cd2c207c65db8ea912e0afd64892ca` | renommage et adaptations : texte d'usage (racine v12) |
| `tests/support/tests.cmake` | `tests/support/tests.cmake` | `f2b3af0302cff3653ea7039fc146be0a1096caa223c034784abb14669903f613` | renommage et adaptations : porte `g4_matrix` rangée sous son propre titre ; portes ajoutées `mhgp12_support_configure_coord_bits_18` et `_32` (refus à la configuration) |
| `tools/check_style.py` | `tools/check_style.py` | `2fee2f1ffed99e6ef7e2f6be06633f31944118420027f3539de682988bcd60ff` | renommage et adaptations : table des modules lue au § 5 ; règle `[mhgp11]` ajoutée (identifiants de la v11 refusés dans le C++ et le CMake, sur le modèle de `[mhgp10]`) ; textes v12 |
| `tools/g4_matrix.py` | `tools/g4_matrix.py` | `2c41d275732618539fc483962139c8a844c10636628b827dfd74cb2705dd9c21` | renommage et adaptations : docstring et aide : `morsehgp3D_v12`, lanceur de session de la v12 à écrire ; `g4_matrix.json` de la v11 non porté (`--matrix` obligatoire d'ici là) |

## Non portés

| Élément de la v11 (`ac081a06f`) | Raison |
| --- | --- |
| `src/` et `tests/` de `catalogue`, `tower`, `supports`, `points`, `head`, `api`, et `cli/` | consigne : modules réécrits par la v12 |
| `tests/mutants/{catalogue,tower,supports,points,head,api,cli}.json` | manifestes des modules non portés |
| `tests/io/full_march_collector.py` | collecteur des reçus de la tour, enregistré par `tests/tower/tests.cmake` : aucune porte du socle |
| `tests/support/full_capture_reader_test.py` | lecteur des captures FULL, dépend de `bench/verify_full_captures.py` ; ce n'était pas une porte CTest |
| `tools/g4_matrix.json` | matrice de qualification G4 de la v11, centrée sur le profil 18 et sur les modules non portés ; `tools/g4_matrix.py` est porté (sa porte aussi), la matrice de la v12 s'écrira avec son lanceur de session |
| `tools/g4_prepare_host.py`, `tools/g4_prepare_host_test.py` | préparation de la VM G4, liée au lanceur de session de la v11 ; pas une porte CTest |
| `bench/index_g4.py`, `bench/index_collector_test.py`, `bench/index_asan18_matrix.json` | pilote G4 de l'index : exige les constructions qualifiées aux profils 18, 21 et 24 et le complément ASan 18 bits ; porte `mhgp11_index_bench_collector` (et sa jumelle `_opt`) retirée |
| `bench/catalogue_g4.py` et sa chaîne (`catalogue_profiles.py`, `catalogue_diagnostics.py`, `catalogue_semantic.py`, `semantic_cache.py`) | bancs du catalogue ; les deux fonctions dont `bench/index_io_test.py` avait besoin sont copiées dans `bench/index_semantic.py` |
| `bench/roots_cost.cpp` et les portes `mhgp11_num_roots_cost_*` | exigent le module `catalogue` (jamais enregistrées sans lui) ; données nommées au profil 18 (`uniform_u18_n*`) |
| `bench/points_radius.py` | banc de la hiérarchie de points (numpy) ; la copie `tests/num/radical_port.py` des huit définitions qu'emploient les portes `mhgp12_num_radical` et `mhgp12_num_roots` est jugée contre l'empreinte épinglée de leur texte dans ce banc (une lecture de `../morsehgp3D_v11/` aurait cassé les copies des campagnes de mutants) |
| `.gitattributes` | ne concerne que des reçus de la v11 |
| option CMake `MHGP11_ENABLE_CUDA` | aucun module du socle ne compile pour le GPU ; elle reviendra avec le catalogue |

## Portes : retirées, ajoutées

Configurations comparées (Release, u21, unités `core num sched cloud io index reference`) : la v11 à `ac081a06f`
enregistre 404 portes (381 hors `long`), le socle 406 (383 hors `long`). Différence exacte :

- **retirées (2)** : `mhgp11_index_bench_collector` et `mhgp11_index_bench_collector_opt` (pilote G4 propre au profil 18,
  voir ci-dessus) ;
- **ajoutées (4)** : `mhgp12_core_coord_bits_18_refusal` et `mhgp12_core_coord_bits_32_refusal` (refus à la compilation),
  `mhgp12_support_configure_coord_bits_18` et `mhgp12_support_configure_coord_bits_32` (refus à la configuration).
- **ajoutées à l'intégration (6)**, hors du port (`a0091e2b7`, `CST-0115`) : `mhgp12_reference_witness_t1`, son
  mutant `mhgp12_reference_witness_t1_mutant_sans_inclusion` et son refus `mhgp12_reference_witness_t1_refusal`, avec
  leurs trois jumelles `-O` : le dépôt enregistre donc 412 portes (389 hors `long`) ; avec la v10 figée, 26 portes
  `diff_v10` de plus, d'où les 415 portes hors `long` jouées à l'intégration.

Attendus retirés avec le profil 18, sans retirer de porte : contrôles `Budgets<18>` (`tests/num/integer_test.cpp`,
plancher 16 → 15), clé de Morton à 18 bits (`tests/cloud/cloud_test.cpp`, plancher 16 → 15), branches et attendus
`kCoordBits == 18` des tests `num` (C++ et oracles Python), comptes gravés du profil 18 (`q4_presentation` 491/14213),
profil 18 des auto-tests des modèles Python (lignes recalculées : `q4_presentation_model` 1761/53877/114 →
1270/39662/76, `triangle_kind_model` 852/1717/72 → 568/1148/48, `borrowed_model` 3030/36 → 2020/24, chaque nouveau
compte égal à l'ancien moins la part du profil 18 mesurée séparément sur le modèle de la v11).

Mutants (`tests/mutants/core.json`, plancher 82 → 84) : `profil_de_20_bits_admis` recalé sur la garde 21/24 ;
`profil_18_readmis` et `profil_32_admis` ajoutés. Les manifestes `num`, `sched`, `cloud`, `io`, `index` ne changent que
par le renommage.

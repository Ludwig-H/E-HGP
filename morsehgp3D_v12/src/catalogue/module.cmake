# Module catalogue (tranche T1) : parcours en largeur et feuilles J3 en source unique (en-tetes), lots de feuilles,
# fin d'etage commune aux voies CPU et appareil (en-tetes finish_*), table S* -> boule, export MHGP12DP ; voie
# appareil (tranche T1-b, en-tetes device_*) : executeur CUDA si MHGP12_ENABLE_CUDA, sinon souche qui rend
# device_unavailable. Sorties en flux de la fin d'etage (tranche T2-d) : premier toucher des sorties (outputs.cpp).
mhgp12_module_sources(traversal.cpp leaves.cpp assemble.cpp table.cpp catalogue.cpp export.cpp outputs.cpp)
if(MHGP12_ENABLE_CUDA)
  mhgp12_module_sources(device_cuda.cu device_leaf.cu)
  target_compile_definitions(mhgp12 PUBLIC MHGP12_HAVE_CUDA=1)
  target_link_libraries(mhgp12 PUBLIC CUDA::cudart_static)
else()
  mhgp12_module_sources(device_stub.cpp)
endif()

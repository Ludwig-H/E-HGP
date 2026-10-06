# Module catalogue : reference sequentielle et frontiere possedee pour le Pool de la Session.
mhgp11_module_sources(catalogue.cpp boxes.cpp leaf.cpp support.cpp assemble.cpp assembly_parallel.cpp sort_indices.cpp frontier.cpp
                      adaptive_frontier.cpp adaptive_prepare.cpp adaptive_replay.cpp parallel.cpp single_pass.cpp
                      single_pass_batch.cpp leaf_batch.cpp leaf_overlap.cpp)
# Voie GPU (option) : executeur CUDA du lot de feuilles ; sans elle, run_leaf_batch_cuda refuse avant tout calcul.
if(MHGP11_ENABLE_CUDA)
  mhgp11_module_sources(leaf_batch_cuda.cu leaf_batch_cuda_context.cu)
  target_compile_definitions(mhgp11 PUBLIC MHGP11_HAVE_CUDA=1)
  target_compile_options(mhgp11 PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr -lineinfo -Xptxas=-v>)
  target_link_libraries(mhgp11 PUBLIC CUDA::cudart_static)
endif()

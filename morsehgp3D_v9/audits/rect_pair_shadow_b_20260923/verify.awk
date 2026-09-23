function fail(msg) { print "FAIL " msg > "/dev/stderr"; exit 1 }
$1 == "META" {
  if ($2 != 123389 || $4 != 6175011 || $5 != 238364135 ||
      $6 != 2548453 || $7 != 22034426 || $8 != 503488729 ||
      $9 != 3986433 || $10 != 559661741 || $11 != 1771 || $12 != 5819562)
    fail("global pinned ledger");
}
$1 == "STRATUM" {
  population[$2] = $3; product_mass[$2] = $4; sample[$2] = $5;
}
$1 == "ROW" {
  s = $2; rows[s]++; product[s] += $4;
  if ($4 != $5 * $6 || $12 > 32 || $13 > 512 ||
      $17 > $12 * ($12 - 1) / 2 || $19 > $18 || $22 > $21 ||
      $20 > 64 * $18 || $23 > 16 || $24 > 16 ||
      $35 < $23 || $36 < $24 || $35 > $12 / 2 || $36 > $12 / 2)
    fail("row work or geometry invariant");
  if ($25 != (($7 == 4) || $23 >= 4) ||
      $26 != (($7 == 2) || $24 >= 3))
    fail("lane threshold");
  if (s >= 3) {
    max_close3 = (($7 == 4) || $35 >= 4);
    max_close4 = (($7 == 2) || $36 >= 3);
    heavy++; heavy_product += $4; edges += $8; forms += $9;
    pops += $13; boxes += $14; sites += $15; cuts += $16;
    pairs += $17; first3 += $18; first4 += $19;
    corners += $20; accepted3 += $21; accepted4 += $22;
    singleton_corners += $27;
    select_ns += $32; pair_ns += $33; singleton_ns += $34;
    if ($25 && $26) {
      full++;
      if ($8 > 0) { positive_full++; full_edges += $8; full_forms += $9; }
    }
    if (max_close3 && max_close4) {
      max_full++;
      if ($8 > 0) {
        max_positive_full++; max_full_edges += $8;
        max_full_forms += $9; max_positive_product += $4;
      } else max_empty_product += $4;
    }
    if ($8 > 0) {
      positive++; if ($8 >= 16) { segment16++; f16 += $9;
        if ($25 && $26) { full16++; full16_forms += $9; } }
      if ($7 == 6 && $25 && !$26) { partial3++; partial3_forms += $9; }
      if ($7 == 6 && !$25 && $26) { partial4++; partial4_forms += $9; }
      if ($30 && $31) singleton_positive_full++;
      if ($7 == 6 && max_close3 && !max_close4) {
        max_partial3++; max_partial3_forms += $9;
      }
      if ($7 == 6 && !max_close3 && max_close4) max_partial4++;
    } else empty++;
  }
}
END {
  for (s = 0; s <= 5; s++)
    if (rows[s] != sample[s] || (s >= 3 &&
        (rows[s] != population[s] || product[s] != product_mass[s])))
      fail("stratum census");
  if (heavy != 1747 || heavy_product != 5815031 || edges != 81089 ||
      forms != 397354920 || positive != 299 || empty != 1448 ||
      full != 1037 || positive_full != 69 || full_edges != 2022 ||
      full_forms != 4140538 || partial3 != 48 || partial3_forms != 11260426 ||
      partial4 != 0 || singleton_positive_full != 0 ||
      segment16 != 202 || f16 != 396309030 || full16 != 30 ||
      full16_forms != 3754851)
    fail("heavy aggregate");
  if (max_full != 1051 || max_positive_full != 72 ||
      max_full_edges != 2175 || max_full_forms != 5059809 ||
      max_positive_product != 193443 || max_empty_product != 4033983 ||
      max_partial3 != 46 || max_partial3_forms != 11290428 ||
      max_partial4 != 0)
    fail("maximum-matching aggregate");
  printf("PASS heavy=%d positive=%d empty=%d full_positive=%d F=%d/%d partial_q3=%d singleton_positive_full=%d\n", heavy,positive,empty,positive_full,full_forms,forms,partial3,singleton_positive_full);
  printf("work pops=%.0f boxes=%.0f sites=%.0f cuts=%.0f pairs=%.0f first_q3=%.0f first_q4=%.0f corners=%.0f accepted_q3=%.0f accepted_q4=%.0f singleton_corners=%.0f\n",pops,boxes,sites,cuts,pairs,first3,first4,corners,accepted3,accepted4,singleton_corners);
  printf("timed_ns selection=%.0f paired=%.0f singleton=%.0f\n",select_ns,pair_ns,singleton_ns);
  printf("max_full=%d max_positive=%d edges=%.0f F=%.0f positive_product=%.0f empty_product=%.0f max_partial_q3=%d F=%.0f\n",max_full,max_positive_full,max_full_edges,max_full_forms,max_positive_product,max_empty_product,max_partial3,max_partial3_forms);
}

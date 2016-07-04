.set basename csp
..
.set wantdefs
..
.set i
.append i csp
.append i csp_sentence csp_not_sentence csp_or_sentence csp_and_sentence csp_impl_sentence csp_lt_sentence csp_equiv_sentence csp_atomic_sentence
.append i csp_term csp_function_term csp_variable_term csp_constant_term
.append i csp_hierarchy csp_flat_kb
..
.foreach t $i
.append wantdefs $t
.append wantdefs cmp_$t
.append wantdefs fre_$t
.append wantdefs fscan_$t
.append wantdefs fwrite_$t
.append wantdefs fread_$t
.append wantdefs isequal_$t
.append wantdefs new_$t
.append wantdefs fprint_$t
.append wantdefs rdup_$t
.append wantdefs rfre_$t
..
.append wantdefs $t_list
.append wantdefs append_$t_list
.append wantdefs cmp_$t_list
.append wantdefs concat_$t_list
.append wantdefs delete_$t_list
.append wantdefs fre_$t_list
.append wantdefs fscan_$t_list
.append wantdefs fwrite_$t_list
.append wantdefs fread_$t_list
.append wantdefs insert_$t_list
.append wantdefs isequal_$t_list
.append wantdefs new_$t_list
.append wantdefs fprint_$t_list
.append wantdefs rdup_$t_list
.append wantdefs reverse_$t_list
.append wantdefs rfre_$t_list
.append wantdefs setroom_$t_list
.endforeach


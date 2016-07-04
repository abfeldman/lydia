.set basename mv
..
.set wantdefs
..
.set i
.append i mv_wff mv_cnf mv_dnf 
.append i mv_wff_expr mv_wff_e_not mv_wff_e_and mv_wff_e_or
.append i mv_wff_e_impl mv_wff_e_equiv mv_wff_e_var mv_wff_e_const 
.append i mv_clause mv_term mv_literal
.append i mv_wff_hierarchy mv_cnf_hierarchy mv_dnf_hierarchy
.append i mv_wff_flat_kb mv_cnf_flat_kb mv_dnf_flat_kb
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
.endforeach
..
.set i
.append i mv_wff mv_cnf mv_dnf 
.append i mv_wff_expr mv_wff_e_not mv_wff_e_and mv_wff_e_or
.append i mv_wff_e_impl mv_wff_e_equiv mv_wff_e_var mv_wff_e_const 
.append i mv_clause mv_term mv_literal
.append i mv_wff_hierarchy mv_cnf_hierarchy mv_dnf_hierarchy
.append i mv_term_list
..
.foreach t $i
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


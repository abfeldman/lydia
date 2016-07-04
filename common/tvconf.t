.set basename tv
..
.set wantdefs
..
.set i
.append i tv_wff tv_nnf tv_cnf tv_dnf tv_nf horn
.append i tv_wff_expr tv_wff_e_not tv_wff_e_and tv_wff_e_or
.append i tv_wff_e_impl tv_wff_e_equiv tv_wff_e_var tv_wff_e_const 
.append i tv_clause tv_term tv_literal_set material_implication
.append i tv_nnf_expr tv_nnf_e_not tv_nnf_e_and tv_nnf_e_or tv_nnf_e_var
.append i tv_wff_hierarchy tv_nnf_hierarchy tv_cnf_hierarchy tv_dnf_hierarchy horn_hierarchy
.append i tv_wff_flat_kb tv_nnf_flat_kb tv_cnf_flat_kb tv_dnf_flat_kb horn_flat_kb
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
.append i tv_wff tv_nnf tv_cnf tv_dnf tv_nf horn
.append i tv_wff_expr tv_wff_e_not tv_wff_e_and tv_wff_e_or
.append i tv_wff_e_impl tv_wff_e_equiv tv_wff_e_var tv_wff_e_const 
.append i tv_clause tv_term tv_literal_set material_implication
.append i tv_wff_hierarchy tv_nnf_hierarchy tv_cnf_hierarchy tv_dnf_hierarchy horn_hierarchy
.append i tv_nnf_expr tv_nnf_e_not tv_nnf_e_and tv_nnf_e_or tv_nnf_e_var horn_flat_kb
.append i tv_term_list
..
.foreach t $i
.append wantdefs $t_list
.append wantdefs append_$t_list
.append wantdefs cmp_$t_list
.append wantdefs concat_$t_list
.append wantdefs delete_$t_list
.append wantdefs extract_$t_list
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
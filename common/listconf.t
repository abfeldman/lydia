.set basename list
..
.set wantdefs
..
.set i lydia_symbol int double int_list double_list int_list_list double_list_list lydia_bool
..
.foreach t $i
.append wantdefs $t_list
.append wantdefs append_$t_list
.append wantdefs cmp_$t_list
.append wantdefs concat_$t_list
.append wantdefs delete_$t_list
.append wantdefs deletelist_$t_list
.append wantdefs fre_$t_list
.append wantdefs fscan_$t_list
.append wantdefs insert_$t_list
.append wantdefs isequal_$t_list
.append wantdefs new_$t_list
.append wantdefs fprint_$t_list
.append wantdefs rdup_$t_list
.append wantdefs reverse_$t_list
.append wantdefs rfre_$t_list
.append wantdefs setroom_$t_list
.append wantdefs fwrite_$t_list
.append wantdefs fread_$t_list
.endforeach


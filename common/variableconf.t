.set basename variable
..
.set wantdefs
..
.set i
.append i variable_attribute bool_variable_attribute int_variable_attribute float_variable_attribute enum_variable_attribute
.append i variable_assignment bool_variable_assignment int_variable_assignment float_variable_assignment enum_variable_assignment
.append i variable bool_variable int_variable float_variable enum_variable
.append i constant bool_constant int_constant float_constant enum_constant
.append i values_set kb faultmode identifier qualifier
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
.append i variable_attribute bool_variable_attribute int_variable_attribute float_variable_attribute enum_variable_attribute
.append i variable_assignment bool_variable_assignment int_variable_assignment float_variable_assignment enum_variable_assignment
.append i variable bool_variable int_variable float_variable enum_variable
.append i constant bool_constant int_constant float_constant enum_constant
.append i values_set faultmode identifier qualifier
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
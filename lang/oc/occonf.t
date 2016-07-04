.set basename obs

.set wantdefs

.set t
.append t obs_bool_type
.append t obs_choice
.append t obs_dump
.append t obs_expr ${subclasses obs_expr}
.append t obs_extent
.append t obs_instance
.append t obs_orig
.append t obs_orig_symbol
.append t obs_type
.append t obs_enum_type
.append t obs_variable_identifier
.append t obs_variable_qualifier

.foreach i $t
.append wantdefs $i
.append wantdefs new_$i
.append wantdefs rdup_$i
.append wantdefs fre_$i
.append wantdefs rfre_$i
.endforeach

.foreach i obs_choice obs_expr obs_extent obs_instance obs_variable_qualifier
.append wantdefs $i_list
.append wantdefs new_$i_list
.append wantdefs rdup_$i_list
.append wantdefs append_$i_list
.append wantdefs setroom_$i_list
.append wantdefs fre_$i_list
.append wantdefs rfre_$i_list
.endforeach

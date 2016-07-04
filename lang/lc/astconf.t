..File: lcconf.t
.set basename lc
.set wantdefs
.set notwantdefs
..
.append notwantdefs rfre_function_user_type_entry
.append notwantdefs rfre_system_user_type_entry
.append notwantdefs rfre_constant_user_type_entry
.append notwantdefs isequal_orig_symbol
..
.set i
.append i extent
.append i origin
.append i orig_symbol
.append i type ${subclasses type}
.append i model
.append i definition ${subclasses definition}
.append i formal
.append i local
.append i reference
.append i attribute
.append i predicate ${subclasses predicate}
.append i system_instantiation
.append i variable_instantiation
.append i attribute_instantiation
.append i switch_choice
.append i expr ${subclasses expr}
.append i choice
.append i user_type_entry ${subclasses user_type_entry}
.append i attribute_entry
.append i struct_entry
.append i variable_identifier
.append i variable_qualifier
.append i index_entry
.append i quantifier_entry
..
.foreach t $i
.append wantdefs $t
.if ${not ${member $t orig_symbol}}
.append wantdefs isequal_$t
.endif
.append wantdefs fre_$t
.append wantdefs new_$t
.append wantdefs fprint_$t
.append wantdefs rdup_$t
.if ${not ${member $t rfre_function_user_type_entry rfre_system_user_type_entry rfre_constant_user_type_entry}}
.append wantdefs rfre_$t
.endif
..
.append wantdefs $t_list
.append wantdefs isequal_$t_list
.append wantdefs append_$t_list
.append wantdefs concat_$t_list
.append wantdefs delete_$t_list
.append wantdefs fre_$t_list
.append wantdefs insert_$t_list
.append wantdefs new_$t_list
.append wantdefs fprint_$t_list
.append wantdefs rdup_$t_list
.append wantdefs rfre_$t_list
.append wantdefs setroom_$t_list
.endforeach
..
.append wantdefs extent_list_list
.append wantdefs isequal_extent_list_list
.append wantdefs append_extent_list_list
.append wantdefs fre_extent_list_list
.append wantdefs new_extent_list_list
.append wantdefs fprint_extent_list_list
.append wantdefs rdup_extent_list_list
.append wantdefs rfre_extent_list_list
.append wantdefs setroom_extent_list_list
..
.append wantdefs insertlist_variable_identifier_list
.append wantdefs insertlist_expr_list
.append wantdefs insertlist_formal_list
.append wantdefs insertlist_local_list
.append wantdefs insertlist_attribute_list
..
.append wantdefs extract_definition_list


// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

function ifc_explorer_clear_content(content) {
    remove_all_children(content);
}

function update_history_sidebar() {
    const elements = ifc_explorer.history.stack.map(e => {
        if (e instanceof DeclIndex) {
            const sort = sort_to_string(DeclIndex, e.sort);
            const call_func = `onclick='set_ifc_explorer_selected_decl({ sort: ${e.sort}, index: ${e.index}}, true)'`;
            return `<a ${ifc_explorer_css_class} ${call_func}>DeclIndex{${e.sort}(${sort}),${e.index}}</a>`;
        }
        else if (e instanceof ExprIndex) {
            const sort = sort_to_string(ExprIndex, e.sort);
            const call_func = `onclick='set_ifc_explorer_selected_expr({ sort: ${e.sort}, index: ${e.index}}, true)'`;
            return `<a ${ifc_explorer_css_class} ${call_func}>ExprIndex{${e.sort}(${sort}),${e.index}}</a>`;
        }
        else if (e instanceof TypeIndex) {
            const sort = sort_to_string(TypeIndex, e.sort);
            const call_func = `onclick='set_ifc_explorer_selected_type({ sort: ${e.sort}, index: ${e.index}}, true)'`;
            return `<a ${ifc_explorer_css_class} ${call_func}>TypeIndex{${e.sort}(${sort}),${e.index}}</a>`;
        }
        return "unknown";
    })
        // Reverse this list so that it will be displayed as the most recent add at the top.
        .reverse()
        .join('<br />');
    ifc_explorer_clear_content(ifc_explorer.history.sidebar);
    const container = document.createElement("span");
    container.innerHTML = elements;
    ifc_explorer.history.sidebar.appendChild(container);
}

function update_history_with_new_element(T, sort, index) {
    // Remove the oldest element if we've reached the maximum capacity.
    if (ifc_explorer.history.stack.length + 1 > ifc_explorer.history.max_history) {
        ifc_explorer.history.stack.shift();
    }
    ifc_explorer.history.stack.push(index_from_raw(T, sort, index));
    update_history_sidebar();
}

const ifc_explorer_css_class = "class='explorer-element'";

class IFCExplorerJSONReplacer {
    static replace_text_offset(offset) {
        const text = sgraph.resolver.resolve_text_offset(offset);
        return `(${offset.offset}) = ${text}`;
    }

    static replace_name_index(index) {
        const text = sgraph.resolver.resolve_name_index(index);
        const sort = sort_to_string(NameIndex, index.sort);
        return `NameIndex{${index.sort}(${sort}),${index.index}} = '${text}'`;
    }

    static replace_locus(locus) {
        const resolved_locus = new ResolvedLocus(locus, sgraph.resolver);
        return `Locus{${locus.line}(line)} = ${resolved_locus.file}(${resolved_locus.line},${resolved_locus.column})`
    }

    static replace_decl_index(index) {
        if (null_index(index))
            return "DeclIndex{null}";
        const sort = sort_to_string(DeclIndex, index.sort);
        const call_func = `onclick='set_ifc_explorer_selected_decl({ sort: ${index.sort}, index: ${index.index}}, false)'`;
        return `<a ${ifc_explorer_css_class} ${call_func}>DeclIndex{${index.sort}(${sort}),${index.index}}</a>`;
    }

    static replace_type_index(index) {
        if (null_index(index))
            return "TypeIndex{null}";
        const sort = sort_to_string(TypeIndex, index.sort);
        const call_func = `onclick='set_ifc_explorer_selected_type({ sort: ${index.sort}, index: ${index.index}}, false)'`;
        return `<a ${ifc_explorer_css_class} ${call_func}>TypeIndex{${index.sort}(${sort}),${index.index}}</a>`;
    }

    static replace_expr_index(index) {
        if (null_index(index))
            return "ExprIndex{null}";
        const sort = sort_to_string(ExprIndex, index.sort);
        const call_func = `onclick='set_ifc_explorer_selected_expr({ sort: ${index.sort}, index: ${index.index}}, false)'`;
        return `<a ${ifc_explorer_css_class} ${call_func}>ExprIndex{${index.sort}(${sort}),${index.index}}</a>`;
    }

    static replace_dir_index(index) {
        if (null_index(index))
            return "DirIndex{null}";
        const symbolic = symbolic_for_dir_sort(index.sort);
        const symbolic_dir = sgraph.resolver.read(symbolic, index.index);
        const sort = sort_to_string(DirIndex, index.sort);
        const dir_index_str = `DirIndex{${index.sort}(${sort}),${index.index}}`;
        return { index: dir_index_str, value: symbolic_dir };
    }

    static replace_vendor_index(index) {
        // Note: VendorIndex utilizes 0-based indexing.
        const symbolic = symbolic_for_vendor_sort(index.sort);
        const symbolic_vendor = sgraph.resolver.read(symbolic, index.index);
        const sort = sort_to_string(VendorIndex, index.sort);
        const vendor_index_str = `VendorIndex{${index.sort}(${sort}),${index.index}}`;
        return { index: vendor_index_str, value: symbolic_vendor };
    }

    static replace_lit_index(index) {
        const literal = sgraph.resolver.resolve_lit_index(index);
        const sort = sort_to_string(LitIndex, index.sort);
        const lit_index_str = `LitIndex{${index.sort}(${sort}),${index.index}}`;
        if (index.sort == LitIndex.Sort.Immediate)
            return `${lit_index_str} = ${literal.value}`;
        return { index: lit_index_str, value: literal };
    }

    static replace_chart_index(index) {
        if (null_index(index))
            return "ChartIndex{null}";
        const chart = sgraph.resolver.resolve_chart_index(index);
        const sort = sort_to_string(ChartIndex, index.sort);
        const chart_index_str = `ChartIndex{${index.sort}(${sort}),${index.index}}`;
        return { index: chart_index_str, value: chart };
    }

    static replace_unilevel_chart(chart) {
        const elms = sgraph.resolver.resolve_sequence(chart.seq);
        return { requirement: chart.requirement, parameters: elms };
    }

    static replace_operator(op) {
        const sort = sort_to_string(Operator, op.sort);
        console.log(op);
        return { sort: sort, value: op.value };
    }

    static replace_heap_seq(seq) {
        const resolved_seq = sgraph.resolver.resolve_heap(seq);
        return resolved_seq;
    }

    static replace_scope_index(index) {
        if (null_scope(index)) {
            return { scope_index: index.value, decls: [] };
        }
        const decls = sgraph
                        .resolver
                        .decls_for_scope(index)
                        .map(x => x.decl);
        return { scope_index: index.value, decls: decls };
    }

    static generic_bitset_to_string(T, bitset) {
        return bitset_to_string(T, bitset.value);
    }

    static generic_value_to_string(T, value) {
        const str = value_to_string(T, value.value);
        return `${str}(${value.value})`;
    }
}

function ifc_explorer_json_replacer(key, value) {
    if (value instanceof DeclIndex)
        return IFCExplorerJSONReplacer.replace_decl_index(value);
    if (value instanceof TypeIndex)
        return IFCExplorerJSONReplacer.replace_type_index(value);
    if (value instanceof ExprIndex)
        return IFCExplorerJSONReplacer.replace_expr_index(value);
    if (value instanceof DirIndex)
        return IFCExplorerJSONReplacer.replace_dir_index(value);
    if (value instanceof VendorIndex)
        return IFCExplorerJSONReplacer.replace_vendor_index(value);
    if (value instanceof TextOffset)
        return IFCExplorerJSONReplacer.replace_text_offset(value);
    if (value instanceof NameIndex)
        return IFCExplorerJSONReplacer.replace_name_index(value);
    if (value instanceof LitIndex)
        return IFCExplorerJSONReplacer.replace_lit_index(value);
    if (value instanceof ChartIndex)
        return IFCExplorerJSONReplacer.replace_chart_index(value);
    if (value instanceof UnilevelChart)
        return IFCExplorerJSONReplacer.replace_unilevel_chart(value);
    if (value instanceof SourceLocation)
        return IFCExplorerJSONReplacer.replace_locus(value);
    if (value instanceof ScopeIndex)
        return IFCExplorerJSONReplacer.replace_scope_index(value);
    if (value instanceof Operator)
        return IFCExplorerJSONReplacer.replace_operator(value);
    if (value instanceof HeapSequence)
        return IFCExplorerJSONReplacer.replace_heap_seq(value);
    if (value instanceof BasicSpecifiers)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(BasicSpecifiers, value);
    if (value instanceof ScopeTraits)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(ScopeTraits, value);
    if (value instanceof ReachableProperties)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(ReachableProperties, value);
    if (value instanceof FunctionTraits)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(FunctionTraits, value);
    if (value instanceof FunctionTypeTraits)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(FunctionTypeTraits, value);
    if (value instanceof Qualifier)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(Qualifier, value);
    if (value instanceof Phases)
        return IFCExplorerJSONReplacer.generic_bitset_to_string(Phases, value);
    if (value instanceof TypeBasis)
        return IFCExplorerJSONReplacer.generic_value_to_string(TypeBasis, value);
    if (value instanceof TypePrecision)
        return IFCExplorerJSONReplacer.generic_value_to_string(TypePrecision, value);
    if (value instanceof TypeSign)
        return IFCExplorerJSONReplacer.generic_value_to_string(TypeSign, value);
    if (value instanceof CallingConvention)
        return IFCExplorerJSONReplacer.generic_value_to_string(CallingConvention, value);
    if (value instanceof Access)
        return IFCExplorerJSONReplacer.generic_value_to_string(Access, value);
    if (value instanceof ReadExprKind)
        return IFCExplorerJSONReplacer.generic_value_to_string(ReadExprKind, value);
    if (value instanceof NiladicOperator)
        return IFCExplorerJSONReplacer.generic_value_to_string(NiladicOperator, value);
    if (value instanceof MonadicOperator)
        return IFCExplorerJSONReplacer.generic_value_to_string(MonadicOperator, value);
    if (value instanceof DyadicOperator)
        return IFCExplorerJSONReplacer.generic_value_to_string(DyadicOperator, value);
    if (value instanceof TriadicOperator)
        return IFCExplorerJSONReplacer.generic_value_to_string(TriadicOperator, value);
    if (value instanceof VariadicOperator)
        return IFCExplorerJSONReplacer.generic_value_to_string(VariadicOperator, value);

    // Skip these instances.
    if (value instanceof StructPadding)
        return undefined;

    return value;
}

const SkipNavigation = {};

function set_ifc_explorer_selected_decl(index, from_history, skip_navigation) {
    if (null_index(index)) return;
    ifc_explorer_clear_content(ifc_explorer.decls.content);
    if (!from_history) {
        update_history_with_new_element(DeclIndex, index.sort, index.index);
    }

    if (!(skip_navigation != undefined && skip_navigation == SkipNavigation)) {
        navigate_to_decl_if_possible(index);
    }

    // Update edits.
    ifc_explorer.decls.sort_dropdown.selectedIndex = Array
                                                        .from(ifc_explorer.decls.sort_dropdown.options)
                                                        .findIndex(opt => opt.value == index.sort);
    ifc_explorer.decls.index_edit.value = index.index;

    const symbolic = symbolic_for_decl_sort(index.sort);
    const symbolic_decl = sgraph.resolver.read(symbolic, index.index);
    const json_str = JSON.stringify(symbolic_decl, ifc_explorer_json_replacer, 2);
    const container = document.createElement("pre");
    const sort = sort_to_string(DeclIndex, index.sort);
    container.innerHTML = `DeclIndex{${index.sort}(${sort}),${index.index}}\n${json_str}`;
    ifc_explorer.decls.content.appendChild(container);

    const tab = $(ifc_explorer.decls.tab);
    tab.tab("show");
}

function ifc_explorer_load_decl(e) {
    mark_edit_valid(ifc_explorer.decls.index_edit);
    const sort = parseInt(ifc_explorer.decls.sort_dropdown.value);
    const value = ifc_explorer.decls.index_edit.value;
    if (!valid_integral_value(value)) {
        mark_edit_invalid(ifc_explorer.decls.index_edit, ifc_explorer.decls.validation_tooltip, `"${value}" is not an integer value.`);
        return;
    }
    const index = parseInt(value);
    const decl_index = { sort: sort, index: index };
    const bounded = sgraph.resolver.decl_index_in_bounds(decl_index);
    if (!bounded.in_bounds) {
        const err = `DeclIndex{${sort}(${sort_to_string(DeclIndex, sort)}),${index}} is out of bounds (Cardinality: ${bounded.partition_size}).`;
        mark_edit_invalid(ifc_explorer.decls.index_edit, ifc_explorer.decls.validation_tooltip, err);
        return;
    }
    set_ifc_explorer_selected_decl(decl_index, false);
}

function ifc_explorer_init_decls() {
    Object.entries(DeclIndex.Sort)
          .sort(function(lhs, rhs) {
                if (lhs[0] < rhs[0])
                    return -1;
                if (lhs[0] > rhs[0])
                    return 1;
                return 0;
          })
          .forEach(element => {
                if (element[1] != DeclIndex.Sort.Count) {
                    var entry = document.createElement("option");
                    entry.textContent = `${element[0]} - ${element[1]}`;
                    entry.value = element[1];
                    ifc_explorer.decls.sort_dropdown.appendChild(entry);
                }
          });

    ifc_explorer.decls.index_edit.addEventListener("keyup", e => { if (e.key === "Enter") ifc_explorer_load_decl(e); });
    ifc_explorer.decls.load.addEventListener("click", e => ifc_explorer_load_decl(e));
}

function set_ifc_explorer_selected_type(index, from_history) {
    if (null_index(index)) return;
    ifc_explorer_clear_content(ifc_explorer.types.content);
    if (!from_history) {
        update_history_with_new_element(TypeIndex, index.sort, index.index);
    }

    // Update edits.
    ifc_explorer.types.sort_dropdown.selectedIndex = Array
                                                      .from(ifc_explorer.types.sort_dropdown.options)
                                                      .findIndex(opt => opt.value == index.sort);
    ifc_explorer.types.index_edit.value = index.index;

    const symbolic = symbolic_for_type_sort(index.sort);
    const symbolic_decl = sgraph.resolver.read(symbolic, index.index);
    const json_str = JSON.stringify(symbolic_decl, ifc_explorer_json_replacer, 2);
    const container = document.createElement("pre");
    const sort = sort_to_string(TypeIndex, index.sort);
    container.innerHTML = `TypeIndex{${index.sort}(${sort}),${index.index}}\n${json_str}`;
    ifc_explorer.types.content.appendChild(container);

    const tab = $(ifc_explorer.types.tab);
    tab.tab("show");
}

function ifc_explorer_load_type(e) {
    mark_edit_valid(ifc_explorer.types.index_edit);
    const sort = parseInt(ifc_explorer.types.sort_dropdown.value);
    const value = ifc_explorer.types.index_edit.value;
    if (!valid_integral_value(value)) {
        mark_edit_invalid(ifc_explorer.types.index_edit, ifc_explorer.types.validation_tooltip, `"${value}" is not an integer value.`);
        return;
    }
    const index = parseInt(value);
    const type_index = { sort: sort, index: index };
    const bounded = sgraph.resolver.type_index_in_bounds(type_index);
    if (!bounded.in_bounds) {
        const err = `TypeIndex{${sort}(${sort_to_string(TypeIndex, sort)}),${index}} is out of bounds (Cardinality: ${bounded.partition_size}).`;
        mark_edit_invalid(ifc_explorer.types.index_edit, ifc_explorer.types.validation_tooltip, err);
        return;
    }
    set_ifc_explorer_selected_type(type_index, false);
}

function ifc_explorer_init_types() {
    Object.entries(TypeIndex.Sort)
          .sort(function(lhs, rhs) {
                if (lhs[0] < rhs[0])
                    return -1;
                if (lhs[0] > rhs[0])
                    return 1;
                return 0;
          })
          .forEach(element => {
                if (element[1] != DeclIndex.Sort.Count) {
                    var entry = document.createElement("option");
                    entry.textContent = `${element[0]} - ${element[1]}`;
                    entry.value = element[1];
                    ifc_explorer.types.sort_dropdown.appendChild(entry);
                }
          });

    ifc_explorer.types.index_edit.addEventListener("keyup", e => { if (e.key === "Enter") ifc_explorer_load_type(e); });
    ifc_explorer.types.load.addEventListener("click", e => ifc_explorer_load_type(e));
}

function set_ifc_explorer_selected_expr(index, from_history) {
    if (null_index(index)) return;
    ifc_explorer_clear_content(ifc_explorer.exprs.content);
    if (!from_history) {
        update_history_with_new_element(ExprIndex, index.sort, index.index);
    }

    // Update edits.
    ifc_explorer.exprs.sort_dropdown.selectedIndex = Array
                                                      .from(ifc_explorer.exprs.sort_dropdown.options)
                                                      .findIndex(opt => opt.value == index.sort);
    ifc_explorer.exprs.index_edit.value = index.index;

    const symbolic = symbolic_for_expr_sort(index.sort);
    const symbolic_decl = sgraph.resolver.read(symbolic, index.index);
    const json_str = JSON.stringify(symbolic_decl, ifc_explorer_json_replacer, 2);
    const container = document.createElement("pre");
    const sort = sort_to_string(ExprIndex, index.sort);
    container.innerHTML = `ExprIndex{${index.sort}(${sort}),${index.index}}\n${json_str}`;
    ifc_explorer.exprs.content.appendChild(container);

    const tab = $(ifc_explorer.exprs.tab);
    tab.tab("show");
}

function ifc_explorer_load_expr(e) {
    mark_edit_valid(ifc_explorer.exprs.index_edit);
    const sort = parseInt(ifc_explorer.exprs.sort_dropdown.value);
    const value = ifc_explorer.exprs.index_edit.value;
    if (!valid_integral_value(value)) {
        mark_edit_invalid(ifc_explorer.exprs.index_edit, ifc_explorer.exprs.validation_tooltip, `"${value}" is not an integer value.`);
        return;
    }
    const index = parseInt(value);
    const expr_index = { sort: sort, index: index };
    const bounded = sgraph.resolver.expr_index_in_bounds(expr_index);
    if (!bounded.in_bounds) {
        const err = `ExprIndex{${sort}(${sort_to_string(ExprIndex, sort)}),${index}} is out of bounds (Cardinality: ${bounded.partition_size}).`;
        mark_edit_invalid(ifc_explorer.exprs.index_edit, ifc_explorer.exprs.validation_tooltip, err);
        return;
    }
    set_ifc_explorer_selected_expr(expr_index, false);
}

function ifc_explorer_init_exprs() {
    Object.entries(ExprIndex.Sort)
          .sort(function(lhs, rhs) {
                if (lhs[0] < rhs[0])
                    return -1;
                if (lhs[0] > rhs[0])
                    return 1;
                return 0;
          })
          .forEach(element => {
                if (element[1] != DeclIndex.Sort.Count) {
                    var entry = document.createElement("option");
                    entry.textContent = `${element[0]} - ${element[1]}`;
                    entry.value = element[1];
                    ifc_explorer.exprs.sort_dropdown.appendChild(entry);
                }
          });

    ifc_explorer.exprs.index_edit.addEventListener("keyup", e => { if (e.key === "Enter") ifc_explorer_load_expr(e); });
    ifc_explorer.exprs.load.addEventListener("click", e => ifc_explorer_load_expr(e));
}

function ifc_explorer_init_files() {
    const sourcefile_partition = sgraph.resolver.toc.partition(SourceFileName);
    if (sourcefile_partition == null) {
        return;
    }
    const count = sourcefile_partition.cardinality;
    var files = new Array();
    for (var i = 0; i < count; ++i) {
        const source_file = sgraph.resolver.read(SourceFileName, i);
        files.push(sgraph.resolver.string_table.get(source_file.name));
    }
    const json_str = JSON.stringify(files, null, 2);
    const container = document.createElement("pre");
    container.innerHTML = json_str;
    ifc_explorer.files.content.appendChild(container);
}

function ifc_explorer_init_header() {
    const json_str = JSON.stringify(sgraph.header, null, 2);
    const container = document.createElement("pre");
    container.innerHTML = json_str;
    ifc_explorer.header.content.appendChild(container);
}

function ifc_explorer_toc_replacer(key, value) {
    if (value instanceof Map)
        return Array.from(value.entries());
    return value;
}

function ifc_explorer_init_toc() {
    const json_str = JSON.stringify(sgraph.resolver.toc.partition_by_name_map, ifc_explorer_toc_replacer, 2);
    const container = document.createElement("pre");
    container.innerHTML = json_str;
    ifc_explorer.toc.content.appendChild(container);
}

function ifc_explorer_ifc_loaded() {
    ifc_explorer.button.hidden = false;
    ifc_explorer_init_decls();
    ifc_explorer_init_types();
    ifc_explorer_init_exprs();
    ifc_explorer_init_files();
    ifc_explorer_init_header();
    ifc_explorer_init_toc();
}

function init_ifc_explorer() {
    ifc_explorer.button.hidden = true;
}
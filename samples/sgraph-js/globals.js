// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

const file_selector = document.getElementById('file-selector');
const title = document.getElementById('title');
const output = document.getElementById('output');

// Sidebar
const name_filter = document.getElementById('name-filter');
const sort_filter = document.getElementById('sort-filter');
const center_view = document.getElementById('center-view');
const prop_filter_exported = document.getElementById('prop-filter-exported');
const prop_filter_non_exported = document.getElementById('prop-filter-non-exported');
const narrow_graph_context = document.getElementById('filter-narrow-context');

// Options dialog
const decl_color_dropdown = document.getElementById('decl-color-dropdown');
const decl_color_selector = document.getElementById('decl-color-selector');
const reset_decl_colors_btn = document.getElementById('reset-decl-colors');
const transpose_graph_toggle = document.getElementById('transpose-graph-toggle');
const class_color_dropdown = document.getElementById('class-color-dropdown');
const class_color_selector = document.getElementById('class-color-selector');
const reset_class_colors_btn = document.getElementById('reset-class-colors');
const filter_opacity_edit = document.getElementById('filter-opacity-edit');
const bad_filter_opacity_text = document.getElementById('bad-filter-opacity-text');
const graph_fps_toggle = document.getElementById('graph-fps-toggle');
const graph_animations = document.getElementById('graph-animations');

// Various helper constants.
const invalid_css_class = "is-invalid";

// IFC explorer dialog
const ifc_explorer = {
    button: document.getElementById('ifc-explorer-button'),
    history: {
        sidebar: document.getElementById('ifc-explorer-history'),
        stack: new Array(),
        max_history: 20
    },
    decls: {
        tab: '#ifc-explorer-decls-tab',
        content: document.getElementById('ifc-explorer-content-decls'),
        sort_dropdown: document.getElementById('ifc-explorer-content-decls-sort-dropdown'),
        index_edit: document.getElementById('ifc-explorer-content-decls-index-edit'),
        load: document.getElementById('ifc-explorer-content-decls-load'),
        validation_tooltip: document.getElementById('ifc-explorer-content-decls-load-validation')
    },
    types: {
        tab: '#ifc-explorer-types-tab',
        content: document.getElementById('ifc-explorer-content-types'),
        sort_dropdown: document.getElementById('ifc-explorer-content-types-sort-dropdown'),
        index_edit: document.getElementById('ifc-explorer-content-types-index-edit'),
        load: document.getElementById('ifc-explorer-content-types-load'),
        validation_tooltip: document.getElementById('ifc-explorer-content-types-load-validation')
    },
    exprs: {
        tab: '#ifc-explorer-exprs-tab',
        content: document.getElementById('ifc-explorer-content-exprs'),
        sort_dropdown: document.getElementById('ifc-explorer-content-exprs-sort-dropdown'),
        index_edit: document.getElementById('ifc-explorer-content-exprs-index-edit'),
        load: document.getElementById('ifc-explorer-content-exprs-load'),
        validation_tooltip: document.getElementById('ifc-explorer-content-exprs-load-validation')
    },
    files: {
        content: document.getElementById('ifc-explorer-content-ifc-files')
    },
    toc: {
        content: document.getElementById('ifc-explorer-content-toc')
    },
    header: {
        content: document.getElementById('ifc-explorer-content-ifc-header')
    }
};

// Scripting globals
var graph = {
    data: {
        original_data: null
    },
    drawing: {
        native_width: 1260.0,
        native_height: 800.0,
        custom_base_element: null,
        working_data: null,
        last_update_time: 0,
        timer: null,
        canvas_dirty: false,
        backing_canvas_dirty: false,
        node_mapper: null,
        partition: null
    },
    element: document.getElementById('icicle')
};
var graph_filter = null;
var options = null;
var sgraph = { resolver: null, header: null };
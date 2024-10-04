// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

function has_property(obj, prop) {
    return obj.hasOwnProperty(prop);
}

function is_object(obj) {
    return typeof obj === 'object';
}

function remove_all_children(content) {
    // Yes, it is intentional to check for the first and remove the last.
    while (content.firstChild != null) {
        content.removeChild(content.lastChild);
    }
}

function valid_integral_value(str) {
    if (str == "")
        return false;
    if (isNaN(str))
        return false;
    return true;
}

function mark_edit_valid(edit) {
    edit.classList.remove(invalid_css_class);
}

function mark_edit_invalid(edit, tooltip, reason) {
    tooltip.innerHTML = reason;
    edit.classList.add(invalid_css_class);
}

// Implementation pulled from: https://www.freecodecamp.org/news/javascript-debounce-example/
function debounce(func, timeout = 300) {
    let timer = undefined;
    return (...args) => {
        clearTimeout(timer);
        timer = setTimeout(() => { func.apply(this, args); }, timeout);
    };
}
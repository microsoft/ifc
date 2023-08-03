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

// Implementation pulled from: https://www.freecodecamp.org/news/javascript-debounce-example/
function debounce(func, timeout = 300) {
    let timer = undefined;
    return (...args) => {
        clearTimeout(timer);
        timer = setTimeout(() => { func.apply(this, args); }, timeout);
    };
}
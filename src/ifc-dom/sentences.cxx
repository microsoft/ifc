// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "common.hxx"

namespace ifc::util {
    void load(Loader&, Node& node, SentenceIndex index)
    {
        // At some point all of the token based initializers will be going
        // away. If that does not happen. We can add token loading here.
        node.id = to_string(index);
    }
} // namespace ifc::util

/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_FIELD
#define SKSL_FIELD

#include "src/sksl/SkSLPosition.h"
#include "src/sksl/ir/SkSLModifiers.h"
#include "src/sksl/ir/SkSLSymbol.h"
#include "src/sksl/ir/SkSLType.h"
#include "src/sksl/ir/SkSLVariable.h"

namespace SkSL {

/**
 * A symbol which should be interpreted as a field access. Fields are added to the symboltable
 * whenever a bare reference to an identifier should refer to a struct field; in GLSL, this is the
 * result of declaring anonymous interface blocks.
 */
struct Field : public Symbol {
    Field(int offset, const Variable& owner, int fieldIndex)
    : INHERITED(offset, kField_Kind, owner.fType.fields()[fieldIndex].fName)
    , fOwner(owner)
    , fFieldIndex(fieldIndex) {}

#ifdef SK_DEBUG
    virtual String description() const override {
        return fOwner.description() + "." + fOwner.fType.fields()[fFieldIndex].fName;
    }
#endif

    const Variable& fOwner;
    const int fFieldIndex;

    typedef Symbol INHERITED;
};

} // namespace SkSL

#endif
